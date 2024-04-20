// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <algorithm>
#include "fboss/agent/HwAsicTable.h"
#include "fboss/agent/hw/HwSwitchFb303Stats.h"
#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/test/AgentHwTest.h"
#include "fboss/agent/test/utils/FabricTestUtils.h"
#include "fboss/lib/CommonUtils.h"

DECLARE_bool(disable_looped_fabric_ports);

namespace facebook::fboss {
class AgentFabricSwitchTest : public AgentHwTest {
 public:
  cfg::SwitchConfig initialConfig(
      const AgentEnsemble& ensemble) const override {
    auto config = utility::onePortPerInterfaceConfig(
        ensemble.getSw(),
        ensemble.masterLogicalPortIds(),
        false /*interfaceHasSubnet*/,
        false /*setInterfaceMac*/,
        utility::kBaseVlanId,
        true /*enable fabric ports*/);
    utility::populatePortExpectedNeighbors(
        ensemble.masterLogicalPortIds(), config);
    return config;
  }

  void SetUp() override {
    AgentHwTest::SetUp();
    if (!IsSkipped()) {
      ASSERT_TRUE(
          std::any_of(getAsics().begin(), getAsics().end(), [](auto& iter) {
            return iter.second->getSwitchType() == cfg::SwitchType::FABRIC;
          }));
    }
  }

  std::vector<production_features::ProductionFeature>
  getProductionFeaturesVerified() const override {
    return {production_features::ProductionFeature::FABRIC};
  }
  std::unordered_set<SwitchID> getFabricSwitchIds() const {
    auto fabSwitchIds = getSw()->getSwitchInfoTable().getSwitchIdsOfType(
        cfg::SwitchType::FABRIC);
    CHECK_GT(fabSwitchIds.size(), 0) << " No fab switch ids found";
    return fabSwitchIds;
  }

 protected:
  std::map<SwitchID, std::vector<PortID>> switch2PortIds() const {
    std::map<SwitchID, std::vector<PortID>> switch2PortIds;
    for (auto switchId : getFabricSwitchIds()) {
      switch2PortIds[switchId] =
          getAgentEnsemble()->masterLogicalFabricPortIds(switchId);
    }
    return switch2PortIds;
  }
  void setCmdLineFlagOverrides() const override {
    AgentHwTest::setCmdLineFlagOverrides();
    FLAGS_hide_fabric_ports = false;
  }
};

TEST_F(AgentFabricSwitchTest, init) {
  auto setup = []() {};
  auto verify = [this]() {
    auto state = getProgrammedState();
    for (auto& portMap : std::as_const(*state->getPorts())) {
      for (auto& port : std::as_const(*portMap.second)) {
        EXPECT_EQ(port.second->getAdminState(), cfg::PortState::ENABLED);
        auto portSwitchId =
            getSw()->getScopeResolver()->scope(port.second->getID()).switchId();
        auto portAsic = getSw()->getHwAsicTable()->getHwAsic(portSwitchId);
        EXPECT_EQ(
            port.second->getLoopbackMode(),
            portAsic->getDesiredLoopbackMode(port.second->getPortType()));
      }
    }
  };
  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(AgentFabricSwitchTest, checkFabricReachabilityStats) {
  auto setup = [=, this]() {
    auto newCfg = getSw()->getConfig();
    // reset the neighbor reachability information
    for (const auto& portID : masterLogicalPortIds()) {
      auto portCfg = utility::findCfgPort(newCfg, portID);
      if (portCfg->portType() == cfg::PortType::FABRIC_PORT) {
        portCfg->expectedNeighborReachability() = {};
      }
    }
    applyNewConfig(newCfg);
  };
  auto verify = [this]() {
    EXPECT_GT(getProgrammedState()->getPorts()->numNodes(), 0);
    for (const auto& switchId : getFabricSwitchIds()) {
      utility::checkFabricReachabilityStats(getAgentEnsemble(), switchId);
    }
  };
  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(AgentFabricSwitchTest, collectStats) {
  auto verify = [this]() {
    EXPECT_GT(getProgrammedState()->getPorts()->numNodes(), 0);
    getSw()->updateStats();
  };
  verifyAcrossWarmBoots([] {}, verify);
}

TEST_F(AgentFabricSwitchTest, checkFabricReachability) {
  auto verify = [this]() {
    EXPECT_GT(getProgrammedState()->getPorts()->numNodes(), 0);
    for (const auto& switchId : getFabricSwitchIds()) {
      utility::checkFabricReachability(getAgentEnsemble(), switchId);
    }
  };
  verifyAcrossWarmBoots([] {}, verify);
}

TEST_F(AgentFabricSwitchTest, fabricIsolate) {
  auto setup = [=, this]() {
    auto newCfg = getSw()->getConfig();
    auto fabricPortId =
        PortID(masterLogicalPortIds({cfg::PortType::FABRIC_PORT})[0]);
    for (auto& portCfg : *newCfg.ports()) {
      if (PortID(*portCfg.logicalID()) == fabricPortId) {
        *portCfg.drainState() = cfg::PortDrainState::DRAINED;
        break;
      }
    }
    applyNewConfig(newCfg);
  };

  auto verify = [=, this]() {
    EXPECT_GT(getProgrammedState()->getPorts()->numNodes(), 0);
    auto fabricPortId =
        PortID(masterLogicalPortIds({cfg::PortType::FABRIC_PORT})[0]);
    for (const auto& switchId : getFabricSwitchIds()) {
      utility::checkPortFabricReachability(
          getAgentEnsemble(), switchId, fabricPortId);
    }
  };
  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(AgentFabricSwitchTest, fabricSwitchIsolate) {
  auto setup = [=, this]() {
    setSwitchDrainState(getSw()->getConfig(), cfg::SwitchDrainState::DRAINED);
  };

  auto verify = [=, this]() {
    EXPECT_GT(getProgrammedState()->getPorts()->numNodes(), 0);
    for (const auto& switchId : getFabricSwitchIds()) {
      utility::checkFabricReachability(getAgentEnsemble(), switchId);
    }
  };
  verifyAcrossWarmBoots(setup, verify);
}

class AgentFabricSwitchSelfLoopTest : public AgentFabricSwitchTest {
 public:
  cfg::SwitchConfig initialConfig(
      const AgentEnsemble& ensemble) const override {
    auto config = AgentFabricSwitchTest::initialConfig(ensemble);
    // Start off as drained else all looped ports will get disabled
    config.switchSettings()->switchDrainState() =
        cfg::SwitchDrainState::DRAINED;
    return config;
  }

 protected:
  void verifyState(cfg::PortState desiredState, const PortMap& ports) const {
    std::vector<PortID> portIds;
    for (const auto& [portId, _] : ports) {
      portIds.push_back(PortID(portId));
    }
    verifyState(desiredState, portIds);
  }
  void verifyState(cfg::PortState desiredState, std::vector<PortID>& ports)
      const {
    WITH_RETRIES({
      if (desiredState == cfg::PortState::DISABLED) {
        auto numPorts = ports.size();
        auto switch2SwitchStats = getSw()->getHwSwitchStatsExpensive();
        int missingConnectivity{0};
        for (const auto& [_, switchStats] : switch2SwitchStats) {
          missingConnectivity +=
              *switchStats.fabricReachabilityStats()->missingCount();
        }
        // When disabled all ports should lose connectivity info
        EXPECT_EVENTUALLY_EQ(missingConnectivity, numPorts);
      }
      std::vector<PortError> expectedErrors;
      if (desiredState == cfg::PortState::DISABLED) {
        expectedErrors.push_back(PortError::ERROR_DISABLE_LOOP_DETECTED);
      }
      for (const auto& portId : ports) {
        auto port = getProgrammedState()->getPorts()->getNode(portId);
        EXPECT_EVENTUALLY_EQ(port->getAdminState(), desiredState);
        EXPECT_EVENTUALLY_EQ(port->getActiveErrors(), expectedErrors);

        auto ledExternalState = port->getLedPortExternalState();
        EXPECT_EVENTUALLY_TRUE(ledExternalState.has_value());

        // If port is disabled, connectivity info would disappear
        // so cabling error should clear out.
        auto desiredLedState =
            (desiredState == cfg::PortState::DISABLED
                 ? PortLedExternalState::NONE
                 : PortLedExternalState::CABLING_ERROR_LOOP_DETECTED);
        EXPECT_EVENTUALLY_EQ(*ledExternalState, desiredLedState)
            << " LED State mismatch for port: " << port->getName();
      }
    });
  }

 private:
  void setCmdLineFlagOverrides() const override {
    AgentFabricSwitchTest::setCmdLineFlagOverrides();
    FLAGS_hide_fabric_ports = false;
    FLAGS_disable_looped_fabric_ports = true;
  }
};

TEST_F(AgentFabricSwitchSelfLoopTest, selfLoopDetection) {
  auto verify = [this]() {
    auto allPorts = getProgrammedState()->getPorts()->getAllNodes();
    // Since switch is drained, ports should stay enabled
    verifyState(cfg::PortState::ENABLED, *allPorts);
    // Undrain
    applySwitchDrainState(cfg::SwitchDrainState::UNDRAINED);
    // Ports should now get disabled
    verifyState(cfg::PortState::DISABLED, *allPorts);
  };
  verifyAcrossWarmBoots([]() {}, verify);
}

TEST_F(AgentFabricSwitchSelfLoopTest, portDrained) {
  const auto kPortId =
      getProgrammedState()->getPorts()->getAllNodes()->begin()->first;
  auto setup = [this, kPortId]() {
    // Drain port
    applyNewState([&, kPortId](const std::shared_ptr<SwitchState>& in) {
      auto out = in->clone();
      auto port = out->getPorts()->getNodeIf(kPortId);
      auto newPort = port->modify(&out);
      newPort->setPortDrainState(cfg::PortDrainState::DRAINED);
      return out;
    });
  };
  auto verify = [this, kPortId]() {
    auto portsToCheck = getProgrammedState()->getPorts()->getAllNodes();
    // Since switch is drained, ports should stay enabled
    verifyState(cfg::PortState::ENABLED, *portsToCheck);
    // Undrain
    applySwitchDrainState(cfg::SwitchDrainState::UNDRAINED);
    // All but the drained ports should now get disabled
    portsToCheck->removeNode(kPortId);
    verifyState(cfg::PortState::DISABLED, *portsToCheck);
  };
  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(AgentFabricSwitchTest, reachDiscard) {
  auto verify = [this]() {
    for (auto switchId : getFabricSwitchIds()) {
      auto beforeSwitchDrops =
          *getSw()->getHwSwitchStatsExpensive(switchId).switchDropStats();
      std::string out;
      getAgentEnsemble()->runDiagCommand(
          "TX 1 destination=-1 destinationModid=-1 flags=0x8000\n",
          out,
          switchId);
      WITH_RETRIES({
        auto afterSwitchDrops =
            *getSw()->getHwSwitchStatsExpensive(switchId).switchDropStats();
        XLOG(INFO) << " Before reach drops: "
                   << *beforeSwitchDrops.globalReachabilityDrops()
                   << " After reach drops: "
                   << *afterSwitchDrops.globalReachabilityDrops()
                   << " Before global drops: "
                   << *beforeSwitchDrops.globalDrops()
                   << " After global drops: : "
                   << *afterSwitchDrops.globalDrops();
        EXPECT_EVENTUALLY_EQ(
            *afterSwitchDrops.globalReachabilityDrops(),
            *beforeSwitchDrops.globalReachabilityDrops() + 1);
        // Global drops are in bytes
        EXPECT_EVENTUALLY_GT(
            *afterSwitchDrops.globalDrops(), *beforeSwitchDrops.globalDrops());
      });
    }
    checkNoStatsChange();
  };
  verifyAcrossWarmBoots([]() {}, verify);
}

TEST_F(AgentFabricSwitchTest, dtlQueueWatermarks) {
  auto verify = [this]() {
    std::string out;
    for (auto switchId : getFabricSwitchIds()) {
      WITH_RETRIES({
        auto beforeWatermarks = getAllSwitchWatermarkStats()[switchId];
        EXPECT_EVENTUALLY_TRUE(
            beforeWatermarks.dtlQueueWatermarkBytes().has_value());
        EXPECT_EVENTUALLY_EQ(*beforeWatermarks.dtlQueueWatermarkBytes(), 0);
      });
      getAgentEnsemble()->runDiagCommand(
          "modify RTP_RMHMT 5 1 LINK_BIT_MAP=1\ntx 100 DeSTination=13 DeSTinationModid=5 flags=0x8000\n",
          out,
          switchId);
      WITH_RETRIES({
        auto afterWatermarks = getAllSwitchWatermarkStats()[switchId];
        EXPECT_EVENTUALLY_TRUE(
            afterWatermarks.dtlQueueWatermarkBytes().has_value());
        EXPECT_EVENTUALLY_GT(*afterWatermarks.dtlQueueWatermarkBytes(), 0);
        XLOG(INFO) << "SwitchId: " << switchId
                   << " After DTL queue watermarks: "
                   << *afterWatermarks.dtlQueueWatermarkBytes();
      });
    }
  };

  verifyAcrossWarmBoots([]() {}, verify);
}
} // namespace facebook::fboss
