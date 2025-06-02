// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/AsicUtils.h"
#include "fboss/agent/test/AgentHwTest.h"
#include "fboss/lib/multinode/MultiNodeUtil.h"

DECLARE_bool(disable_neighbor_updates);
DECLARE_bool(disable_looped_fabric_ports);
DECLARE_bool(dsf_subscribe);

using facebook::fboss::utility::MultiNodeUtil;

namespace facebook::fboss {

class MultiNodeAgentVoqSwitchTest : public AgentHwTest {
 public:
  cfg::SwitchConfig initialConfig(
      const AgentEnsemble& ensemble) const override {
    XLOG(DBG0) << "initialConfig() loaded config from file " << FLAGS_config;

    auto hwAsics = ensemble.getSw()->getHwAsicTable()->getL3Asics();
    auto asic = checkSameAndGetAsic(hwAsics);

    auto it = asic->desiredLoopbackModes().find(cfg::PortType::INTERFACE_PORT);
    CHECK(it != asic->desiredLoopbackModes().end());
    auto desiredLoopbackModeForInterfacePort = it->second;

    auto agentConfig = AgentConfig::fromFile(FLAGS_config);
    auto config = *agentConfig->thrift.sw();
    for (auto& port : *config.ports()) {
      if (port.portType() == cfg::PortType::INTERFACE_PORT) {
        // TODO determine loopback mode based on ASIC type
        port.loopbackMode() = desiredLoopbackModeForInterfacePort;
      }
    }

    return config;
  }

  std::vector<production_features::ProductionFeature>
  getProductionFeaturesVerified() const override {
    return {production_features::ProductionFeature::VOQ};
  }

 private:
  void setCmdLineFlagOverrides() const override {
    AgentHwTest::setCmdLineFlagOverrides();
    FLAGS_hide_fabric_ports = false;
    // Allow disabling of looped ports. This should
    // be a noop for VOQ switches
    FLAGS_disable_looped_fabric_ports = true;
    FLAGS_enable_lldp = true;
    FLAGS_tun_intf = true;
    FLAGS_disable_neighbor_updates = false;
    FLAGS_publish_stats_to_fsdb = true;
    FLAGS_publish_state_to_fsdb = true;
    FLAGS_dsf_subscribe = true;
  }
};

TEST_F(MultiNodeAgentVoqSwitchTest, verifyDsfCluster) {
  auto setup = []() {};

  auto verify = [this]() {
    auto multiNodeUtil =
        std::make_unique<MultiNodeUtil>(getProgrammedState()->getDsfNodes());

    WITH_RETRIES_N_TIMED(10, std::chrono::milliseconds(5000), {
      EXPECT_EVENTUALLY_TRUE(multiNodeUtil->verifySystemPorts());
      EXPECT_EVENTUALLY_TRUE(multiNodeUtil->verifyRifs());
      EXPECT_EVENTUALLY_TRUE(multiNodeUtil->verifyStaticNdpEntries());
      EXPECT_EVENTUALLY_TRUE(multiNodeUtil->verifyDsfSessions());
    });
  };

  verifyAcrossWarmBoots(setup, verify);
}

} // namespace facebook::fboss
