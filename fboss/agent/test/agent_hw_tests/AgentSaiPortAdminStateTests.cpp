// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/test/AgentHwTest.h"
#include "fboss/agent/test/utils/ConfigUtils.h"

namespace facebook::fboss {

class AgentSaiPortAdminStateTest : public AgentHwTest {
 public:
  cfg::SwitchConfig initialConfig(
      const AgentEnsemble& ensemble) const override {
    auto cfg = utility::onePortPerInterfaceConfig(
        ensemble.getSw(),
        ensemble.masterLogicalPortIds(),
        true,
        true,
        utility::kBaseVlanId,
        !FLAGS_hide_fabric_ports);
    for (auto& port : *cfg.ports()) {
      port.state() = cfg::PortState::DISABLED;
    }
    return cfg;
  }

  std::vector<ProductionFeature> getProductionFeaturesVerified()
      const override {
    return {ProductionFeature::L3_FORWARDING};
  }

  void setCmdLineFlagOverrides() const override {
    AgentHwTest::setCmdLineFlagOverrides();
    FLAGS_hide_fabric_ports = false;
  }
};

TEST_F(AgentSaiPortAdminStateTest, assertAdminState) {
  auto setup = [=]() {
    // Ports should be disabled from initialConfig.
    // For VOQ switches, sys ports are always kept enabled. There was a
    // bug in SDK where sys port admin state ended up updating NIF port
    // admin state. This test asserts that these are decoupled.
  };
  auto verify = [=, this]() {
    WITH_RETRIES({
      for (const auto& portMap :
           std::as_const(*getProgrammedState()->getPorts())) {
        for (const auto& [_, port] : std::as_const(*portMap.second)) {
          EXPECT_EVENTUALLY_EQ(port->getAdminState(), cfg::PortState::DISABLED);
        }
      }
    });
  };
  verifyAcrossWarmBoots(setup, verify);
}

} // namespace facebook::fboss
