// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/hw/test/HwTest.h"

namespace facebook::fboss {

class HwFabricSwitchTest : public HwTest {
 private:
  std::pair<std::optional<SwitchID>, cfg::SwitchType> getSwitchIdAndType()
      const override {
    return std::make_pair(SwitchID(0), cfg::SwitchType::FABRIC);
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
