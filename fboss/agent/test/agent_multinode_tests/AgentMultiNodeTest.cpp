// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/test/AgentHwTest.h"

namespace facebook::fboss {

class AgentMultiNodeTest : public AgentHwTest {
  // TODO: Factor out to VOQ specific Test subclass and implement there
  std::vector<ProductionFeature> getProductionFeaturesVerified()
      const override {
    return {ProductionFeature::VOQ};
  }
};

TEST_F(AgentMultiNodeTest, verifyCluster) {}

} // namespace facebook::fboss
