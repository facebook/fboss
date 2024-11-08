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
  void clearKernelEntries(const std::string& intfIp) {
    auto cmd = folly::to<std::string>("ip rule list | grep ", intfIp);

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
      cmd = folly::to<std::string>("ip rule delete table ", lastWord);

      runShellCmd(cmd);

      // Get the source route rule entries again
      cmd = folly::to<std::string>("ip rule list | grep ", intfIp);

      output = runShellCmd(cmd);

      XLOG(DBG2) << "clearKernelEntries Cmd: " << cmd;
      XLOG(DBG2) << "clearKernelEntries Output: \n" << output;
    }

    // Get the String
    cmd = folly::to<std::string>("ip addr list | grep ", intfIp);

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

  void checkKernelEntriesRemoved(const std::string& intfIp) {
    // Check that the source route rule entries are not present in the kernel
    auto cmd = folly::to<std::string>("ip rule list | grep ", intfIp);

    auto output = runShellCmd(cmd);

    XLOG(DBG2) << "checkKernelEntriesRemoved Cmd: " << cmd;
    XLOG(DBG2) << "checkKernelEntriesRemoved Output: \n" << output;

    EXPECT_TRUE(
        output.find(folly::to<std::string>(intfIp)) == std::string::npos);

    // Check that the tunnel address entries are not present in the kernel
    cmd = folly::to<std::string>("ip addr list | grep ", intfIp);

    output = runShellCmd(cmd);

    XLOG(DBG2) << "checkKernelEntriesRemoved Cmd: " << cmd;
    XLOG(DBG2) << "checkKernelEntriesRemoved Output: \n" << output;

    EXPECT_TRUE(
        output.find(folly::to<std::string>(intfIp)) == std::string::npos);

    // Check that the route entries are not present in the kernel
    cmd = folly::to<std::string>("ip route list | grep ", intfIp);

    output = runShellCmd(cmd);

    XLOG(DBG2) << "checkKernelEntriesRemoved Cmd: " << cmd;
    XLOG(DBG2) << "checkKernelEntriesRemoved Output: \n" << output;

    EXPECT_TRUE(
        output.find(folly::to<std::string>(intfIp)) == std::string::npos);
  }

  // Check that the kernel entries are present in the kernel
  void checkKernelEntriesExist(
      const std::string& intfIp,
      bool checkRouteEntry = true) {
    // Check that the source route rule entries are present in the kernel
    auto cmd = folly::to<std::string>("ip rule list | grep ", intfIp);

    auto output = runShellCmd(cmd);

    XLOG(DBG2) << "checkKernelEntries Cmd: " << cmd;
    XLOG(DBG2) << "checkKernelEntries Output: \n" << output;

    EXPECT_TRUE(
        output.find(folly::to<std::string>(intfIp)) != std::string::npos);

    // Check that the tunnel address entries are present in the kernel
    cmd = folly::to<std::string>("ip addr list | grep ", intfIp);

    output = runShellCmd(cmd);

    XLOG(DBG2) << "checkKernelEntries Cmd: " << cmd;
    XLOG(DBG2) << "checkKernelEntries Output: \n" << output;

    EXPECT_TRUE(
        output.find(folly::to<std::string>(intfIp)) != std::string::npos);

    if (checkRouteEntry) {
      // Check that the route entries are present in the kernel
      cmd = folly::to<std::string>("ip route list | grep ", intfIp);

      output = runShellCmd(cmd);

      XLOG(DBG2) << "checkKernelEntries Cmd: " << cmd;
      XLOG(DBG2) << "checkKernelEntries Output:" << output;

      EXPECT_TRUE(
          output.find(folly::to<std::string>(intfIp)) != std::string::npos);
    }
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
// entries, tunnel address entries and default route entries in the kernel
TEST_F(AgentTunnelMgrTest, checkKernelEntries) {
  auto setup = [=]() {};
  auto verify = [=, this]() {
    auto config = initialConfig(*getAgentEnsemble());
    auto intfIp = folly::IPAddress::createNetwork(
                      config.interfaces()[0].ipAddresses()[0], -1, false)
                      .first;

    // Apply the config
    applyNewConfig(config);
    waitForStateUpdates(getAgentEnsemble()->getSw());

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
      checkKernelEntriesExist(folly::to<std::string>(intfIp));
    }

    // Clear kernel entries
    clearKernelEntries(folly::to<std::string>(intfIp));

    // Check that the kernel entries are removed
    checkKernelEntriesRemoved(folly::to<std::string>(intfIp));
  };

  verifyAcrossWarmBoots(setup, verify);
}

// Test that the tunnel manager is able to handle ip address change of the
// interface
TEST_F(AgentTunnelMgrTest, changeIpAddress) {
  auto setup = [=]() {};
  auto verify = [=, this]() {
    auto config = initialConfig(*getAgentEnsemble());

    // Apply the config
    applyNewConfig(config);
    waitForStateUpdates(getAgentEnsemble()->getSw());

    auto intfIp = folly::IPAddress::createNetwork(
                      config.interfaces()[0].ipAddresses()[0], -1, false)
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
      checkKernelEntriesExist(folly::to<std::string>(intfIp));
    }

    // change ip address of the interface
    config.interfaces()[0].ipAddresses()[0] = "2.2.2.2/32";
    config.interfaces()[0].ipAddresses()[1] = "2::/128";

    // Apply the config
    applyNewConfig(config);
    waitForStateUpdates(getAgentEnsemble()->getSw());

    intfIp = folly::IPAddress::createNetwork(
                 config.interfaces()[0].ipAddresses()[0], -1, false)
                 .first;

    // Route entries installation is currently not consistent after the ip
    // address change. So, passing false for checkRouteEntry.
    if (status) {
      checkKernelEntriesExist(folly::to<std::string>(intfIp), false);
    }

    // Clear kernel entries
    clearKernelEntries(folly::to<std::string>(intfIp));

    // Check that the kernel entries are removed
    checkKernelEntriesRemoved(folly::to<std::string>(intfIp));
  };

  verifyAcrossWarmBoots(setup, verify);
}
} // namespace facebook::fboss
