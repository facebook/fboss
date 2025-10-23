// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/test/AgentHwTest.h"

#include "fboss/agent/AsicUtils.h"
#include "fboss/agent/test/agent_multinode_tests/TopologyInfo.h"

namespace facebook::fboss {

class AgentMultiNodeTest : public AgentHwTest {
  void SetUp() override {
    AgentHwTest::SetUp();
    topologyInfo_ =
        std::make_unique<utility::TopologyInfo>(getProgrammedState());

    if (!topologyInfo_->isTestDriver(*getSw())) {
      throw FbossError(
          "Agent Multi Node test binary can be run on TestDriver only");
    }
  }

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

  // TODO: Factor out to VOQ specific Test subclass and implement there
  std::vector<ProductionFeature> getProductionFeaturesVerified()
      const override {
    return {ProductionFeature::VOQ};
  }

 protected:
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

  void verifyDsfCluster() const {}

  std::unique_ptr<utility::TopologyInfo> topologyInfo_;
};

TEST_F(AgentMultiNodeTest, verifyCluster) {
  switch (topologyInfo_->getTopologyType()) {
    case utility::TopologyInfo::TopologyType::DSF:
      verifyDsfCluster();
      break;
  }
}

} // namespace facebook::fboss
