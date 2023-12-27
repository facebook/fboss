// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/test/SplitAgentTest.h"

namespace facebook::fboss {

class AgentEmptyTest : public SplitAgentTest {
 public:
  std::vector<production_features::ProductionFeature>
  getProductionFeaturesVerified() const override {
    return {};
  }
};

TEST_F(AgentEmptyTest, CheckInit) {
  verifyAcrossWarmBoots([]() {}, []() {});
}

} // namespace facebook::fboss
