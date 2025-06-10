// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/test/AgentHwTest.h"
#include "fboss/lib/CommonUtils.h"

namespace facebook::fboss {

class AgentEmptyTest : public AgentHwTest {
 public:
  std::vector<ProductionFeature> getProductionFeaturesVerified()
      const override {
    return {};
  }
  void setCmdLineFlagOverrides() const override {
    AgentHwTest::setCmdLineFlagOverrides();
    // check basic init with fabric ports enabled
    FLAGS_hide_fabric_ports = false;
  }
};

TEST_F(AgentEmptyTest, CheckInit) {
  auto verify = [this]() {
    auto state = getProgrammedState();
    for (auto& portMap : std::as_const(*state->getPorts())) {
      for (auto& port : std::as_const(*portMap.second)) {
        if (port.second->isEnabled()) {
          EXPECT_EQ(
              port.second->getLoopbackMode(),
              // TODO: Handle multiple Asics
              getAsics().cbegin()->second->getDesiredLoopbackMode(
                  port.second->getPortType()));
        }
      }
    }
#if defined(BRCM_SAI_SDK_DNX_GTE_13_0)
    WITH_RETRIES({
      for (const auto& [switchId, hwSwitchStats] : getHwSwitchStats()) {
        auto asicRevision = hwSwitchStats.fb303GlobalStats()->asic_revision();
        ASSERT_EVENTUALLY_TRUE(asicRevision.has_value());
        XLOG(INFO) << "Switch Id: " << switchId
                   << " asic revision : " << *asicRevision;
      }
    });
#endif
  };
  verifyAcrossWarmBoots([]() {}, verify);
}

} // namespace facebook::fboss
