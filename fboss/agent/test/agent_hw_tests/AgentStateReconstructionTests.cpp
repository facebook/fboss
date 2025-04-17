// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/test/AgentHwTest.h"

namespace facebook::fboss {

class AgentStateReconstructionTests : public AgentHwTest {
  std::vector<production_features::ProductionFeature>
  getProductionFeaturesVerified() const override {
    return {production_features::ProductionFeature::HW_SWITCH};
  };
  void setCmdLineFlagOverrides() const override {
    AgentHwTest::setCmdLineFlagOverrides();
    // enable running on fab switches as well
    FLAGS_hide_fabric_ports = false;
  }
};

TEST_F(AgentStateReconstructionTests, test) {
  EXPECT_THROW(
      getSw()->getHwSwitchHandler()->reconstructSwitchState(SwitchID(0)),
      FbossError);
}
} // namespace facebook::fboss
