// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/test/AgentHwTest.h"

namespace facebook::fboss {

class AgentStateReconstructionTests : public AgentHwTest {
  std::vector<production_features::ProductionFeature>
  getProductionFeaturesVerified() const override {
    // TODO: add a production feature that applies on platforms
    return {};
  };
};

TEST_F(AgentStateReconstructionTests, test) {
  EXPECT_THROW(
      getSw()->getHwSwitchHandler()->reconstructSwitchState(SwitchID(0)),
      FbossError);
}
} // namespace facebook::fboss
