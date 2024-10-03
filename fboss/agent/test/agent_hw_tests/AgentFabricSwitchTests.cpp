// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <algorithm>
#include "fboss/agent/AgentFeatures.h"
#include "fboss/agent/HwAsicTable.h"
#include "fboss/agent/hw/HwSwitchFb303Stats.h"
#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/test/AgentHwTest.h"
#include "fboss/agent/test/utils/AsicUtils.h"
#include "fboss/agent/test/utils/DsfConfigUtils.h"
#include "fboss/agent/test/utils/FabricTestUtils.h"
#include "fboss/lib/CommonUtils.h"

namespace {
constexpr auto kNumRemoteFabricNodes = 8;
constexpr auto kNumParallelLinks = 5;
} // namespace

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
    utility::populatePortExpectedNeighborsToSelf(
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
  /*
   * Get switchIds of type fabric which have non zero fabric ports
   */
  std::unordered_set<SwitchID> getFabricSwitchIdsWithPorts() const {
    auto fabSwitchIds = getSw()->getSwitchInfoTable().getSwitchIdsOfType(
        cfg::SwitchType::FABRIC);
    // Return fabric switches with ports
    std::erase_if(fabSwitchIds, [this](auto switchId) {
      return getAgentEnsemble()->masterLogicalFabricPortIds(switchId).empty();
    });
    CHECK_GT(fabSwitchIds.size(), 0) << " No fab switch ids found";
    return fabSwitchIds;
  }

 protected:
  std::map<SwitchID, std::vector<PortID>> switch2FabricPortIds() const {
    std::map<SwitchID, std::vector<PortID>> switch2FabricPortIds;
    for (auto switchId : getFabricSwitchIdsWithPorts()) {
      auto fabricPortIds =
          getAgentEnsemble()->masterLogicalFabricPortIds(switchId);
      CHECK(!fabricPortIds.empty());
      switch2FabricPortIds[switchId] = std::move(fabricPortIds);
    }
    return switch2FabricPortIds;
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

TEST_F(AgentFabricSwitchTest, checkFabricConnectivityStats) {
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
    WITH_RETRIES({
      auto reachabilityStats = getAgentEnsemble()->getFabricReachabilityStats();
      EXPECT_EVENTUALLY_EQ(
          reachabilityStats.missingCount(),
          masterLogicalFabricPortIds().size());
      EXPECT_EVENTUALLY_EQ(reachabilityStats.mismatchCount(), 0);
    });
  };
  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(AgentFabricSwitchTest, collectStats) {
  auto verify = [this]() {
    EXPECT_GT(getProgrammedState()->getPorts()->numNodes(), 0);
    WITH_RETRIES({
      getSw()->updateStats();
      for (auto& portMap : std::as_const(*getProgrammedState()->getPorts())) {
        for (auto& [_, port] : std::as_const(*portMap.second)) {
          auto loadBearingInErrors = fb303::fbData->getCounterIfExists(
              port->getName() + ".load_bearing_in_errors.sum.60");
          auto loadBearingFecErrors = fb303::fbData->getCounterIfExists(
              port->getName() +
              ".load_bearing_fec_uncorrectable_errors.sum.60");
          auto loadBearingFlaps = fb303::fbData->getCounterIfExists(
              port->getName() + ".load_bearing_link_state.flap.sum.60");
          EXPECT_EVENTUALLY_TRUE(loadBearingInErrors.has_value());
          EXPECT_EVENTUALLY_TRUE(loadBearingFecErrors.has_value());
          EXPECT_EVENTUALLY_TRUE(loadBearingFlaps.has_value());
        }
      }
    });
  };
  verifyAcrossWarmBoots([] {}, verify);
}

TEST_F(AgentFabricSwitchTest, checkFabricConnectivity) {
  auto verify = [this]() {
    EXPECT_GT(getProgrammedState()->getPorts()->numNodes(), 0);
    for (const auto& switchId : getFabricSwitchIdsWithPorts()) {
      utility::checkFabricConnectivity(getAgentEnsemble(), switchId);
    }
  };
  verifyAcrossWarmBoots([] {}, verify);
}

TEST_F(AgentFabricSwitchTest, fabricPortIsolate) {
  std::map<SwitchID, PortID> switchId2FabricPortId;
  std::set<PortID> fabricPortIds;
  for (const auto& [switchId, portIds] : switch2FabricPortIds()) {
    fabricPortIds.insert(portIds[0]);
    switchId2FabricPortId.insert({switchId, portIds[0]});
  }
  ASSERT_GT(fabricPortIds.size(), 0);
  ASSERT_GT(switchId2FabricPortId.size(), 0);
  auto setup = [=, this]() {
    auto newCfg = getSw()->getConfig();
    for (auto& portCfg : *newCfg.ports()) {
      if (fabricPortIds.find(PortID(*portCfg.logicalID())) !=
          fabricPortIds.end()) {
        *portCfg.drainState() = cfg::PortDrainState::DRAINED;
      }
    }
    applyNewConfig(newCfg);
  };

  auto verify = [=, this]() {
    EXPECT_GT(getProgrammedState()->getPorts()->numNodes(), 0);
    for (auto [switchId, fabricPortId] : switchId2FabricPortId) {
      utility::checkPortFabricReachability(
          getAgentEnsemble(), switchId, fabricPortId);
    }
    std::vector<PortID> undrainedPortIds = masterLogicalFabricPortIds();
    std::vector<PortID> drainedPortIds(
        fabricPortIds.begin(), fabricPortIds.end());
    // Remove the drained port
    undrainedPortIds.erase(
        std::remove_if(
            undrainedPortIds.begin(),
            undrainedPortIds.end(),
            [&fabricPortIds](const PortID& portId) {
              return fabricPortIds.find(portId) != fabricPortIds.end();
            }),
        undrainedPortIds.end());
    // Only drained port will be inactive
    utility::checkFabricPortsActiveState(
        getAgentEnsemble(), drainedPortIds, false /*expectActive*/);
    utility::checkFabricPortsActiveState(
        getAgentEnsemble(), undrainedPortIds, true /*expectActive*/);
  };
  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(AgentFabricSwitchTest, fabricSwitchIsolate) {
  auto setup = [=, this]() {
    setSwitchDrainState(getSw()->getConfig(), cfg::SwitchDrainState::DRAINED);
  };

  auto verify = [=, this]() {
    EXPECT_GT(getProgrammedState()->getPorts()->numNodes(), 0);
    for (const auto& switchId : getFabricSwitchIdsWithPorts()) {
      utility::checkFabricConnectivity(getAgentEnsemble(), switchId);
    }
    // All ports should go to inactive state when switch is drained and
    // ports are in loopback!
    utility::checkFabricPortsActiveState(
        getAgentEnsemble(),
        masterLogicalFabricPortIds(),
        false /*expectActive*/);
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

 protected:
  void checkDataCellFilter(bool expectFilterOn) {
    for (const auto& [_, portIds] : switch2FabricPortIds()) {
      checkDataCellFilter(expectFilterOn, portIds);
    }
  }
  void checkDataCellFilter(
      bool expectFilterOn,
      const std::vector<PortID>& portIds) {
    WITH_RETRIES({
      for (const auto& [_, portStats] : getNextUpdatedPortStats(portIds)) {
        EXPECT_EVENTUALLY_TRUE(portStats.dataCellsFilterOn().has_value());
        EXPECT_EVENTUALLY_EQ(*portStats.dataCellsFilterOn(), expectFilterOn);
      }
    });
  }

 private:
  void setCmdLineFlagOverrides() const override {
    AgentFabricSwitchTest::setCmdLineFlagOverrides();
    FLAGS_disable_looped_fabric_ports = true;
    FLAGS_detect_wrong_fabric_connections = true;
  }
};

TEST_F(AgentFabricSwitchSelfLoopTest, selfLoopDetection) {
  auto setup = [this]() {
    auto allPorts = getProgrammedState()->getPorts()->getAllNodes();
    // Since switch is drained, ports should stay enabled
    verifyState(cfg::PortState::ENABLED, *allPorts);
    // Regardless of drain state, data filter should be turned on
    // upon detecting a loop
    checkDataCellFilter(true /*expectFilterOn*/);
    // Undrain
    setSwitchDrainState(getSw()->getConfig(), cfg::SwitchDrainState::UNDRAINED);
  };
  auto verify = [this]() {
    auto allPorts = getProgrammedState()->getPorts()->getAllNodes();
    // Ports should now get disabled
    verifyState(cfg::PortState::DISABLED, *allPorts);
  };
  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(AgentFabricSwitchSelfLoopTest, portDrained) {
  std::set<PortID> drainedPorts;
  for (const auto& [_, ports] : switch2FabricPortIds()) {
    drainedPorts.insert(ports[0]);
  }
  auto setup = [this, drainedPorts]() {
    // Drain port
    auto newCfg = getSw()->getConfig();
    for (auto& port : *newCfg.ports()) {
      if (drainedPorts.find(PortID(*port.logicalID())) != drainedPorts.end()) {
        port.drainState() = cfg::PortDrainState::DRAINED;
      }
    }
    applyNewConfig(newCfg);
    auto portsToCheck = getProgrammedState()->getPorts()->getAllNodes();
    // Since switch is drained, ports should stay enabled
    verifyState(cfg::PortState::ENABLED, *portsToCheck);
    // Regardless of drain state, data filter should be turned on
    // upon detecting a loop
    checkDataCellFilter(true /*expectFilterOn*/);
    // Undrain
    setSwitchDrainState(newCfg, cfg::SwitchDrainState::UNDRAINED);
  };
  auto verify = [this, drainedPorts]() {
    auto portsToCheck = getProgrammedState()->getPorts()->getAllNodes();
    // All but the drained ports should now get disabled
    for (auto port : drainedPorts) {
      portsToCheck->removeNode(port);
    }
    verifyState(cfg::PortState::DISABLED, *portsToCheck);
    // ENABLED, but drained ports should still detect wrong connection
    // and have filter on. For disabled ports, we stop collecting stats,
    // so filter status is not updated.
    checkDataCellFilter(
        true, std::vector<PortID>(drainedPorts.begin(), drainedPorts.end()));
    // Verify that global drops are zero
    auto switch2SwitchStats = getSw()->getHwSwitchStatsExpensive();
    for (const auto& [_, switchStats] : switch2SwitchStats) {
      const auto& dropStats = *switchStats.switchDropStats();
      EXPECT_EQ(*dropStats.globalDrops(), 0);
    }
  };
  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(AgentFabricSwitchTest, reachDiscard) {
  auto verify = [this]() {
    for (auto switchId : getFabricSwitchIdsWithPorts()) {
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
    for (auto switchId : getFabricSwitchIdsWithPorts()) {
      utility::checkFabricPortsActiveState(
          getAgentEnsemble(),
          masterLogicalFabricPortIds(),
          true /*expectActive*/);
      WITH_RETRIES({
        auto beforeWatermarks = getAllSwitchWatermarkStats()[switchId];
        EXPECT_EVENTUALLY_TRUE(
            beforeWatermarks.dtlQueueWatermarkBytes().has_value());
        EXPECT_EVENTUALLY_EQ(*beforeWatermarks.dtlQueueWatermarkBytes(), 0);
      });
      getAgentEnsemble()->runDiagCommand(
          "modify RTP_RMHMT 5 1 LINK_BIT_MAP=1\ntx 1000 DeSTination=13 DeSTinationModid=5 flags=0x8000\n",
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

TEST_F(AgentFabricSwitchTest, switchReachability) {
  auto verify = [this]() {
    utility::checkFabricPortsActiveState(
        getAgentEnsemble(),
        masterLogicalFabricPortIds(),
        true /*expectActive*/);
    for (auto switchId : getFabricSwitchIdsWithPorts()) {
      bool switchReachabilityWorking = false;
      WITH_RETRIES({
        auto switchIter = getSw()->getSwitchReachability().find(switchId);
        EXPECT_EVENTUALLY_TRUE(
            switchIter != getSw()->getSwitchReachability().end());
        for (auto& [destinationSwitchId, portGroupId] :
             *switchIter->second.switchIdToFabricPortGroupMap()) {
          auto portGroupIter =
              switchIter->second.fabricPortGroupMap()->find(portGroupId);
          if (portGroupIter != switchIter->second.fabricPortGroupMap()->end()) {
            XLOG(DBG0) << "On local switch id " << switchId
                       << ", destination switch id " << destinationSwitchId
                       << " reachability available over port group ID "
                       << portGroupId << ", with group size "
                       << portGroupIter->second.size();
            // Fabric device does not have physical rechability to
            // any other device, so no ports expected in port group.
            EXPECT_EQ(portGroupIter->second.size(), 0);
            switchReachabilityWorking = true;
          }
        }
        EXPECT_EVENTUALLY_TRUE(switchReachabilityWorking);
      });
    }
  };

  verifyAcrossWarmBoots([]() {}, verify);
}

class AgentBalancedInputModeTest : public AgentFabricSwitchTest {
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
    // Initialize local switch as level 2 (SDSW)
    const auto selfFabricLevel = 2;
    const auto remoteFabricLevel = 1;
    for (auto& [_, dsfNode] : *config.dsfNodes()) {
      dsfNode.fabricLevel() = selfFabricLevel;
    }
    std::vector<int> fabricSwitchIds;
    // Add fabric nodes to DsfConfig with different clusterIDs
    for (int i = 0; i < kNumRemoteFabricNodes; i++) {
      auto [fabricSwitchId, fabricNode] = utility::getRemoteFabricNodeCfg(
          *config.dsfNodes(),
          remoteFabricLevel,
          i /* clusterId */,
          utility::checkSameAndGetAsic(
              ensemble.getHwAsicTable()->getFabricAsics())
              ->getAsicType(),
          ensemble.getSw()->getPlatformType());
      config.dsfNodes()->insert({fabricSwitchId, fabricNode});
      fabricSwitchIds.push_back(fabricSwitchId);
    }
    // Populate expected neighbor reachability to all remote fabric nodes
    utility::populatePortExpectedNeighborsToRemote(
        ensemble.masterLogicalFabricPortIds(),
        config,
        fabricSwitchIds,
        kNumParallelLinks);
    return config;
  }

 private:
  void setCmdLineFlagOverrides() const override {
    AgentFabricSwitchTest::setCmdLineFlagOverrides();
    FLAGS_disable_looped_fabric_ports = false;
    FLAGS_detect_wrong_fabric_connections = false;
  }
};

TEST_F(AgentBalancedInputModeTest, init) {
  auto verify = [this]() {
    auto fabricPortIds = masterLogicalFabricPortIds();
    auto fabricPortIter = fabricPortIds.begin();
    std::optional<int> currReachabilityGroupId;

    for (int i = 0; i < kNumRemoteFabricNodes; i++) {
      currReachabilityGroupId = std::nullopt;

      for (int j = 0; j < kNumParallelLinks - 1; j++) {
        CHECK(fabricPortIter != fabricPortIds.end());
        auto port = getProgrammedState()->getPorts()->getNode(*fabricPortIter);
        EXPECT_TRUE(port->getReachabilityGroupId().has_value());

        // Check ports to the same remote neighbor has same reachabilityGroupId
        if (!currReachabilityGroupId.has_value()) {
          currReachabilityGroupId = port->getReachabilityGroupId();
        }
        EXPECT_EQ(currReachabilityGroupId, port->getReachabilityGroupId());
      }
    }
  };
  verifyAcrossWarmBoots([]() {}, verify);
}
} // namespace facebook::fboss
