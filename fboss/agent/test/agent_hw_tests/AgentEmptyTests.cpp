// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/test/AgentHwTest.h"

namespace facebook::fboss {

class AgentEmptyTest : public AgentHwTest {
 public:
  std::vector<production_features::ProductionFeature>
  getProductionFeaturesVerified() const override {
    return {};
  }
  void setCmdLineFlagOverrides() const override {
    AgentHwTest::setCmdLineFlagOverrides();
    // check basic init with fabric ports enabled
    FLAGS_hide_fabric_ports = false;
  }
};

TEST_F(AgentEmptyTest, CheckInit) {
  verifyAcrossWarmBoots([]() {}, []() {});
}

} // namespace facebook::fboss
