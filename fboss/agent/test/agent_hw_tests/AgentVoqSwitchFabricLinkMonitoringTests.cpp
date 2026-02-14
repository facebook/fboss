// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/test/agent_hw_tests/AgentVoqSwitchFabricLinkMonitoringTests.h"

#include "fboss/agent/AgentConfig.h"
#include "fboss/agent/AgentFeatures.h"
#include "fboss/agent/FabricConnectivityManager.h"
#include "fboss/agent/FabricLinkMonitoringManager.h"
#include "fboss/agent/HwSwitchThriftClientTable.h"
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/test/utils/FabricTestUtils.h"

namespace facebook::fboss {

namespace {

// 40 fabric switches, each reachable over 4 consecutive fabric ports
// Total of 160 fabric ports (40 * 4 = 160)
constexpr int kNumFabricSwitches = 40;
constexpr int kFabricPortsPerSwitch = 4;
constexpr int kTotalFabricPorts = kNumFabricSwitches * kFabricPortsPerSwitch;
constexpr int kFabricLinkMonitoringSystemPortOffset = -480;
constexpr int64_t kFabricSwitchIdStart = 512;
constexpr int64_t kMinCounterIncrement = 3;

int64_t getFabricSwitchId(int fabricSwitchIndex) {
  return kFabricSwitchIdStart + (fabricSwitchIndex * 4);
}

std::string getFabricSwitchName(int64_t fabricSwitchId) {
  return "fabric" + std::to_string(fabricSwitchId);
}

void addFabricDsfNode(
    cfg::SwitchConfig& config,
    int64_t switchId,
    int fabricLevel,
    PlatformType platformType,
    cfg::AsicType asicType) {
  cfg::DsfNode node;
  node.switchId() = switchId;
  node.name() = getFabricSwitchName(switchId);
  node.type() = cfg::DsfNodeType::FABRIC_NODE;
  node.fabricLevel() = fabricLevel;
  node.platformType() = platformType;
  node.asicType() = asicType;
  (*config.dsfNodes())[switchId] = node;
}

// Collect initial fabric link monitoring stats from PortStats
// Returns a map of portID -> {txCount, rxCount}
std::map<PortID, std::pair<int64_t, int64_t>> collectInitialStats(
    SwSwitch* sw,
    const std::vector<PortID>& fabricPorts,
    size_t numPortsToCheck) {
  std::map<PortID, std::pair<int64_t, int64_t>> initialStats;

  for (size_t i = 0; i < numPortsToCheck; i++) {
    const auto& portId = fabricPorts[i];
    auto* portStat = sw->portStats(portId);
    CHECK(portStat) << "PortStats should exist for port " << portId;

    int64_t txCount = portStat->getFabricLinkMonitoringTxPackets();
    int64_t rxCount = portStat->getFabricLinkMonitoringRxPackets();

    initialStats[portId] = {txCount, rxCount};
  }
  return initialStats;
}

bool allPortsHaveIncrementedCounters(
    SwSwitch* sw,
    const std::vector<PortID>& fabricPorts,
    const std::map<PortID, std::pair<int64_t, int64_t>>& initialStats,
    size_t numPortsToCheck) {
  auto state = sw->getState();

  for (size_t i = 0; i < numPortsToCheck; i++) {
    auto portId = fabricPorts[i];
    auto port = state->getPorts()->getNodeIf(portId);
    CHECK(port) << "Port " << portId << " should exist in state";
    auto portName = port->getName();

    auto it = initialStats.find(portId);
    CHECK(it != initialStats.end())
        << "Port " << portId << " should exist in initialStats";
    auto [initialTx, initialRx] = it->second;

    // Get current fabric link monitoring counters from PortStats
    auto* portStat = sw->portStats(portId);
    CHECK(portStat) << "PortStats should exist for port " << portId;

    int64_t currentTx = portStat->getFabricLinkMonitoringTxPackets();
    int64_t currentRx = portStat->getFabricLinkMonitoringRxPackets();

    int64_t txIncrement = currentTx - initialTx;
    int64_t rxIncrement = currentRx - initialRx;

    XLOG(DBG3) << "Port " << portName << ": TX increment=" << txIncrement
               << ", RX increment=" << rxIncrement;

    if (txIncrement < kMinCounterIncrement ||
        rxIncrement < kMinCounterIncrement) {
      return false;
    }
  }
  return true;
}

} // namespace

cfg::SwitchConfig AgentVoqSwitchFabricLinkMonitoringTest::initialConfig(
    const AgentEnsemble& ensemble) const {
  FLAGS_hwswitch_query_timeout = 5000;
  auto config = utility::onePortPerInterfaceConfig(
      ensemble.getSw(),
      ensemble.masterLogicalPortIds(),
      true, /*interfaceHasSubnet*/
      true, /*setInterfaceMac*/
      utility::kBaseVlanId,
      true /*enable fabric ports*/);

  auto l3Asics = ensemble.getL3Asics();
  CHECK(!l3Asics.empty()) << "No L3 ASICs found in ensemble";
  auto switchId = l3Asics[0]->getSwitchId();
  CHECK(switchId.has_value()) << "Switch ID not set on ASIC";
  config.switchSettings()->switchId() = static_cast<int64_t>(*switchId);
  config.switchSettings()->switchType() = cfg::SwitchType::VOQ;

  config.switchSettings()->fabricLinkMonitoringSystemPortOffset() =
      kFabricLinkMonitoringSystemPortOffset;

  addFabricLinkMonitoringDsfNodes(config, ensemble);

  return config;
}

void AgentVoqSwitchFabricLinkMonitoringTest::setCmdLineFlagOverrides() const {
  AgentVoqSwitchTest::setCmdLineFlagOverrides();
  FLAGS_hide_fabric_ports = false;
  FLAGS_enable_fabric_link_monitoring = true;
  if (!FLAGS_multi_switch) {
    FLAGS_janga_single_npu_for_testing = true;
  }
}

void AgentVoqSwitchFabricLinkMonitoringTest::addFabricLinkMonitoringDsfNodes(
    cfg::SwitchConfig& config,
    const AgentEnsemble& ensemble) const {
  // Hardcode fabric platform and ASIC
  auto asicType = cfg::AsicType::ASIC_TYPE_RAMON3;
  auto fabricPlatformType = PlatformType::PLATFORM_MERU800BFA;

  // Create 40 fabric switches starting at switch ID 512
  // Fabric switch IDs are 4 apart: 512, 516, 520, ..., 668
  for (int i = 0; i < kNumFabricSwitches; i++) {
    addFabricDsfNode(
        config, getFabricSwitchId(i), 1, fabricPlatformType, asicType);
  }

  // Configure expected neighbor reachability for each fabric port
  // Each remote fabric switch is reachable over 4 consecutive fabric
  // ports. Remote port names follow the pattern fab1/1/1, fab1/1/2,
  // fab1/1/3, fab1/1/4
  auto fabricPorts = ensemble.masterLogicalFabricPortIds();
  int fabricPortIndex = 0;
  for (auto& portCfg : *config.ports()) {
    if (*portCfg.portType() != cfg::PortType::FABRIC_PORT) {
      continue;
    }
    if (fabricPortIndex >= kTotalFabricPorts) {
      break;
    }

    // Determine which fabric switch this port connects to (0-39)
    int fabricSwitchIndex = fabricPortIndex / kFabricPortsPerSwitch;
    // Determine which port on the remote fabric switch (0-3)
    int remotePortIndex = fabricPortIndex % kFabricPortsPerSwitch;

    cfg::PortNeighbor neighbor;
    neighbor.remoteSystem() =
        getFabricSwitchName(getFabricSwitchId(fabricSwitchIndex));
    // Remote port names: fab1/1/1, fab1/1/2, fab1/1/3, fab1/1/4
    neighbor.remotePort() = "fab1/1/" + std::to_string(remotePortIndex + 1);
    portCfg.expectedNeighborReachability() = {neighbor};

    fabricPortIndex++;
  }
}

TEST_F(AgentVoqSwitchFabricLinkMonitoringTest, validateFabricLinkMonitoring) {
  auto setup = []() {};
  auto verify = [this]() {
    auto fabricPorts = masterLogicalFabricPortIds();
    if (fabricPorts.empty()) {
      XLOG(WARNING) << "No fabric ports found, skipping verification";
      return;
    }

    size_t numPortsToCheck =
        std::min(fabricPorts.size(), static_cast<size_t>(kTotalFabricPorts));

    // Collect initial stats from PortStats fb303 counters
    // (fabric_link_monitoring_tx_packets and fabric_link_monitoring_rx_packets)
    auto initialStats =
        collectInitialStats(getSw(), fabricPorts, numPortsToCheck);

    WITH_RETRIES({
      getSw()->updateStats();
      EXPECT_EVENTUALLY_TRUE(allPortsHaveIncrementedCounters(
          getSw(), fabricPorts, initialStats, numPortsToCheck))
          << "Waiting for fabric_link_monitoring_tx_packets/rx_packets "
          << "counters to increment by at least " << kMinCounterIncrement
          << " on all " << numPortsToCheck << " fabric ports";
    });
  };
  verifyAcrossWarmBoots(setup, verify);
}

} // namespace facebook::fboss
