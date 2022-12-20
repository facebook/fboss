// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/hw/test/HwFabricUtils.h"
#include "fboss/agent/hw/test/HwLinkStateDependentTest.h"
#include "fboss/agent/hw/test/HwTest.h"

namespace facebook::fboss {

class HwFabricSwitchTest : public HwLinkStateDependentTest {
 public:
  cfg::SwitchConfig initialConfig() const override {
    return utility::onePortPerInterfaceConfig(
        getHwSwitch(),
        masterLogicalPortIds(),
        getAsic()->desiredLoopbackMode(),
        false /*interfaceHasSubnet*/);
  }
  void SetUp() override {
    HwLinkStateDependentTest::SetUp();
    ASSERT_EQ(getHwSwitch()->getSwitchType(), cfg::SwitchType::FABRIC);
  }
};

TEST_F(HwFabricSwitchTest, init) {
  auto setup = [this]() {};
  auto verify = [this]() {
    auto state = getProgrammedState();
    for (auto& port : *state->getPorts()) {
      EXPECT_EQ(port->getAdminState(), cfg::PortState::ENABLED);
      EXPECT_EQ(port->getLoopbackMode(), getAsic()->desiredLoopbackMode());
    }
  };
  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(HwFabricSwitchTest, collectStats) {
  auto verify = [this]() {
    EXPECT_GT(getProgrammedState()->getPorts()->size(), 0);
    for (auto& port : *getProgrammedState()->getPorts()) {
      getLatestPortStats(port->getID());
    }
  };
  verifyAcrossWarmBoots([] {}, verify);
}

TEST_F(HwFabricSwitchTest, checkFabricReacability) {
  verifyAcrossWarmBoots(
      [] {}, [this]() { checkFabricReachability(getHwSwitch()); });
}

} // namespace facebook::fboss
