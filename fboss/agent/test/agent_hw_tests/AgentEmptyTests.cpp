// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/test/AgentHwTest.h"

namespace facebook::fboss {

// Base test class for basic init verification
class AgentEmptyTestBase : public AgentHwTest {
 public:
  std::vector<ProductionFeature> getProductionFeaturesVerified()
      const override {
    return {ProductionFeature::HW_SWITCH};
  }
  void AgentEmptyTest() {
    auto verify = [this]() {
      auto switchId = getCurrentSwitchIdForTesting();
      auto asic = hwAsicForSwitch(switchId);
      auto state = getProgrammedState();
      for (auto portId : masterLogicalPortIds(switchId)) {
        auto port = state->getPorts()->getNodeIf(portId);
        if (port->isEnabled()) {
          EXPECT_EQ(
              port->getLoopbackMode(),
              asic->getDesiredLoopbackMode(port->getPortType()));
        }
      }
#if defined(BRCM_SAI_SDK_DNX_GTE_13_0)
      auto switchIndex =
          getSw()->getSwitchInfoTable().getSwitchIndexFromSwitchId(switchId);
      WITH_RETRIES({
        auto hwSwitchStats = getHwSwitchStats(switchIndex);
        auto asicRevision = hwSwitchStats.fb303GlobalStats()->asic_revision();
        ASSERT_EVENTUALLY_TRUE(asicRevision.has_value());
        XLOG(INFO) << "Switch Id: " << static_cast<int>(switchId)
                   << " asic revision : " << *asicRevision;
      });
#endif
    };
    verifyAcrossWarmBoots([]() {}, verify);
  }
};

class AgentEmptyTest : public AgentEmptyTestBase {
 protected:
  void setCmdLineFlagOverrides() const override {
    AgentEmptyTestBase::setCmdLineFlagOverrides();
    FLAGS_hide_fabric_ports = false;
  }
};

class AgentEmptyVoqTest : public AgentEmptyTestBase {
 protected:
  void setCmdLineFlagOverrides() const override {
    AgentEmptyTestBase::setCmdLineFlagOverrides();
  }
  std::vector<ProductionFeature> getProductionFeaturesVerified()
      const override {
    return {ProductionFeature::VOQ};
  }
};

TEST_F(AgentEmptyTest, CheckInit) {
  AgentEmptyTestBase::AgentEmptyTest();
}

TEST_F(AgentEmptyVoqTest, CheckInit) {
  AgentEmptyTestBase::AgentEmptyTest();
}

} // namespace facebook::fboss
