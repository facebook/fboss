// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/SwSwitch.h"
#include "fboss/agent/test/HwTestHandle.h"
#include "fboss/agent/test/TestUtils.h"

namespace {
constexpr auto kRemoteSwitchID = 4;
constexpr auto kShelSrcIp = "2222::1";
constexpr auto kShelDstIp = "2222::2";
constexpr auto kShelPeriodicIntervalMS = 5000;
} // namespace

namespace facebook::fboss {

class SelfHealingEcmpLagTest : public ::testing::Test {
 public:
  void SetUp() override {
    auto config = initialConfig();
    handle_ = createTestHandle(&config);
    sw_ = handle_->getSw();
  }

  cfg::SwitchConfig initialConfig() {
    auto config = testConfigA(cfg::SwitchType::VOQ);
    auto remoteDsfNode = makeDsfNodeCfg(kRemoteSwitchID);
    config.dsfNodes()->insert({kRemoteSwitchID, remoteDsfNode});
    return config;
  }

  cfg::SwitchConfig selfHealingEcmpLagEnabledConfig() {
    auto config = initialConfig();
    cfg::SelfHealingEcmpLagConfig shelConfig;
    shelConfig.shelSrcIp() = kShelSrcIp;
    shelConfig.shelDstIp() = kShelDstIp;
    shelConfig.shelPeriodicIntervalMS() = kShelPeriodicIntervalMS;
    config.switchSettings()->selfHealingEcmpLagConfig() = shelConfig;
    for (auto& port : *config.ports()) {
      if (port.portType() == cfg::PortType::INTERFACE_PORT ||
          port.portType() == cfg::PortType::MANAGEMENT_PORT) {
        port.selfHealingECMPLagEnable() = true;
      }
    }
    return config;
  }

  void checkSelfHealingEcmpLag(bool enable) {
    auto state = sw_->getState();
    for (const auto& [_, portMap] : std::as_const(*state->getPorts())) {
      for (const auto& [_, port] : std::as_const(*portMap)) {
        if (port->getPortType() == cfg::PortType::INTERFACE_PORT ||
            port->getPortType() == cfg::PortType::MANAGEMENT_PORT) {
          if (enable) {
            EXPECT_EQ(port->getSelfHealingECMPLagEnable(), enable);
          } else {
            EXPECT_TRUE(
                !port->getSelfHealingECMPLagEnable().has_value() ||
                !port->getSelfHealingECMPLagEnable().value());
          }
        }
      }
    }
    // Only had global RCY port in remote system ports, hence shel destination
    // should be enabled.
    EXPECT_GT(state->getRemoteSystemPorts()->size(), 0);
    for (const auto& [_, sysPortMap] :
         std::as_const(*state->getRemoteSystemPorts())) {
      EXPECT_EQ(sysPortMap->size(), 1);
      for (const auto& [_, sysPort] : std::as_const(*sysPortMap)) {
        EXPECT_EQ(sysPort->getShelDestinationEnabled(), enable);
      }
    }

    for (const auto& [_, switchSettings] :
         std::as_const(*state->getSwitchSettings())) {
      EXPECT_EQ(
          switchSettings->getSelfHealingEcmpLagConfig().has_value(), enable);
      if (enable) {
        EXPECT_EQ(
            switchSettings->getSelfHealingEcmpLagConfig()->shelSrcIp(),
            kShelSrcIp);
        EXPECT_EQ(
            switchSettings->getSelfHealingEcmpLagConfig()->shelDstIp(),
            kShelDstIp);
        EXPECT_EQ(
            switchSettings->getSelfHealingEcmpLagConfig()
                ->shelPeriodicIntervalMS(),
            kShelPeriodicIntervalMS);
      }
    }
  }

 protected:
  SwSwitch* sw_;
  std::unique_ptr<HwTestHandle> handle_;
};

TEST_F(SelfHealingEcmpLagTest, enableSelfHealingEcmpLag) {
  // SHEL is disabled by initialConfig()
  checkSelfHealingEcmpLag(false);

  // Enable SHEL
  auto shelEnabledconfig = selfHealingEcmpLagEnabledConfig();
  sw_->applyConfig("Enable SHEL", shelEnabledconfig);
  checkSelfHealingEcmpLag(true);

  // Disable SHEL
  auto config = initialConfig();
  sw_->applyConfig("Disable SHEL", config);
  checkSelfHealingEcmpLag(false);
}
} // namespace facebook::fboss
