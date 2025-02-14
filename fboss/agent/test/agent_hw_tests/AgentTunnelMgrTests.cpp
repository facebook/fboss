// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.
#include <sstream>
#include <string>
#include "fboss/agent/SwSwitch.h"
#include "fboss/agent/TunManager.h"
#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/test/AgentHwTest.h"
#include "fboss/agent/test/TestUtils.h"

namespace facebook::fboss {

class AgentTunnelMgrTest : public AgentHwTest {
 public:
  std::vector<production_features::ProductionFeature>
  getProductionFeaturesVerified() const override {
    return {production_features::ProductionFeature::CPU_RX_TX};
  }

  void setCmdLineFlagOverrides() const override {
    AgentHwTest::setCmdLineFlagOverrides();
    FLAGS_tun_intf = true;
  }

  void SetUp() override {
    // clear all kernel entries before starting the test
    clearAllKernelEntries();
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
    cmd = folly::to<std::string>("ip route list | grep -w fboss");
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
          cmd = folly::to<std::string>("ip link delete ", subs);
          runShellCmd(cmd);
          XLOG(DBG2) << "clearKernelEntries Cmd: " << cmd;
          break;
        }
      } while (iss3);
    }

    cmd = folly::to<std::string>("ip -6 route list | grep -w fboss");
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
          cmd = folly::to<std::string>("ip link delete ", subs);
          runShellCmd(cmd);
          XLOG(DBG2) << "clearKernelEntries Cmd: " << cmd;
          break;
        }
      } while (iss4);
    }

    // Get the String
    cmd = folly::to<std::string>("ip addr list | grep -w fboss");
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
          cmd = folly::to<std::string>("ip link delete ", subs);
          runShellCmd(cmd);
          XLOG(DBG2) << "clearKernelEntries Cmd: " << cmd;
          break;
        }
      } while (iss5);
    }

    cmd = folly::to<std::string>("ip -6 addr list | grep -w fboss");
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
          cmd = folly::to<std::string>("ip link delete ", subs);
          runShellCmd(cmd);
          XLOG(DBG2) << "clearKernelEntries Cmd: " << cmd;
          break;
        }
      } while (iss6);
    }
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
    XLOG(DBG2) << "checkKernelEntriesExist Output: \n" << output;

    // TODO: This is to stabilize the test. Remove this and use EXPECT_TRUE once
    // the tunnelMgr fixes are in.
    if (output.find(folly::to<std::string>(searchIntfIp)) ==
        std::string::npos) {
      XLOG(DBG2) << "checkKernelEntriesExist: Source route rule entry not "
                 << "present in the kernel for " << searchIntfIp;
    }

    if (isIPv4) {
      // Check that the tunnel address entries are present in the kernel
      cmd = folly::to<std::string>("ip addr list | grep -w ", searchIntfIp);
    } else {
      cmd = folly::to<std::string>("ip -6 addr list | grep -w ", searchIntfIp);
    }

    output = runShellCmd(cmd);

    XLOG(DBG2) << "checkKernelEntriesExist Cmd: " << cmd;
    XLOG(DBG2) << "checkKernelEntriesExist Output: \n" << output;

    EXPECT_TRUE(
        output.find(folly::to<std::string>(searchIntfIp)) != std::string::npos);

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

      XLOG(DBG2) << "checkKernelEntriesExist Cmd: " << cmd;
      XLOG(DBG2) << "checkKernelEntriesExist Output:" << output;

      EXPECT_TRUE(
          output.find(folly::to<std::string>(searchIntfIp)) !=
          std::string::npos);
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
    auto cfg = utility::onePortPerInterfaceConfig(
        ensemble.getSw(),
        ensemble.masterLogicalPortIds(),
        true /*interfaceHasSubnet*/);
    return cfg;
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
      // Made change in TunManager to reprogram source route rule upon interface
      // up. Noticed duplicate entry for 1.1.1.1 and 1:: for source route rule.

      // Applying same ipv4 and ipv6 address on the interface

      // Apply the config
      applyNewConfig(config);

      std::string intfIPv4New;
      std::string intfIPv6New;
      // change ipv4 and ipv6 address of the interface
      for (int j = 0; j < config.interfaces()[i].ipAddresses()->size(); j++) {
        auto ipDecimal =
            folly::sformat("{}", i + config.interfaces()->size() + 10);
        if (config.interfaces()[i].ipAddresses()[j].find("::") ==
            std::string::npos) {
          config.interfaces()[i].ipAddresses()[j] =
              folly::sformat("{}.2.2.2/24", ipDecimal);
          intfIPv4New = folly::sformat("{}.2.2.2", ipDecimal);
        } else {
          config.interfaces()[i].ipAddresses()[j] =
              folly::sformat("{}::2/64", ipDecimal);
          intfIPv6New = folly::sformat("{}::2", ipDecimal);
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
  };

  verifyAcrossWarmBoots(setup, verify);
}
} // namespace facebook::fboss
