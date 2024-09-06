// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/test/AgentHwTest.h"
#include "fboss/agent/test/utils/AsicUtils.h"

DECLARE_bool(disable_neighbor_updates);
DECLARE_bool(disable_looped_fabric_ports);
DECLARE_bool(dsf_subscribe);

namespace facebook::fboss {

class MultiNodeAgentVoqSwitchTest : public AgentHwTest {
 public:
  cfg::SwitchConfig initialConfig(
      const AgentEnsemble& ensemble) const override {
    XLOG(DBG0) << "initialConfig() loaded config from file " << FLAGS_config;

    auto hwAsics = ensemble.getSw()->getHwAsicTable()->getL3Asics();
    auto asic = utility::checkSameAndGetAsic(hwAsics);

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

TEST_F(MultiNodeAgentVoqSwitchTest, verifyInbandPing) {
  auto setup = []() {};

  auto verify = [this]() {
    std::string ipAddrsToPing;
    for (const auto& [_, dsfNodes] :
         std::as_const(*getProgrammedState()->getDsfNodes())) {
      for (const auto& [_, node] : std::as_const(*dsfNodes)) {
        if (node->getType() == cfg::DsfNodeType::INTERFACE_NODE) {
          CHECK_GE(node->getLoopbackIpsSorted().size(), 1);

          auto ip = node->getLoopbackIpsSorted().begin()->first.str();
          ipAddrsToPing = folly::to<std::string>(ipAddrsToPing, ip, " ");
        }
      }
    }

    auto switchSettings =
        utility::getFirstNodeIf(getSw()->getState()->getSwitchSettings());
    auto switchId =
        SwitchID(switchSettings->getSwitchIdToSwitchInfo().begin()->first);
    auto recyclePortIntfID =
        getRecyclePortIntfID(getProgrammedState(), switchId);
    auto recyclePortIntf = folly::to<std::string>("fboss", recyclePortIntfID);
    auto cmd = folly::to<std::string>(
        "/usr/sbin/fping6 -I ", recyclePortIntf, " ", ipAddrsToPing);

    auto output = runShellCmd(cmd);
    XLOG(DBG2) << "Cmd: " << cmd;
    XLOG(DBG2) << "Output: \n" << output;
  };

  verifyAcrossWarmBoots(setup, verify);
}

} // namespace facebook::fboss
