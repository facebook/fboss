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
    return {};
  }

  void setCmdLineFlagOverrides() const override {
    AgentHwTest::setCmdLineFlagOverrides();
    FLAGS_tun_intf = true;
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
      cmd = folly::to<std::string>("ip addr list | grep -w ", intfIp);
    } else {
      cmd = folly::to<std::string>("ip -6 addr list | grep -w ", intfIp);
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
  }

  void checkIpKernelEntriesRemoved(
      const std::string& intfIp,
      bool isIPv4 = true) {
    // Check that the source route rule entries are not present in the kernel
    std::string cmd;
    // ipv6 address can match with other ipv6 addresses e.g. 1:: can match with
    // 1::1. So, adding a space before and after the address to avoid matching
    // with other addresses
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

    EXPECT_TRUE(
        output.find(folly::to<std::string>(searchIntfIp)) == std::string::npos);

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

    EXPECT_TRUE(
        output.find(folly::to<std::string>(searchIntfIp)) != std::string::npos);

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
    auto cfg = utility::oneL3IntfConfig(
        ensemble.getSw()->getPlatformMapping(),
        ensemble.getL3Asics(),
        ensemble.masterLogicalPortIds()[0],
        ensemble.getSw()->getPlatformSupportsAddRemovePort());
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

    auto intfIPv4 = folly::IPAddress::createNetwork(
                        config.interfaces()[0].ipAddresses()[0], -1, false)
                        .first;

    auto intfIPv6 = folly::IPAddress::createNetwork(
                        config.interfaces()[0].ipAddresses()[1], -1, false)
                        .first;

    // Get TunManager pointer
    auto tunMgr_ = getAgentEnsemble()->getSw()->getTunManager();
    auto status = tunMgr_->getIntfStatus(
        getProgrammedState(), (InterfaceID)config.interfaces()[0].get_intfID());
    // There could be a race condition where the interface is up, but the
    // socket is not created. So, checking for the socket existence.
    auto socketExists = tunMgr_->isValidNlSocket();

    // There is a known limitation in the kernel that the source route rule
    // entries are not created if the interface is not up. So, checking for
    // the kernel entries if the interface is  up
    if (status && socketExists) {
      checkKernelEntriesExist(folly::to<std::string>(intfIPv6), false);
    }

    // change ipv6 address of the interface
    config.interfaces()[0].ipAddresses()[1] = "2::/128";

    // Apply the config
    applyNewConfig(config);
    waitForStateUpdates(getAgentEnsemble()->getSw());

    intfIPv6 = folly::IPAddress::createNetwork(
                   config.interfaces()[0].ipAddresses()[1], -1, false)
                   .first;

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

    auto intfIPv4 = folly::IPAddress::createNetwork(
                        config.interfaces()[0].ipAddresses()[0], -1, false)
                        .first;

    auto intfIPv6 = folly::IPAddress::createNetwork(
                        config.interfaces()[0].ipAddresses()[1], -1, false)
                        .first;

    // Get TunManager pointer
    auto tunMgr_ = getAgentEnsemble()->getSw()->getTunManager();
    auto status = tunMgr_->getIntfStatus(
        getProgrammedState(), (InterfaceID)config.interfaces()[0].get_intfID());
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
    config.interfaces()[0].ipAddresses()[0] = "1.1.1.1/32";
    config.interfaces()[0].ipAddresses()[1] = "1::/128";

    // Apply the config
    applyNewConfig(config);

    // change ipv4 and ipv6 address of the interface
    config.interfaces()[0].ipAddresses()[0] = "2.2.2.2/32";
    config.interfaces()[0].ipAddresses()[1] = "2::/128";

    // Apply the config
    applyNewConfig(config);
    waitForStateUpdates(getAgentEnsemble()->getSw());

    auto intfIPv6New = folly::IPAddress::createNetwork(
                           config.interfaces()[0].ipAddresses()[1], -1, false)
                           .first;
    auto intfIPv4New = folly::IPAddress::createNetwork(
                           config.interfaces()[0].ipAddresses()[0], -1, false)
                           .first;

    // Route entries installation is currently not consistent after the ip
    // address change. So, passing false for checkRouteEntry.
    if (status) {
      // If source route rule is added again for the same IP address, it would
      // create duplicate entries.
      checkKernelEntriesRemoved(
          folly::to<std::string>(intfIPv4), folly::to<std::string>(intfIPv6));
      checkKernelEntriesExist(folly::to<std::string>(intfIPv4New), true, false);
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
  };

  verifyAcrossWarmBoots(setup, verify);
}
} // namespace facebook::fboss
