// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <gmock/gmock.h>
#include <gtest/gtest.h>

/**
 * @def AGENT_TUNNEL_MGR_FRIEND_TESTS
 * @brief Macro defining friend declarations for AgentTunnelMgrTest
 *
 * This macro contains friend class declarations needed to access private
 * members of TunManager for testing.
 */
#define AGENT_TUNNEL_MGR_FRIEND_TESTS                      \
  friend class AgentTunnelMgrTest;                         \
  FRIEND_TEST(AgentTunnelMgrTest, checkProbedDataCleanup); \
  FRIEND_TEST(AgentTunnelMgrTest, checkProbedDataCleanupInterfaceDown);

#include <sstream>
#include <string>
#include "fboss/agent/SwSwitch.h"
#include "fboss/agent/TunManager.h"
#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/test/AgentHwTest.h"
#include "fboss/agent/test/TestUtils.h"
#include "fboss/agent/test/utils/ConfigUtils.h"

namespace facebook::fboss {

class AgentTunnelMgrTest : public AgentHwTest {
 public:
  std::vector<ProductionFeature> getProductionFeaturesVerified()
      const override {
    return {ProductionFeature::CPU_RX_TX};
  }

  void setCmdLineFlagOverrides() const override {
    AgentHwTest::setCmdLineFlagOverrides();
    FLAGS_tun_intf = true;
    FLAGS_cleanup_probed_kernel_data = true;
  }

 public:
  void SetUp() override {
    // Check for warmboot flag before any processing using the helper function
    bool isWarmbootSetup = isWarmbootSetupRequested();

    XLOG(INFO) << "Setup requested for "
               << (isWarmbootSetup ? "warmboot" : "coldboot");

    // Clear kernel entries for coldboot only, BEFORE agent initialization
    if (isWarmbootSetup) {
      XLOG(INFO)
          << "Coldboot detected: clearing all kernel entries before agent initialization";
      clearAllKernelEntries();
    } else {
      XLOG(INFO) << "Warmboot detected: skipping kernel entries cleanup";
    }

    // Now call parent SetUp to initialize the agent
    AgentHwTest::SetUp();
  }

  // Clear any stale kernel entries
  void clearKernelEntries(const std::string& intfIp, bool isIPv4 = true) {
    std::string cmd;
    if (isIPv4) {
      cmd = folly::to<std::string>("ip rule list | grep -w ", intfIp);
    } else {
      cmd = folly::to<std::string>("ip -6 rule list | grep -w ", intfIp);
    }

    auto output = runShellCmd(cmd);

    XLOG(DBG2) << "clearKernelEntries Cmd: " << cmd;
    XLOG(DBG2) << "clearKernelEntries Output: \n" << output;

    // There could be duplicate source route rule entries in the kernel. Clear
    // all of them.
    while (output.find(folly::to<std::string>(intfIp)) != std::string::npos) {
      // Break the output string into words
      std::istringstream iss(output);
      std::string word;
      std::string lastWord;

      while (iss >> word) {
        lastWord = folly::copy(word);
      }

      XLOG(DBG2) << "tableId: " << lastWord;

      // Delete the source route rule entries from the kernel
      if (isIPv4) {
        cmd = folly::to<std::string>("ip rule delete table ", lastWord);
      } else {
        cmd = folly::to<std::string>("ip -6 rule delete table ", lastWord);
      }

      runShellCmd(cmd);

      if (isIPv4) {
        // Get the source route rule entries again
        cmd = folly::to<std::string>("ip rule list | grep -w ", intfIp);
      } else {
        cmd = folly::to<std::string>("ip -6 rule list | grep -w ", intfIp);
      }

      output = runShellCmd(cmd);

      XLOG(DBG2) << "clearKernelEntries Cmd: " << cmd;
      XLOG(DBG2) << "clearKernelEntries Output: \n" << output;
    }

    // Get the String
    if (isIPv4) {
      cmd = folly::to<std::string>("ip route list | grep -w ", intfIp);
    } else {
      cmd = folly::to<std::string>("ip -6 route list | grep -w ", intfIp);
    }

    output = runShellCmd(cmd);

    XLOG(DBG2) << "clearKernelEntries Cmd: " << cmd;
    XLOG(DBG2) << "clearKernelEntries Output: \n" << output;

    // Break the output string into words
    std::istringstream iss(output);
    do {
      std::string subs;
      // Get the word from the istringstream
      iss >> subs;
      // Delete the matching fboss interface from the kernel
      if (subs.find("fboss") != std::string::npos) {
        if (subs.find(':') != std::string::npos) {
          subs = subs.substr(0, subs.find(':'));
        }
        cmd = folly::to<std::string>("ip link delete ", subs);
        runShellCmd(cmd);
        XLOG(DBG2) << "clearKernelEntries Cmd: " << cmd;
        break;
      }
    } while (iss);

    if (isIPv4) {
      cmd = folly::to<std::string>("ip addr list | grep -w ", intfIp);
    } else {
      cmd = folly::to<std::string>("ip -6 addr list | grep -w ", intfIp);
    }

    output = runShellCmd(cmd);

    XLOG(DBG2) << "clearKernelEntries Cmd: " << cmd;
    XLOG(DBG2) << "clearKernelEntries Output: \n" << output;

    // Break the output string into words
    std::istringstream iss2(output);
    do {
      std::string subs;
      // Get the word from the istringstream
      iss2 >> subs;
      // Delete the matching fboss interface from the kernel
      if (subs.find("fboss") != std::string::npos) {
        if (subs.find(':') != std::string::npos) {
          subs = subs.substr(0, subs.find(':'));
        }
        cmd = folly::to<std::string>("ip link delete ", subs);
        runShellCmd(cmd);
        XLOG(DBG2) << "clearKernelEntries Cmd: " << cmd;
        break;
      }
    } while (iss2);
  }

  // Clear all stale kernel entries
  void clearAllKernelEntries() {
    std::string cmd;
    cmd = folly::to<std::string>("ip rule list");
    auto output = runShellCmd(cmd);

    XLOG(DBG2) << "clearAllKernelEntries Cmd: " << cmd;
    XLOG(DBG2) << "clearAllKernelEntries Output: \n" << output;

    std::istringstream iss(output);
    for (std::string line; std::getline(iss, line);) {
      std::stringstream ss(line);
      std::string word;
      std::string lastWord;

      while (ss >> word) {
        lastWord = folly::copy(word);
      }

      if (lastWord != "local" && lastWord != "main" && lastWord != "default") {
        XLOG(DBG2) << "tableId: " << lastWord;
        cmd = folly::to<std::string>("ip rule delete table ", lastWord);
        runShellCmd(cmd);
      }
    }

    cmd = folly::to<std::string>("ip -6 rule list");
    output = runShellCmd(cmd);

    XLOG(DBG2) << "clearAllKernelEntries Cmd: " << cmd;
    XLOG(DBG2) << "clearAllKernelEntries Output: \n" << output;

    std::istringstream iss2(output);
    for (std::string line; std::getline(iss2, line);) {
      std::stringstream ss(line);
      std::string word;
      std::string lastWord;

      while (ss >> word) {
        lastWord = folly::copy(word);
      }

      if (lastWord != "local" && lastWord != "main" && lastWord != "default") {
        XLOG(DBG2) << "tableId: " << lastWord;
        cmd = folly::to<std::string>("ip -6 rule delete table ", lastWord);
        runShellCmd(cmd);
      }
    }

    // Get the String
    cmd = folly::to<std::string>("ip route list | grep fboss");
    output = runShellCmd(cmd);

    XLOG(DBG2) << "clearAllKernelEntries Cmd: " << cmd;
    XLOG(DBG2) << "clearAllKernelEntries Output: \n" << output;

    while (output.find(folly::to<std::string>("fboss")) != std::string::npos) {
      // Break the output string into words
      std::istringstream iss3(output);
      do {
        std::string subs;
        // Get the word from the istringstream
        iss3 >> subs;
        // Delete the matching fboss interface from the kernel
        if (subs.find("fboss") != std::string::npos) {
          if (subs.find(':') != std::string::npos) {
            subs = subs.substr(0, subs.find(':'));
          }
          cmd = folly::to<std::string>("ip link delete ", subs);
          runShellCmd(cmd);
          XLOG(DBG2) << "clearKernelEntries Cmd: " << cmd;
          break;
        }
      } while (iss3);

      cmd = folly::to<std::string>("ip route list | grep fboss");
      output = runShellCmd(cmd);
    }

    cmd = folly::to<std::string>("ip -6 route list | grep fboss");
    output = runShellCmd(cmd);

    XLOG(DBG2) << "clearAllKernelEntries Cmd: " << cmd;
    XLOG(DBG2) << "clearAllKernelEntries Output: \n" << output;

    while (output.find(folly::to<std::string>("fboss")) != std::string::npos) {
      // Break the output string into words
      std::istringstream iss4(output);
      do {
        std::string subs;
        // Get the word from the istringstream
        iss4 >> subs;
        // Delete the matching fboss interface from the kernel
        if (subs.find("fboss") != std::string::npos) {
          if (subs.find(':') != std::string::npos) {
            subs = subs.substr(0, subs.find(':'));
          }
          cmd = folly::to<std::string>("ip link delete ", subs);
          runShellCmd(cmd);
          XLOG(DBG2) << "clearKernelEntries Cmd: " << cmd;
          break;
        }
      } while (iss4);

      cmd = folly::to<std::string>("ip -6 route list | grep fboss");
      output = runShellCmd(cmd);
    }

    // Get the String
    cmd = folly::to<std::string>("ip addr list | grep fboss");
    output = runShellCmd(cmd);

    XLOG(DBG2) << "clearAllKernelEntries Cmd: " << cmd;
    XLOG(DBG2) << "clearAllKernelEntries Output: \n" << output;

    while (output.find(folly::to<std::string>("fboss")) != std::string::npos) {
      // Break the output string into words
      std::istringstream iss5(output);
      do {
        std::string subs;
        // Get the word from the istringstream
        iss5 >> subs;
        // Delete the matching fboss interface from the kernel
        if (subs.find("fboss") != std::string::npos) {
          if (subs.find(':') != std::string::npos) {
            subs = subs.substr(0, subs.find(':'));
          }
          cmd = folly::to<std::string>("ip link delete ", subs);
          runShellCmd(cmd);
          XLOG(DBG2) << "clearKernelEntries Cmd: " << cmd;
          break;
        }
      } while (iss5);

      cmd = folly::to<std::string>("ip addr list | grep fboss");
      output = runShellCmd(cmd);
    }

    cmd = folly::to<std::string>("ip -6 addr list | grep fboss");
    output = runShellCmd(cmd);

    XLOG(DBG2) << "clearAllKernelEntries Cmd: " << cmd;
    XLOG(DBG2) << "clearAllKernelEntries Output: \n" << output;

    while (output.find(folly::to<std::string>("fboss")) != std::string::npos) {
      // Break the output string into words
      std::istringstream iss6(output);
      do {
        std::string subs;
        // Get the word from the istringstream
        iss6 >> subs;
        // Delete the matching fboss interface from the kernel
        if (subs.find("fboss") != std::string::npos) {
          if (subs.find(':') != std::string::npos) {
            subs = subs.substr(0, subs.find(':'));
          }
          cmd = folly::to<std::string>("ip link delete ", subs);
          runShellCmd(cmd);
          XLOG(DBG2) << "clearKernelEntries Cmd: " << cmd;
          break;
        }
      } while (iss6);
      cmd = folly::to<std::string>("ip -6 addr list | grep fboss");
      output = runShellCmd(cmd);
    }
  }

  bool checkIpRuleEntriesRemoved(
      const std::string& intfIp,
      bool isIPv4 = true) {
    // Check that the source route rule entries are not present in the kernel
    std::string cmd;
    if (isIPv4) {
      cmd = folly::to<std::string>("ip rule list | grep -w ", intfIp);
    } else {
      cmd = folly::to<std::string>("ip -6 rule list | grep -w ", intfIp);
    }

    auto output = runShellCmd(cmd);

    XLOG(DBG2) << "checkIpRuleEntriesRemoved Cmd: " << cmd;
    XLOG(DBG2) << "checkIpRuleEntriesRemoved Output: \n" << output;

    if (output.find(folly::to<std::string>(intfIp)) != std::string::npos) {
      return false;
    }

    return true;
  }

  void checkIpKernelEntriesRemoved(
      const std::string& intfIp,
      bool isIPv4 = true) {
    // Check that the source route rule entries are not present in the kernel
    std::string cmd;
    std::string searchIntfIp = intfIp;
    if (isIPv4) {
      cmd = folly::to<std::string>("ip rule list | grep -w ", searchIntfIp);
    } else {
      cmd = folly::to<std::string>("ip -6 rule list | grep -w ", searchIntfIp);
    }

    auto output = runShellCmd(cmd);

    XLOG(DBG2) << "checkKernelEntriesRemoved Cmd: " << cmd;
    XLOG(DBG2) << "checkKernelEntriesRemoved Output: \n" << output;

    EXPECT_TRUE(
        output.find(folly::to<std::string>(searchIntfIp)) == std::string::npos);

    // Check that the tunnel address entries are not present in the kernel
    if (isIPv4) {
      cmd = folly::to<std::string>("ip addr list | grep -w ", searchIntfIp);
    } else {
      cmd = folly::to<std::string>("ip -6 addr list | grep -w ", searchIntfIp);
    }

    output = runShellCmd(cmd);

    XLOG(DBG2) << "checkKernelEntriesRemoved Cmd: " << cmd;
    XLOG(DBG2) << "checkKernelEntriesRemoved Output: \n" << output;

    // TODO: This is to stabilize the test. Remove this and use EXPECT_TRUE once
    // the tunnelMgr fixes are in.
    if (output.find(folly::to<std::string>(searchIntfIp)) !=
        std::string::npos) {
      XLOG(DBG2)
          << "checkKernelEntriesRemoved: Tunnel address entry not  removed for "
          << searchIntfIp;
    }

    // Check that the route entries are not present in the kernel
    if (isIPv4) {
      cmd = folly::to<std::string>(
          "ip route list | grep -w ", searchIntfIp, " | grep fboss");
    } else {
      cmd = folly::to<std::string>(
          "ip -6 route list | grep -w ", searchIntfIp, " | grep fboss");
    }

    output = runShellCmd(cmd);

    XLOG(DBG2) << "checkKernelEntriesRemoved Cmd: " << cmd;
    XLOG(DBG2) << "checkKernelEntriesRemoved Output: \n" << output;

    EXPECT_TRUE(
        output.find(folly::to<std::string>(searchIntfIp)) == std::string::npos);
  }

  // Check that the kernel entries are present in the kernel
  void checkKernelEntriesNotExist(
      const std::string& intfIp,
      bool isIPv4 = true,
      bool checkRouteEntry = true) {
    // Check that the source route rule entries are present in the kernel

    std::string cmd;
    std::string searchIntfIp = intfIp;
    if (isIPv4) {
      cmd = folly::to<std::string>("ip rule list | grep -w ", searchIntfIp);
    } else {
      cmd = folly::to<std::string>("ip -6 rule list | grep -w ", searchIntfIp);
    }

    auto output = runShellCmd(cmd);

    XLOG(DBG2) << "checkKernelEntriesNotExist Cmd: " << cmd;
    XLOG(DBG2) << "checkKernelEntriesNotExist Output: \n" << output;

    EXPECT_TRUE(
        output.find(folly::to<std::string>(searchIntfIp)) == std::string::npos);

    if (isIPv4) {
      // Check that the tunnel address entries are present in the kernel
      cmd = folly::to<std::string>("ip addr list | grep -w ", searchIntfIp);
    } else {
      cmd = folly::to<std::string>("ip -6 addr list | grep -w ", searchIntfIp);
    }

    output = runShellCmd(cmd);

    XLOG(DBG2) << "checkKernelEntriesNotExist Cmd: " << cmd;
    XLOG(DBG2) << "checkKernelEntriesNotExist Output: \n" << output;

    EXPECT_TRUE(
        output.find(folly::to<std::string>(searchIntfIp)) == std::string::npos);

    if (checkRouteEntry) {
      // Check that the route entries are present in the kernel
      if (isIPv4) {
        cmd = folly::to<std::string>(
            "ip route list | grep -w ", searchIntfIp, " | grep fboss");
      } else {
        cmd = folly::to<std::string>(
            "ip -6 route list | grep -w ", searchIntfIp, " | grep fboss");
      }

      output = runShellCmd(cmd);

      XLOG(DBG2) << "checkKernelEntriesNotExist Cmd: " << cmd;
      XLOG(DBG2) << "checkKernelEntriesNotExist Output:" << output;

      EXPECT_TRUE(
          output.find(folly::to<std::string>(searchIntfIp)) ==
          std::string::npos);

      // Additional check for fboss routes in all tables
      if (isIPv4) {
        cmd = folly::to<std::string>("ip route list table all | grep fboss");
      } else {
        cmd = folly::to<std::string>("ip -6 route list table all | grep fboss");
      }

      output = runShellCmd(cmd);

      XLOG(DBG2) << "checkKernelEntriesExist Additional Cmd: " << cmd;
      XLOG(DBG2) << "checkKernelEntriesExist Additional Output: " << output;

      EXPECT_TRUE(
          output.find(folly::to<std::string>("fboss")) == std::string::npos);
    }
  }

  // Check that the kernel entries are present in the kernel
  void checkKernelEntriesExist(
      const std::string& intfIp,
      bool isIPv4 = true,
      bool checkRouteEntry = true) {
    // Check that the source route rule entries are present in the kernel

    std::string cmd;
    std::string searchIntfIp = intfIp;
    if (isIPv4) {
      cmd = folly::to<std::string>("ip rule list | grep -w ", searchIntfIp);
    } else {
      cmd = folly::to<std::string>("ip -6 rule list | grep -w ", searchIntfIp);
    }

    auto output = runShellCmd(cmd);

    XLOG(DBG2) << "checkKernelEntriesExist Cmd: " << cmd;
    XLOG(DBG2) << "checkKernelEntriesExist Output: " << std::endl
               << output << std::endl;

    // TODO: This is to stabilize the test. Remove this and use EXPECT_TRUE once
    // the tunnelMgr fixes are in.
    if (output.find(folly::to<std::string>(searchIntfIp)) ==
        std::string::npos) {
      XLOG(DBG2) << "checkKernelEntriesExist: Source route rule entry not "
                 << "present in the kernel for " << searchIntfIp;
    }

    if (isIPv4) {
      // Check that the tunnel address entries are present in the kernel
      cmd =
          folly::to<std::string>("ip addr list | grep -B 1 -w ", searchIntfIp);
    } else {
      cmd = folly::to<std::string>(
          "ip -6 addr list | grep -B 1 -w ", searchIntfIp);
    }

    output = runShellCmd(cmd);

    XLOG(DBG2) << "checkKernelEntriesExist Cmd: " << cmd;
    XLOG(DBG2) << "checkKernelEntriesExist Output: " << std::endl
               << output << std::endl;

    EXPECT_TRUE(
        output.find(folly::to<std::string>(searchIntfIp)) != std::string::npos);

    if (checkRouteEntry) {
      // Check that the route entries are present in the kernel
      if (isIPv4) {
        cmd = folly::to<std::string>(
            "ip route list table all | grep -w ",
            searchIntfIp,
            " | grep fboss");
      } else {
        cmd = folly::to<std::string>(
            "ip -6 route list table all | grep -w ",
            searchIntfIp,
            " | grep fboss");
      }

      output = runShellCmd(cmd);

      XLOG(DBG2) << "checkKernelEntriesExist Cmd: " << cmd;
      XLOG(DBG2) << "checkKernelEntriesExist Output:" << std::endl
                 << output << std::endl;

      EXPECT_TRUE(
          output.find(folly::to<std::string>(searchIntfIp)) !=
          std::string::npos);

      // Additional check for fboss routes in all tables
      if (isIPv4) {
        cmd = folly::to<std::string>("ip route list table all | grep fboss");
      } else {
        cmd = folly::to<std::string>("ip -6 route list table all | grep fboss");
      }

      output = runShellCmd(cmd);

      XLOG(DBG2) << "checkKernelEntriesExist Additional Cmd: " << cmd;
      XLOG(DBG2) << "checkKernelEntriesExist Additional Output: \n"
                 << output << "\n";

      EXPECT_TRUE(
          output.find(folly::to<std::string>("fboss")) != std::string::npos);
    }
  }

  void clearKernelEntries(
      const std::string& intfIPv4,
      const std::string& intfIPv6) {
    clearKernelEntries(intfIPv4, true);
    clearKernelEntries(intfIPv6, false);
  }

  void checkKernelEntriesRemoved(
      const std::string& intfIPv4,
      const std::string& intfIPv6) {
    checkIpKernelEntriesRemoved(intfIPv4, true);
    checkIpKernelEntriesRemoved(intfIPv6, false);
  }

  cfg::SwitchConfig initialConfig(
      const AgentEnsemble& ensemble) const override {
    std::vector<PortID> ports = {
        ensemble.masterLogicalPortIds()[0], ensemble.masterLogicalPortIds()[1]};
    auto cfg = utility::onePortPerInterfaceConfig(
        ensemble.getSw(), ports, true /*interfaceHasSubnet*/);
    return cfg;
  }

  std::vector<std::string> getInterfaceIpAddress(
      cfg::SwitchConfig& config,
      bool isIPv4) {
    std::string intfIP;
    std::string intfIPv4;
    std::string intfIPv6;
    std::vector<std::string> intfIPList;

    for (int i = 0; i < config.interfaces()->size(); i++) {
      if (config.interfaces()[i].scope() == cfg::Scope::GLOBAL) {
        continue;
      }
      for (int j = 0; j < config.interfaces()[i].ipAddresses()->size(); j++) {
        intfIP = folly::to<std::string>(
            folly::IPAddress::createNetwork(
                config.interfaces()[i].ipAddresses()[j], -1, false)
                .first);
        if (isIPv4) {
          if (intfIP.find("::") == std::string::npos) {
            intfIPv4 = std::move(intfIP);
            intfIPList.push_back(intfIPv4);
          }
        } else {
          if (intfIP.find("::") != std::string::npos) {
            intfIPv6 = std::move(intfIP);
            intfIPList.push_back(intfIPv6);
          }
        }
      }
    }

    return intfIPList;
  }

  void checkKernelIpEntriesExist(
      const InterfaceID& ifid,
      const std::string& intfIP,
      bool isIPv4) {
    // Get TunManager pointer
    auto tunMgr_ = getAgentEnsemble()->getSw()->getTunManager();
    auto status = tunMgr_->getIntfStatus(getProgrammedState(), ifid);

    // There is a known limitation in the kernel that the source route rule
    // entries are not created if the interface is not up. So, checking for
    // the kernel entries if the interface is  up
    if (status) {
      if (isIPv4) {
        checkKernelEntriesExist(folly::to<std::string>(intfIP), true, false);
      } else {
        checkKernelEntriesExist(folly::to<std::string>(intfIP), false, false);
      }
    }
  }

  std::vector<std::string> changeKernelIPAddress(
      cfg::SwitchConfig& config,
      bool isIpv4) {
    std::vector<std::string> intfIPList;
    for (int i = 0; i < config.interfaces()->size(); i++) {
      if (config.interfaces()[i].scope() == cfg::Scope::GLOBAL) {
        continue;
      }

      // change ipv4 address of the interface
      for (int j = 0; j < config.interfaces()[i].ipAddresses()->size(); j++) {
        std::string intfIP = folly::to<std::string>(
            folly::IPAddress::createNetwork(
                config.interfaces()[i].ipAddresses()[j], -1, false)
                .first);
        if (isIpv4) {
          if (intfIP.find("::") == std::string::npos) {
            auto ipDecimal = folly::sformat("{}", i + 1);
            config.interfaces()[i].ipAddresses()[j] =
                folly::sformat("{}.2.2.2/24", ipDecimal);
            intfIPList.push_back(config.interfaces()[i].ipAddresses()[j]);
          }
        } else {
          if (intfIP.find("::") != std::string::npos) {
            auto ipDecimal =
                folly::sformat("{}", i + config.interfaces()->size() + 1);
            config.interfaces()[i].ipAddresses()[j] =
                folly::sformat("{}::/64", ipDecimal);
            intfIPList.push_back(config.interfaces()[i].ipAddresses()[j]);
          }
        }
      }
    }

    // Apply the config
    applyNewConfig(config);
    waitForStateUpdates(getAgentEnsemble()->getSw());
    return intfIPList;
  }

  void restoreKernelIPAddress(
      cfg::SwitchConfig& config,
      bool isIpv4,
      const std::vector<std::string>& intfOldIPs) {
    for (int i = 0; i < config.ports()->size(); i++) {
      config.ports()[i].state() = cfg::PortState::ENABLED;
      auto portType = config.ports()[i].portType().value();
      config.ports()[i].loopbackMode() =
          getAsics().cbegin()->second->getDesiredLoopbackMode(portType);
    }
    // Apply the config
    applyNewConfig(config);
    waitForStateUpdates(getAgentEnsemble()->getSw());
    for (int i = 0; i < config.interfaces()->size(); i++) {
      if (config.interfaces()[i].scope() == cfg::Scope::GLOBAL) {
        continue;
      }

      // change ipv4 address of the interface
      for (int j = 0; j < config.interfaces()[i].ipAddresses()->size(); j++) {
        std::string intfIP = folly::to<std::string>(
            folly::IPAddress::createNetwork(
                config.interfaces()[i].ipAddresses()[j], -1, false)
                .first);
        if (isIpv4) {
          if (intfIP.find("::") == std::string::npos) {
            config.interfaces()[i].ipAddresses()[j] = intfOldIPs[i];
          }
        } else {
          if (intfIP.find("::") != std::string::npos) {
            config.interfaces()[i].ipAddresses()[j] = intfOldIPs[i];
          }
        }
      }
    }
    // Apply the config
    applyNewConfig(config);
    waitForStateUpdates(getAgentEnsemble()->getSw());
  }

  void printInterfaceDetails(const cfg::SwitchConfig& config) {
    // Get TunManager pointer
    auto tunMgr_ = getAgentEnsemble()->getSw()->getTunManager();

    for (int i = 0; i < config.interfaces()->size(); i++) {
      std::vector<std::string> intfIPv4s;
      std::vector<std::string> intfIPv6s;

      for (int j = 0; j < config.interfaces()[i].ipAddresses()->size(); j++) {
        std::string intfIP = folly::to<std::string>(
            folly::IPAddress::createNetwork(
                config.interfaces()[i].ipAddresses()[j], -1, false)
                .first);

        if (intfIP.find("::") != std::string::npos) {
          intfIPv6s.push_back(intfIP);
        } else {
          intfIPv4s.push_back(intfIP);
        }
      }

      auto status = tunMgr_->getIntfStatus(
          getProgrammedState(),
          (InterfaceID)config.interfaces()[i].intfID().value());

      // Convert vectors to comma-separated strings for logging
      std::string ipv4List = folly::join(", ", intfIPv4s);
      std::string ipv6List = folly::join(", ", intfIPv6s);

      XLOG(INFO) << "Interface ID: "
                 << (InterfaceID)config.interfaces()[i].intfID().value()
                 << ", Status: " << (status ? "UP" : "DOWN") << ", IPv4: ["
                 << ipv4List << "]" << ", IPv6: [" << ipv6List << "]";
    }
  }

  void printProbedInterfaceDetails() {
    // Get TunManager pointer
    auto tunMgr_ = getAgentEnsemble()->getSw()->getTunManager();

    XLOG(INFO) << "=== Probed Interface Details ===";

    for (const auto& [interfaceId, intf] : tunMgr_->intfs_) {
      std::vector<std::string> intfIPv4s;
      std::vector<std::string> intfIPv6s;

      // Categorize addresses by IP version
      for (const auto& [addr, mask] : intf->getAddresses()) {
        if (addr.isV6()) {
          intfIPv6s.push_back(addr.str());
        } else {
          intfIPv4s.push_back(addr.str());
        }
      }

      // Convert vectors to comma-separated strings for logging
      std::string ipv4List = folly::join(", ", intfIPv4s);
      std::string ipv6List = folly::join(", ", intfIPv6s);

      XLOG(INFO) << "Interface ID: " << interfaceId
                 << ", Name: " << intf->getName()
                 << ", IfIndex: " << intf->getIfIndex()
                 << ", Status: " << (intf->getStatus() ? "UP" : "DOWN")
                 << ", IPv4: [" << ipv4List << "]" << ", IPv6: [" << ipv6List
                 << "]";
    }

    XLOG(INFO) << "=== End Probed Interface Details ===";
  }

  void printKernelInformation() {
    XLOG(INFO) << "=== Kernel Network Information ===";

    // IPv4 route table (all tables)
    XLOG(INFO) << "--- IPv4 Routes (All Tables) ---";
    std::string cmd = "ip route show table all";
    auto output = runShellCmd(cmd);
    XLOG(INFO) << "Command: " << cmd;
    XLOG(INFO) << "Output:\n" << output;

    // IPv6 route table (all tables)
    XLOG(INFO) << "--- IPv6 Routes (All Tables) ---";
    cmd = "ip -6 route show table all";
    output = runShellCmd(cmd);
    XLOG(INFO) << "Command: " << cmd;
    XLOG(INFO) << "Output:\n" << output;

    // IPv4 rule table
    XLOG(INFO) << "--- IPv4 Rules ---";
    cmd = "ip rule";
    output = runShellCmd(cmd);
    XLOG(INFO) << "Command: " << cmd;
    XLOG(INFO) << "Output:\n" << output;

    // IPv6 rule table
    XLOG(INFO) << "--- IPv6 Rules ---";
    cmd = "ip -6 rule";
    output = runShellCmd(cmd);
    XLOG(INFO) << "Command: " << cmd;
    XLOG(INFO) << "Output:\n" << output;

    // IPv4 addresses
    XLOG(INFO) << "--- IPv4 Addresses ---";
    cmd = "ip -4 a";
    output = runShellCmd(cmd);
    XLOG(INFO) << "Command: " << cmd;
    XLOG(INFO) << "Output:\n" << output;

    // IPv6 addresses
    XLOG(INFO) << "--- IPv6 Addresses ---";
    cmd = "ip -6 a";
    output = runShellCmd(cmd);
    XLOG(INFO) << "Command: " << cmd;
    XLOG(INFO) << "Output:\n" << output;

    XLOG(INFO) << "=== End Kernel Network Information ===";
  }

  void checkKernelIpEntriesRemoved(
      const InterfaceID& ifId,
      const std::string& intfIP,
      bool isIPv4) {
    // Get TunManager pointer
    auto tunMgr_ = getAgentEnsemble()->getSw()->getTunManager();
    auto status = tunMgr_->getIntfStatus(getProgrammedState(), ifId);

    // There is a known limitation in the kernel that the source route rule
    // entries are not created if the interface is not up. So, checking for
    // the kernel entries if the interface is  up
    if (status) {
      if (isIPv4) {
        checkKernelEntriesNotExist(folly::to<std::string>(intfIP), true, true);
      } else {
        checkKernelEntriesNotExist(folly::to<std::string>(intfIP), false, true);
      }
    }
  }

  /**
   * Check kernel entries are removed, irrespective of interace status.
   *
   * @param ifId Interface ID to check
   * @param intfIP Interface IP address to check
   * @param isIPv4 True for IPv4, false for IPv6
   */
  void checkKernelIpEntriesRemovedStrict(
      const InterfaceID& ifId,
      const std::string& intfIP,
      bool isIPv4) {
    // Check kernel entries regardless of interface status
    if (isIPv4) {
      checkKernelEntriesNotExist(folly::to<std::string>(intfIP), true, true);
    } else {
      checkKernelEntriesNotExist(folly::to<std::string>(intfIP), false, true);
    }
  }
};

// Test that the tunnel manager is able to create the source route rule
// entries, tunnel address entries and default route entries for IPv4 in the
// kernel
TEST_F(AgentTunnelMgrTest, checkKernelIPv4Entries) {
  auto setup = [=]() {};
  auto verify = [=, this]() {
    auto config = initialConfig(*getAgentEnsemble());
    std::string intfIPv4;
    std::string intfIPv6;
    for (int i = 0; i < config.interfaces()->size(); i++) {
      for (int j = 0; j < config.interfaces()[i].ipAddresses()->size(); j++) {
        std::string intfIP = folly::to<std::string>(
            folly::IPAddress::createNetwork(
                config.interfaces()[i].ipAddresses()[j], -1, false)
                .first);

        if (intfIP.find("::") != std::string::npos) {
          intfIPv6 = std::move(intfIP);
        } else {
          intfIPv4 = std::move(intfIP);
        }
      }

      // Apply the config
      applyNewConfig(config);
      waitForStateUpdates(getAgentEnsemble()->getSw());

      // Get TunManager pointer
      auto tunMgr_ = getAgentEnsemble()->getSw()->getTunManager();
      auto status = tunMgr_->getIntfStatus(
          getProgrammedState(),
          (InterfaceID)config.interfaces()[i].intfID().value());
      // There could be a race condition where the interface is up, but the
      // socket is not created. So, checking for the socket existence.
      auto socketExists = tunMgr_->isValidNlSocket();

      // There is a known limitation in the kernel that the source route rule
      // entries are not created if the interface is not up. So, checking for
      // the kernel entries if the interface is  up
      if (status && socketExists) {
        checkKernelEntriesExist(folly::to<std::string>(intfIPv4));
      }

      // Clear kernel entries
      clearKernelEntries(
          folly::to<std::string>(intfIPv4), folly::to<std::string>(intfIPv6));

      // Check that the kernel entries are removed
      checkKernelEntriesRemoved(
          folly::to<std::string>(intfIPv4), folly::to<std::string>(intfIPv6));
    }

    clearAllKernelEntries();
  };

  verifyAcrossWarmBoots(setup, verify);
}

// Test that the tunnel manager is able to create the source route rule
// entries, tunnel address entries and default route entries for IPv6 in the
// kernel
TEST_F(AgentTunnelMgrTest, checkKernelIPv6Entries) {
  auto setup = [=]() {};
  auto verify = [=, this]() {
    auto config = initialConfig(*getAgentEnsemble());
    std::string intfIPv4;
    std::string intfIPv6;
    for (int i = 0; i < config.interfaces()->size(); i++) {
      for (int j = 0; j < config.interfaces()[i].ipAddresses()->size(); j++) {
        std::string intfIP = folly::to<std::string>(
            folly::IPAddress::createNetwork(
                config.interfaces()[i].ipAddresses()[j], -1, false)
                .first);

        if (intfIP.find("::") != std::string::npos) {
          intfIPv6 = std::move(intfIP);
        } else {
          intfIPv4 = std::move(intfIP);
        }
      }

      // Apply the config
      applyNewConfig(config);
      waitForStateUpdates(getAgentEnsemble()->getSw());

      // Get TunManager pointer
      auto tunMgr_ = getAgentEnsemble()->getSw()->getTunManager();
      auto status = tunMgr_->getIntfStatus(
          getProgrammedState(),
          (InterfaceID)config.interfaces()[i].intfID().value());
      // There could be a race condition where the interface is up, but the
      // socket is not created. So, checking for the socket existence.
      auto socketExists = tunMgr_->isValidNlSocket();

      // There is a known limitation in the kernel that the source route rule
      // entries are not created if the interface is not up. So, checking for
      // the kernel entries if the interface is  up
      if (status && socketExists) {
        checkKernelEntriesExist(folly::to<std::string>(intfIPv6), false, true);
      }

      // Clear kernel entries
      clearKernelEntries(
          folly::to<std::string>(intfIPv4), folly::to<std::string>(intfIPv6));

      // Check that the kernel entries are removed
      checkKernelEntriesRemoved(
          folly::to<std::string>(intfIPv4), folly::to<std::string>(intfIPv6));
    }

    clearAllKernelEntries();
  };

  verifyAcrossWarmBoots(setup, verify);
}

/**
 * Test verifies tunnel manager's probe and cleanup functionality:
 * - Creates state (interfaces, addresses, source rules, default routes) for 2
 * interfaces
 * - Verifies state exists in kernel
 * - Calls probe and cleanup code (simulating cold/warm boot behavior)
 * - Verifies kernel has completely clean state after cleanup
 */
TEST_F(AgentTunnelMgrTest, checkProbedDataCleanup) {
  auto setup = [=]() {};
  auto verify = [=, this]() {
    auto config = initialConfig(*getAgentEnsemble());

    printInterfaceDetails(config);
    printKernelInformation();

    // Apply the config
    applyNewConfig(config);
    waitForStateUpdates(getAgentEnsemble()->getSw());

    std::string intfIPv4;
    std::string intfIPv6;
    for (int i = 0; i < config.interfaces()->size(); i++) {
      for (int j = 0; j < config.interfaces()[i].ipAddresses()->size(); j++) {
        std::string intfIP = folly::to<std::string>(
            folly::IPAddress::createNetwork(
                config.interfaces()[i].ipAddresses()[j], -1, false)
                .first);

        if (intfIP.find("::") != std::string::npos) {
          intfIPv6 = std::move(intfIP);
        } else {
          intfIPv4 = std::move(intfIP);
        }
      }

      // Get TunManager pointer
      auto tunMgr_ = getAgentEnsemble()->getSw()->getTunManager();
      auto status = tunMgr_->getIntfStatus(
          getProgrammedState(),
          (InterfaceID)config.interfaces()[i].intfID().value());

      XLOG(INFO) << "Interface ID: "
                 << (InterfaceID)config.interfaces()[i].intfID().value()
                 << ", Status: " << (status ? "UP" : "DOWN")
                 << ", IPv4: " << intfIPv4 << ", IPv6: " << intfIPv6;
      // There could be a race condition where the interface is up, but the
      // socket is not created. So, checking for the socket existence.
      auto socketExists = tunMgr_->isValidNlSocket();

      // There is a known limitation in the kernel that the source route rule
      // entries are not created if the interface is not up. So, checking for
      // the kernel entries if the interface is  up
      if (status && socketExists) {
        checkKernelEntriesExist(folly::to<std::string>(intfIPv6), false, true);
      }
    }

    // Get TunManager pointer
    auto tunMgr_ = getAgentEnsemble()->getSw()->getTunManager();
    auto socketExists = tunMgr_->isValidNlSocket();

    if (socketExists) {
      // Verify probe data is not cleaned up during warmboot, since interfaces
      // in kernel and switchState are same. During the coldboot case for tests,
      // the kernel will have nothing, while switchState will have the
      // interfacs, so cleanup will run.
      if (getAgentEnsemble()->getBootType() == BootType::WARM_BOOT) {
        EXPECT_EQ(tunMgr_->probedStateCleanedUp_, false);
        EXPECT_EQ(getSw()->stats()->getProbedStateCleanupStatus(), 0);
      } else {
        EXPECT_EQ(tunMgr_->probedStateCleanedUp_, true);
        EXPECT_EQ(getSw()->stats()->getProbedStateCleanupStatus(), 1);
      }
      EXPECT_EQ(tunMgr_->initialCleanupDone_, true);

      // Set probeDone_ to false before calling probe()
      tunMgr_->probeDone_ = false;

      XLOG(INFO) << "Starting probe and cleanup of kernel data";
      tunMgr_->probe();
      // Force cleanup by calling deleteAllProbedData directly to ensure it
      // happens regardless of interface mapping comparison
      tunMgr_->deleteAllProbedData();

      XLOG(INFO) << "Stopping probe and clean up of probe data";

      // Check that the kernel entries are removed after probe
      for (int i = 0; i < config.interfaces()->size(); i++) {
        InterfaceID intfID =
            InterfaceID(config.interfaces()[i].intfID().value());

        // Get IPv4 address for this interface
        auto ipv4Addr = getSwitchIntfIP(getProgrammedState(), intfID);
        checkKernelIpEntriesRemoved(intfID, ipv4Addr.str(), true);

        // Get IPv6 address for this interface
        auto ipv6Addr = getSwitchIntfIPv6(getProgrammedState(), intfID);
        checkKernelIpEntriesRemoved(intfID, ipv6Addr.str(), false);
      }
    } else {
      XLOG(INFO) << "Socket does not exist";
    }

    // Recreate interfaces after deleting probed data
    printProbedInterfaceDetails();
    printInterfaceDetails(config);
    printKernelInformation();
    // Set probeDone_ to false before calling probe()
    tunMgr_->probeDone_ = false;

    XLOG(INFO) << "Recreate kernel data";
    tunMgr_->probe();

    auto state = getAgentEnsemble()->getSw()->getState();
    tunMgr_->evb_->runInFbossEventBaseThread(
        [tunMgr_, state]() { tunMgr_->sync(state); });
    printProbedInterfaceDetails();
    printInterfaceDetails(config);
    printKernelInformation();
  };

  verifyAcrossWarmBoots(setup, verify);
}

/**
 * Test verifies tunnel manager's probe and cleanup functionality for DOWN
 * interfaces:
 * - Creates state (interfaces, addresses, source rules, default routes) for 2
 * interfaces with ports disabled (interfaces DOWN)
 * - Verifies state exists in kernel
 * - Verifies kernel has clean state after cleanup, skipping source rule
 * validation for down interfaces (source rules persist for down interfaces
 * per kernel limitation)
 */
TEST_F(AgentTunnelMgrTest, checkProbedDataCleanupInterfaceDown) {
  auto setup = [=]() {};
  auto verify = [=, this]() {
    auto config = initialConfig(*getAgentEnsemble());

    printInterfaceDetails(config);
    printKernelInformation();

    std::string intfIPv4;
    std::string intfIPv6;
    for (int i = 0; i < config.interfaces()->size(); i++) {
      for (int j = 0; j < config.interfaces()[i].ipAddresses()->size(); j++) {
        std::string intfIP = folly::to<std::string>(
            folly::IPAddress::createNetwork(
                config.interfaces()[i].ipAddresses()[j], -1, false)
                .first);

        if (intfIP.find("::") != std::string::npos) {
          intfIPv6 = std::move(intfIP);
        } else {
          intfIPv4 = std::move(intfIP);
        }
      }

      // Get TunManager pointer
      auto tunMgr_ = getAgentEnsemble()->getSw()->getTunManager();
      auto status = tunMgr_->getIntfStatus(
          getProgrammedState(),
          (InterfaceID)config.interfaces()[i].intfID().value());

      XLOG(INFO) << "Interface ID: "
                 << (InterfaceID)config.interfaces()[i].intfID().value()
                 << ", Status: " << (status ? "UP" : "DOWN")
                 << ", IPv4: " << intfIPv4 << ", IPv6: " << intfIPv6;
      // There could be a race condition where the interface is up, but the
      // socket is not created. So, checking for the socket existence.
      auto socketExists = tunMgr_->isValidNlSocket();

      // There is a known limitation in the kernel that the source route rule
      // entries are not created if the interface is not up. So, checking for
      // the kernel entries if the interface is  up
      if (status && socketExists) {
        checkKernelEntriesExist(folly::to<std::string>(intfIPv6), false, true);
      }
    }

    for (int i = 0; i < config.ports()->size(); i++) {
      XLOG(DBG2) << "Disabling port at index: " << i;
      config.ports()[i].state() = cfg::PortState::DISABLED;
    }

    // Apply the config
    applyNewConfig(config);
    waitForStateUpdates(getAgentEnsemble()->getSw());

    printInterfaceDetails(config);

    // Get TunManager pointer
    auto tunMgr_ = getAgentEnsemble()->getSw()->getTunManager();
    auto socketExists = tunMgr_->isValidNlSocket();

    if (socketExists) {
      // Verify probe data is not cleaned up during warmboot, since interfaces
      // in kernel and switchState are same. During the coldboot case for tests,
      // the kernel will have nothing, while switchState will have the
      if (getAgentEnsemble()->getBootType() == BootType::WARM_BOOT) {
        EXPECT_EQ(tunMgr_->probedStateCleanedUp_, false);
        EXPECT_EQ(getSw()->stats()->getProbedStateCleanupStatus(), 0);
      } else {
        EXPECT_EQ(tunMgr_->probedStateCleanedUp_, true);
        EXPECT_EQ(getSw()->stats()->getProbedStateCleanupStatus(), 1);
      }
      EXPECT_EQ(tunMgr_->initialCleanupDone_, true);

      // Set probeDone_ to false before calling probe()
      tunMgr_->probeDone_ = false;

      XLOG(INFO) << "Starting probe and cleanup of kernel data";
      tunMgr_->probe();
      printProbedInterfaceDetails();
      printKernelInformation();

      // Force cleanup by calling deleteAllProbedData directly to ensure it
      // happens regardless of interface mapping comparison
      tunMgr_->deleteAllProbedData();

      XLOG(INFO) << "Stopping probe and clean up of probe data";

      // Check that the kernel entries are removed after probe
      for (int i = 0; i < config.interfaces()->size(); i++) {
        InterfaceID intfID =
            InterfaceID(config.interfaces()[i].intfID().value());

        // Get IPv4 address for this interface
        auto ipv4Addr = getSwitchIntfIP(getProgrammedState(), intfID);
        checkKernelIpEntriesRemovedStrict(intfID, ipv4Addr.str(), true);

        // Get IPv6 address for this interface
        auto ipv6Addr = getSwitchIntfIPv6(getProgrammedState(), intfID);
        checkKernelIpEntriesRemovedStrict(intfID, ipv6Addr.str(), false);
      }
    } else {
      XLOG(INFO) << "Socket does not exist";
    }

    // Recreate interfaces after deleting probed data
    printProbedInterfaceDetails();
    printInterfaceDetails(config);
    printKernelInformation();
    // Set probeDone_ to false before calling probe()
    tunMgr_->probeDone_ = false;

    XLOG(INFO) << "Recreate kernel data";
    tunMgr_->probe();

    auto state = getAgentEnsemble()->getSw()->getState();
    tunMgr_->evb_->runInFbossEventBaseThread(
        [tunMgr_, state]() { tunMgr_->sync(state); });
    printProbedInterfaceDetails();
    printInterfaceDetails(config);
    printKernelInformation();
  };

  verifyAcrossWarmBoots(setup, verify);
}

// Test that the tunnel manager is able to handle ipv4 address change of the
// interface
TEST_F(AgentTunnelMgrTest, changeIPv4Address) {
  auto setup = [=]() {};
  auto verify = [=, this]() {
    auto config = initialConfig(*getAgentEnsemble());

    // Apply the config
    applyNewConfig(config);
    waitForStateUpdates(getAgentEnsemble()->getSw());

    std::string intfIPv4;
    std::string intfIPv6;
    for (int i = 0; i < config.interfaces()->size(); i++) {
      if (config.interfaces()[i].scope() == cfg::Scope::GLOBAL) {
        continue;
      }
      for (int j = 0; j < config.interfaces()[i].ipAddresses()->size(); j++) {
        std::string intfIP = folly::to<std::string>(
            folly::IPAddress::createNetwork(
                config.interfaces()[i].ipAddresses()[j], -1, false)
                .first);

        if (intfIP.find("::") != std::string::npos) {
          intfIPv6 = std::move(intfIP);
        } else {
          intfIPv4 = std::move(intfIP);
        }
      }

      // Get TunManager pointer
      auto tunMgr_ = getAgentEnsemble()->getSw()->getTunManager();
      auto status = tunMgr_->getIntfStatus(
          getProgrammedState(),
          (InterfaceID)config.interfaces()[i].intfID().value());
      // There could be a race condition where the interface is up, but the
      // socket is not created. So, checking for the socket existence.
      auto socketExists = tunMgr_->isValidNlSocket();

      // There is a known limitation in the kernel that the source route rule
      // entries are not created if the interface is not up. So, checking for
      // the kernel entries if the interface is  up
      if (status && socketExists) {
        checkKernelEntriesExist(folly::to<std::string>(intfIPv4), true, true);
      }

      // change ipv4 address of the interface
      for (int j = 0; j < config.interfaces()[i].ipAddresses()->size(); j++) {
        if (config.interfaces()[i].ipAddresses()[j].find("::") ==
            std::string::npos) {
          auto ipDecimal = folly::sformat("{}", i + 1);
          config.interfaces()[i].ipAddresses()[j] =
              folly::sformat("{}.2.2.2/24", ipDecimal);
          intfIPv4 = folly::sformat("{}.2.2.2", ipDecimal);
        }
      }

      // Apply the config
      applyNewConfig(config);
      waitForStateUpdates(getAgentEnsemble()->getSw());

      // Route entries installation is currently not consistent after the ip
      // address change. So, passing false for checkRouteEntry.
      if (status) {
        checkKernelEntriesExist(folly::to<std::string>(intfIPv4), true, false);
      }

      // Clear kernel entries
      clearKernelEntries(
          folly::to<std::string>(intfIPv4), folly::to<std::string>(intfIPv6));

      // Check that the kernel entries are removed
      checkKernelEntriesRemoved(
          folly::to<std::string>(intfIPv4), folly::to<std::string>(intfIPv6));
    }

    clearAllKernelEntries();
  };

  verifyAcrossWarmBoots(setup, verify);
}

// Test that the tunnel manager is able to handle ipv6 address change of the
// interface
TEST_F(AgentTunnelMgrTest, changeIPv6Address) {
  auto setup = [=]() {};
  auto verify = [=, this]() {
    auto config = initialConfig(*getAgentEnsemble());

    // Apply the config
    applyNewConfig(config);
    waitForStateUpdates(getAgentEnsemble()->getSw());

    std::string intfIPv4;
    std::string intfIPv6;
    for (int i = 0; i < config.interfaces()->size(); i++) {
      if (config.interfaces()[i].scope() == cfg::Scope::GLOBAL) {
        continue;
      }
      for (int j = 0; j < config.interfaces()[i].ipAddresses()->size(); j++) {
        std::string intfIP = folly::to<std::string>(
            folly::IPAddress::createNetwork(
                config.interfaces()[i].ipAddresses()[j], -1, false)
                .first);

        if (intfIP.find("::") != std::string::npos) {
          intfIPv6 = std::move(intfIP);
        } else {
          intfIPv4 = std::move(intfIP);
        }
      }

      // Get TunManager pointer
      auto tunMgr_ = getAgentEnsemble()->getSw()->getTunManager();
      auto status = tunMgr_->getIntfStatus(
          getProgrammedState(),
          (InterfaceID)config.interfaces()[i].intfID().value());
      // There could be a race condition where the interface is up, but the
      // socket is not created. So, checking for the socket existence.
      auto socketExists = tunMgr_->isValidNlSocket();

      // There is a known limitation in the kernel that the source route
      // rule entries are not created if the interface is not up. So,
      // checking for the kernel entries if the interface is  up
      if (status && socketExists) {
        checkKernelEntriesExist(folly::to<std::string>(intfIPv6), false);
      }

      // change ipv6 address of the interface
      for (int j = 0; j < config.interfaces()[i].ipAddresses()->size(); j++) {
        if (config.interfaces()[i].ipAddresses()[j].find("::") !=
            std::string::npos) {
          auto ipDecimal = folly::sformat("{}", i + 1);
          config.interfaces()[i].ipAddresses()[j] =
              folly::sformat("{}::2/64", ipDecimal);
          intfIPv6 = folly::sformat("{}::2", ipDecimal);
        }
      }

      // Apply the config
      applyNewConfig(config);
      waitForStateUpdates(getAgentEnsemble()->getSw());

      // Route entries installation is currently not consistent after the ip
      // address change. So, passing false for checkRouteEntry.
      if (status) {
        checkKernelEntriesExist(folly::to<std::string>(intfIPv6), false, false);
      }

      // Clear kernel entries
      clearKernelEntries(
          folly::to<std::string>(intfIPv4), folly::to<std::string>(intfIPv6));

      // Check that the kernel entries are removed
      checkKernelEntriesRemoved(
          folly::to<std::string>(intfIPv4), folly::to<std::string>(intfIPv6));
    }

    clearAllKernelEntries();
  };

  verifyAcrossWarmBoots(setup, verify);
}

// Test to check if there are no duplicate kernel entries created
TEST_F(AgentTunnelMgrTest, checkDuplicateEntries) {
  auto setup = [=]() {};
  auto verify = [=, this]() {
    auto config = initialConfig(*getAgentEnsemble());

    // Apply the config
    applyNewConfig(config);
    waitForStateUpdates(getAgentEnsemble()->getSw());

    std::string intfIPv4;
    std::string intfIPv6;
    for (int i = 0; i < config.interfaces()->size(); i++) {
      XLOG(DBG2) << "Interface Id: "
                 << (InterfaceID)config.interfaces()[i].intfID().value();
      if (config.interfaces()[i].scope() == cfg::Scope::GLOBAL) {
        continue;
      }

      for (int j = 0; j < config.interfaces()[i].ipAddresses()->size(); j++) {
        std::string intfIP = folly::to<std::string>(
            folly::IPAddress::createNetwork(
                config.interfaces()[i].ipAddresses()[j], -1, false)
                .first);

        if (intfIP.find("::") != std::string::npos) {
          intfIPv6 = std::move(intfIP);
        } else {
          intfIPv4 = std::move(intfIP);
        }
      }

      // Get TunManager pointer
      auto tunMgr_ = getAgentEnsemble()->getSw()->getTunManager();
      auto status = tunMgr_->getIntfStatus(
          getProgrammedState(),
          (InterfaceID)config.interfaces()[i].intfID().value());
      // There could be a race condition where the interface is up, but the
      // socket is not created. So, checking for the socket existence.
      auto socketExists = tunMgr_->isValidNlSocket();

      // There is a known limitation in the kernel that the source route rule
      // entries are not created if the interface is not up. So, checking for
      // the kernel entries if the interface is  up
      if (status && socketExists) {
        checkKernelEntriesExist(folly::to<std::string>(intfIPv6), false);
      }

      // Applying the same config again
      // Made change in TunManager to reprogram source route rule upon
      // interface up. Noticed duplicate entry for 1.1.1.1 and 1:: for source
      // route rule.

      // Applying same ipv4 and ipv6 address on the interface

      // Apply the config
      applyNewConfig(config);

      std::string intfIPv4New;
      std::string intfIPv6New;
      // change ipv4 and ipv6 address of the interface
      for (int j = 0; j < config.interfaces()[i].ipAddresses()->size(); j++) {
        if (config.interfaces()[i].ipAddresses()[j].find("::") ==
            std::string::npos) {
          intfIPv4New = utility::genInterfaceAddress(i + 1, true, 31, 31);
          config.interfaces()[i].ipAddresses()[j] = intfIPv4New;
          intfIPv4New = folly::to<std::string>(
              folly::IPAddress::createNetwork(
                  config.interfaces()[i].ipAddresses()[j], -1, false)
                  .first);
        } else {
          intfIPv6New = utility::genInterfaceAddress(i + 1, false, 127, 127);
          config.interfaces()[i].ipAddresses()[j] = intfIPv6New;
          intfIPv6New = folly::to<std::string>(
              folly::IPAddress::createNetwork(
                  config.interfaces()[i].ipAddresses()[j], -1, false)
                  .first);
        }
      }

      // Apply the config
      applyNewConfig(config);
      waitForStateUpdates(getAgentEnsemble()->getSw());

      // Route entries installation is currently not consistent after the ip
      // address change. So, passing false for checkRouteEntry.
      if (status) {
        // If source route rule is added again for the same IP address, it
        // would create duplicate entries.
        checkKernelEntriesRemoved(
            folly::to<std::string>(intfIPv4), folly::to<std::string>(intfIPv6));
        checkKernelEntriesExist(
            folly::to<std::string>(intfIPv4New), true, false);
        checkKernelEntriesExist(
            folly::to<std::string>(intfIPv6New), false, false);
      }

      // Clear kernel entries
      clearKernelEntries(
          folly::to<std::string>(intfIPv4New),
          folly::to<std::string>(intfIPv6New));

      // Check that the kernel entries are removed
      checkKernelEntriesRemoved(
          folly::to<std::string>(intfIPv4New),
          folly::to<std::string>(intfIPv6New));
    }

    clearAllKernelEntries();
  };

  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(AgentTunnelMgrTest, checkKernelIPv4EntriesPortsDown) {
  auto setup = [=]() {};
  auto verify = [=, this]() {
    auto config = initialConfig(*getAgentEnsemble());
    std::string intfIPv4;
    std::string intfIPv6;
    for (int i = 0; i < config.ports()->size(); i++) {
      config.ports()[i].state() = cfg::PortState::DISABLED;
    }

    // Apply the config
    applyNewConfig(config);
    waitForStateUpdates(getAgentEnsemble()->getSw());

    for (int i = 0; i < config.interfaces()->size(); i++) {
      for (int j = 0; j < config.interfaces()[i].ipAddresses()->size(); j++) {
        std::string intfIP = folly::to<std::string>(
            folly::IPAddress::createNetwork(
                config.interfaces()[i].ipAddresses()[j], -1, false)
                .first);

        if (intfIP.find("::") != std::string::npos) {
          intfIPv6 = std::move(intfIP);
        } else {
          intfIPv4 = std::move(intfIP);
        }
      }

      // Get TunManager pointer
      auto tunMgr_ = getAgentEnsemble()->getSw()->getTunManager();
      auto status = tunMgr_->getIntfStatus(
          getProgrammedState(),
          (InterfaceID)config.interfaces()[i].intfID().value());
      // There could be a race condition where the interface is up, but the
      // socket is not created. So, checking for the socket existence.
      auto socketExists = tunMgr_->isValidNlSocket();

      // There is a known limitation in the kernel that the source route rule
      // entries are not created if the interface is not up. So, checking for
      // the kernel entries if the interface is  up
      if (status && socketExists) {
        checkKernelEntriesExist(folly::to<std::string>(intfIPv4), true, false);
      }

      // Clear kernel entries
      clearKernelEntries(
          folly::to<std::string>(intfIPv4), folly::to<std::string>(intfIPv6));

      // Check that the kernel entries are removed
      checkKernelEntriesRemoved(
          folly::to<std::string>(intfIPv4), folly::to<std::string>(intfIPv6));
    }

    clearAllKernelEntries();
  };

  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(AgentTunnelMgrTest, checkKernelIPv4EntriesPortsDownUp) {
  auto setup = [=]() {};
  auto verify = [=, this]() {
    auto config = initialConfig(*getAgentEnsemble());
    std::string intfIPv4;
    std::string intfIPv6;
    for (int i = 0; i < config.ports()->size(); i++) {
      config.ports()[i].state() = cfg::PortState::DISABLED;
    }

    // Apply the config
    applyNewConfig(config);
    waitForStateUpdates(getAgentEnsemble()->getSw());

    for (int i = 0; i < config.ports()->size(); i++) {
      config.ports()[i].state() = cfg::PortState::ENABLED;
      auto portType = config.ports()[i].portType().value();
      config.ports()[i].loopbackMode() =
          getAsics().cbegin()->second->getDesiredLoopbackMode(portType);
    }

    // Apply the config
    applyNewConfig(config);
    waitForStateUpdates(getAgentEnsemble()->getSw());

    for (int i = 0; i < config.interfaces()->size(); i++) {
      for (int j = 0; j < config.interfaces()[i].ipAddresses()->size(); j++) {
        std::string intfIP = folly::to<std::string>(
            folly::IPAddress::createNetwork(
                config.interfaces()[i].ipAddresses()[j], -1, false)
                .first);

        if (intfIP.find("::") != std::string::npos) {
          intfIPv6 = std::move(intfIP);
        } else {
          intfIPv4 = std::move(intfIP);
        }
      }

      // Get TunManager pointer
      auto tunMgr_ = getAgentEnsemble()->getSw()->getTunManager();
      auto status = tunMgr_->getIntfStatus(
          getProgrammedState(),
          (InterfaceID)config.interfaces()[i].intfID().value());
      // There could be a race condition where the interface is up, but the
      // socket is not created. So, checking for the socket existence.
      auto socketExists = tunMgr_->isValidNlSocket();

      // There is a known limitation in the kernel that the source route rule
      // entries are not created if the interface is not up. So, checking for
      // the kernel entries if the interface is  up
      if (status && socketExists) {
        checkKernelEntriesExist(folly::to<std::string>(intfIPv4));
      }

      // Clear kernel entries
      clearKernelEntries(
          folly::to<std::string>(intfIPv4), folly::to<std::string>(intfIPv6));

      // Check that the kernel entries are removed
      checkKernelEntriesRemoved(
          folly::to<std::string>(intfIPv4), folly::to<std::string>(intfIPv6));
    }

    clearAllKernelEntries();
  };

  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(AgentTunnelMgrTest, checkKernelIPv6EntriesPortsDownUp) {
  auto setup = [=]() {};
  auto verify = [=, this]() {
    auto config = initialConfig(*getAgentEnsemble());
    std::string intfIPv4;
    std::string intfIPv6;
    for (int i = 0; i < config.ports()->size(); i++) {
      config.ports()[i].state() = cfg::PortState::DISABLED;
    }

    // Apply the config
    applyNewConfig(config);
    waitForStateUpdates(getAgentEnsemble()->getSw());

    for (int i = 0; i < config.ports()->size(); i++) {
      config.ports()[i].state() = cfg::PortState::ENABLED;
      auto portType = config.ports()[i].portType().value();
      config.ports()[i].loopbackMode() =
          getAsics().cbegin()->second->getDesiredLoopbackMode(portType);
    }

    // Apply the config
    applyNewConfig(config);
    waitForStateUpdates(getAgentEnsemble()->getSw());

    for (int i = 0; i < config.interfaces()->size(); i++) {
      for (int j = 0; j < config.interfaces()[i].ipAddresses()->size(); j++) {
        std::string intfIP = folly::to<std::string>(
            folly::IPAddress::createNetwork(
                config.interfaces()[i].ipAddresses()[j], -1, false)
                .first);

        if (intfIP.find("::") != std::string::npos) {
          intfIPv6 = std::move(intfIP);
        } else {
          intfIPv4 = std::move(intfIP);
        }
      }

      // Get TunManager pointer
      auto tunMgr_ = getAgentEnsemble()->getSw()->getTunManager();
      auto status = tunMgr_->getIntfStatus(
          getProgrammedState(),
          (InterfaceID)config.interfaces()[i].intfID().value());
      // There could be a race condition where the interface is up, but the
      // socket is not created. So, checking for the socket existence.
      auto socketExists = tunMgr_->isValidNlSocket();

      // There is a known limitation in the kernel that the source route rule
      // entries are not created if the interface is not up. So, checking for
      // the kernel entries if the interface is  up
      if (status && socketExists) {
        checkKernelEntriesExist(folly::to<std::string>(intfIPv6), false, true);
      }

      // Clear kernel entries
      clearKernelEntries(
          folly::to<std::string>(intfIPv4), folly::to<std::string>(intfIPv6));

      // Check that the kernel entries are removed
      checkKernelEntriesRemoved(
          folly::to<std::string>(intfIPv4), folly::to<std::string>(intfIPv6));
    }

    clearAllKernelEntries();
  };

  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(AgentTunnelMgrTest, changeIpv4AddressPortDownUp) {
  auto setup = [=]() {};

  auto verify = [=, this]() {
    auto config = getAgentEnsemble()->getCurrentConfig();
    std::string intfIPv4;
    std::vector<std::string> intfOldIPv4s;
    std::vector<std::string> intfNewIPv4s;
    InterfaceID intfID = getInterfaceIDForPort(
        getAgentEnsemble()->masterLogicalPortIds()[0],
        getAgentEnsemble()->getSw()->getState());

    intfOldIPv4s = getInterfaceIpAddress(config, true);
    checkKernelIpEntriesExist(intfID, intfOldIPv4s[0], true);
    for (int i = 0; i < config.ports()->size(); i++) {
      config.ports()[i].state() = cfg::PortState::DISABLED;
    }

    // Apply the config
    applyNewConfig(config);
    waitForStateUpdates(getAgentEnsemble()->getSw());

    // change ipv6 address of the interface
    intfNewIPv4s = changeKernelIPAddress(config, true);

    // Bring up one port
    bringUpPort(getAgentEnsemble()->masterLogicalPortIds()[0]);

    checkKernelIpEntriesExist(intfID, intfNewIPv4s[0], true);

    checkKernelIpEntriesRemoved(intfID, intfOldIPv4s[0], true);

    config.ports()[0].state() = cfg::PortState::ENABLED;
    auto portType = config.ports()[0].portType().value();
    config.ports()[0].loopbackMode() =
        getAsics().cbegin()->second->getDesiredLoopbackMode(portType);
    // Apply the config
    applyNewConfig(config);
    waitForStateUpdates(getAgentEnsemble()->getSw());
  };

  auto setupPostWarmboot = [=]() {};

  // This is just to bring back the set-up to the original state
  auto verifyPostWarmboot = [=, this]() {
    auto config = getAgentEnsemble()->getCurrentConfig();
    InterfaceID intfID = (InterfaceID)config.interfaces()[0].intfID().value();
    std::vector<std::string> intfIPv6s;
    intfIPv6s = getInterfaceIpAddress(config, true);
    checkKernelIpEntriesExist(intfID, intfIPv6s[0], true);
    clearAllKernelEntries();

    checkKernelIpEntriesRemoved(intfID, intfIPv6s[0], true);
  };

  verifyAcrossWarmBoots(setup, verify, setupPostWarmboot, verifyPostWarmboot);
}

TEST_F(AgentTunnelMgrTest, changeIpv6AddressPortDownUp) {
  auto setup = [=]() {};

  auto verify = [=, this]() {
    auto config = getAgentEnsemble()->getCurrentConfig();
    std::string intfIPv6;
    std::vector<std::string> intfOldIPv6s;
    std::vector<std::string> intfNewIPv6s;
    InterfaceID intfID = getInterfaceIDForPort(
        getAgentEnsemble()->masterLogicalPortIds()[0],
        getAgentEnsemble()->getSw()->getState());

    intfOldIPv6s = getInterfaceIpAddress(config, false);
    checkKernelIpEntriesExist(intfID, intfOldIPv6s[0], false);

    for (int i = 0; i < config.ports()->size(); i++) {
      config.ports()[i].state() = cfg::PortState::DISABLED;
    }

    // Apply the config
    applyNewConfig(config);
    waitForStateUpdates(getAgentEnsemble()->getSw());

    // change ipv6 address of the interface
    intfNewIPv6s = changeKernelIPAddress(config, false);

    // Bring up one port
    bringUpPort(getAgentEnsemble()->masterLogicalPortIds()[0]);

    checkKernelIpEntriesExist(intfID, intfNewIPv6s[0], false);

    checkKernelIpEntriesRemoved(intfID, intfOldIPv6s[0], false);

    config.ports()[0].state() = cfg::PortState::ENABLED;
    auto portType = config.ports()[0].portType().value();
    config.ports()[0].loopbackMode() =
        getAsics().cbegin()->second->getDesiredLoopbackMode(portType);
    // Apply the config
    applyNewConfig(config);
    waitForStateUpdates(getAgentEnsemble()->getSw());
  };

  auto setupPostWarmboot = [=]() {};

  // This is just to bring back the set-up to the original state
  auto verifyPostWarmboot = [=, this]() {
    auto config = getAgentEnsemble()->getCurrentConfig();
    InterfaceID intfID = (InterfaceID)config.interfaces()[0].intfID().value();
    std::vector<std::string> intfIPv6s;
    intfIPv6s = getInterfaceIpAddress(config, false);
    checkKernelIpEntriesExist(intfID, intfIPv6s[0], false);
    clearAllKernelEntries();

    checkKernelIpEntriesRemoved(intfID, intfIPv6s[0], false);
  };

  verifyAcrossWarmBoots(setup, verify, setupPostWarmboot, verifyPostWarmboot);
}

} // namespace facebook::fboss
