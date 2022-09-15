// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/hw/test/HwTest.h"

namespace facebook::fboss {

class HwVoqSwitchTest : public HwTest {
 public:
  void SetUp() override {
    HwTest::SetUp();
    ASSERT_EQ(getHwSwitch()->getSwitchType(), cfg::SwitchType::VOQ);
    ASSERT_TRUE(getHwSwitch()->getSwitchId().has_value());
  }
};

TEST_F(HwVoqSwitchTest, init) {
  auto setup = [this]() {
    auto newState = getProgrammedState()->clone();
    for (auto& port : *newState->getPorts()) {
      auto newPort = port->modify(&newState);
      newPort->setAdminState(cfg::PortState::ENABLED);
    }
    applyNewState(newState);
  };
  auto verify = [this]() {
    auto state = getProgrammedState();
    for (auto& port : *state->getPorts()) {
      EXPECT_EQ(port->getAdminState(), cfg::PortState::ENABLED);
    }
  };
  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(HwVoqSwitchTest, applyConfig) {
  auto setup = [this]() {
    auto config = utility::onePortPerInterfaceConfig(
        getHwSwitch(),
        masterLogicalPortIds(),
        cfg::PortLoopbackMode::NONE,
        false /*interfaceHasSubnet*/);
    applyNewConfig(config);
  };
  auto verify = [this]() {
    auto state = getProgrammedState();
    for (auto& port : *state->getPorts()) {
      EXPECT_EQ(port->getAdminState(), cfg::PortState::ENABLED);
    }
  };
  verifyAcrossWarmBoots(setup, verify);
}
} // namespace facebook::fboss
