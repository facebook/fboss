// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

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
  void clearKernelEntries() {
    // Delete the source route rule entries from the kernel
    auto cmd = folly::to<std::string>("ip rule delete table 1");

    runShellCmd(cmd);

    // Delete the tunnel address entries from the kernel
    cmd = folly::to<std::string>("ip link delete fboss2000");

    runShellCmd(cmd);
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

    // Clear kernel entries
    clearKernelEntries();

    // Check that the kernel entries are removed
    checkKernelEntriesRemoved();
  };

  verifyAcrossWarmBoots(setup, verify);
}

} // namespace facebook::fboss
