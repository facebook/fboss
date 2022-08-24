// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/hw/test/HwTest.h"

namespace facebook::fboss {

class HwFabricSwitchTest : public HwTest {
 public:
  void SetUp() override {
    HwTest::SetUp();
    ASSERT_EQ(getHwSwitch()->getSwitchType(), cfg::SwitchType::FABRIC);
    ASSERT_TRUE(getHwSwitch()->getSwitchId().has_value());
  }
};

TEST_F(HwFabricSwitchTest, init) {
  verifyAcrossWarmBoots([]() {}, []() {});
}

TEST_F(HwFabricSwitchTest, loopbackMode) {
  auto setup = [this]() {
    auto newState = getProgrammedState()->clone();
    for (auto& port : *newState->getPorts()) {
      auto newPort = port->modify(&newState);
      newPort->setLoopbackMode(getAsic()->desiredLoopbackMode());
    }
    applyNewState(newState);
  };

  auto verify = [this]() {
    auto state = getProgrammedState();
    for (auto& port : *state->getPorts()) {
      EXPECT_EQ(getAsic()->desiredLoopbackMode(), port->getLoopbackMode());
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
} // namespace facebook::fboss
