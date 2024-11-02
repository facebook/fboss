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
  void clearKernelEntries(std::string intfIp) {
    // Delete the source route rule entries from the kernel
    auto cmd = folly::to<std::string>("ip rule delete table 1");

    runShellCmd(cmd);

    // Get the String
    cmd = folly::to<std::string>("ip addr list | grep ", intfIp);

    auto output = runShellCmd(cmd);

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
        break;
      }
    } while (iss);
  }

  void checkKernelEntriesRemoved() {
    auto config = initialConfig(*getAgentEnsemble());
    auto intfIp = folly::IPAddress::createNetwork(
                      config.interfaces()[0].ipAddresses()[0], -1, false)
                      .first;

    // Check that the source route rule entries are not present in the kernel
    auto cmd = folly::to<std::string>("ip rule list | grep ", intfIp);

    auto output = runShellCmd(cmd);

    EXPECT_TRUE(
        output.find(folly::to<std::string>(intfIp)) == std::string::npos);

    // Check that the tunnel address entries are not present in the kernel
    cmd = folly::to<std::string>("ip addr list | grep ", intfIp);

    output = runShellCmd(cmd);

    EXPECT_TRUE(
        output.find(folly::to<std::string>(intfIp)) == std::string::npos);

    // Check that the default route entries are not present in the kernel
    cmd = folly::to<std::string>("ip route list | grep ", intfIp);

    output = runShellCmd(cmd);

    EXPECT_TRUE(
        output.find(folly::to<std::string>(intfIp)) == std::string::npos);
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

// Test that the tunnel manager is able to create the source route rule entries,
// tunnel address entries and default route entries in the kernel
TEST_F(AgentTunnelMgrTest, checkKernelEntries) {
  auto setup = [=]() {};
  auto verify = [=, this]() {
    auto config = initialConfig(*getAgentEnsemble());
    auto intfIp = folly::IPAddress::createNetwork(
                      config.interfaces()[0].ipAddresses()[0], -1, false)
                      .first;
    // Get TunManager pointer
    auto tunMgr_ = getAgentEnsemble()->getSw()->getTunManager();
    auto status = tunMgr_->getIntfStatus(
        getProgrammedState(), (InterfaceID)config.interfaces()[0].get_intfID());

    // There is a known limitation in the kernel that the source route rule
    // entries are not created if the interface is not up. So, checking for the
    // kernel entries if the interface is  up
    if (status) {
      // Check that the source route rule entries are present in the kernel
      auto cmd = folly::to<std::string>("ip rule list | grep ", intfIp);

      auto output = runShellCmd(cmd);

      XLOG(DBG2) << "Cmd: " << cmd;
      XLOG(DBG2) << "Output: \n" << output;

      EXPECT_TRUE(
          output.find(folly::to<std::string>(intfIp)) != std::string::npos);

      // Check that the tunnel address entries are present in the kernel
      cmd = folly::to<std::string>("ip addr list | grep ", intfIp);

      output = runShellCmd(cmd);

      EXPECT_TRUE(
          output.find(folly::to<std::string>(intfIp)) != std::string::npos);

      // Check that the default route entries are present in the kernel
      cmd = folly::to<std::string>("ip route list | grep ", intfIp);

      output = runShellCmd(cmd);

      EXPECT_TRUE(
          output.find(folly::to<std::string>(intfIp)) != std::string::npos);
    }

    // Clear kernel entries
    clearKernelEntries(folly::to<std::string>(intfIp));

    // Check that the kernel entries are removed
    checkKernelEntriesRemoved();
  };

  verifyAcrossWarmBoots(setup, verify);
}

} // namespace facebook::fboss
