// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/test/agent_hw_tests/AgentVoqSwitchFabricLinkMonitoringTests.h"

#include "fboss/agent/AgentConfig.h"
#include "fboss/agent/AgentFeatures.h"
#include "fboss/agent/FabricConnectivityManager.h"
#include "fboss/agent/HwSwitchThriftClientTable.h"
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/test/utils/DsfConfigUtils.h"

namespace facebook::fboss {

namespace {

// 40 fabric switches, each reachable over 4 consecutive fabric ports
// Total of 160 fabric ports (40 * 4 = 160)
constexpr int kNumFabricSwitches = 40;
constexpr int kFabricPortsPerSwitch = 4;
constexpr int kTotalFabricPorts = kNumFabricSwitches * kFabricPortsPerSwitch;
// SystemPortOffset is added to fabric port ID to derive the SystemPortId.
// Different offsets ensure appropriate systemPortId ranges for each mode.
constexpr int kFabricLinkMonitoringSystemPortOffsetSingleStage = -480;
constexpr int kFabricLinkMonitoringSystemPortOffsetDualStage = -1015;
constexpr int64_t kFabricSwitchIdStart = 512;

// In dual stage mode, we also need L2 fabric nodes to make the config
// recognized as dual stage by numFabricLevels().
// L2 fabric switch IDs start after L1 switches.
constexpr int kNumL2FabricSwitches = 4;
constexpr int64_t kL2FabricSwitchIdStart = 1024;

// Get the system port offset based on dual stage mode
int getFabricLinkMonitoringSystemPortOffset() {
  if (isDualStage3Q2QMode()) {
    return kFabricLinkMonitoringSystemPortOffsetDualStage;
  }
  return kFabricLinkMonitoringSystemPortOffsetSingleStage;
}

int64_t getFabricSwitchId(int fabricSwitchIndex) {
  return kFabricSwitchIdStart +
      (fabricSwitchIndex * utility::kSwitchIdIncrement);
}

std::string getFabricSwitchName(int64_t fabricSwitchId) {
  return "fabric" + std::to_string(fabricSwitchId);
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
      getFabricLinkMonitoringSystemPortOffset();

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

  // Create 40 L1 fabric switches starting at switch ID 512
  // Fabric switch IDs are 4 apart: 512, 516, 520, ..., 668
  // These are the fabric switches that the VOQ switch directly connects to.
  for (int i = 0; i < kNumFabricSwitches; i++) {
    utility::addFabricDsfNode(
        config,
        getFabricSwitchId(i),
        getFabricSwitchName(getFabricSwitchId(i)),
        utility::kL1FabricLevel,
        fabricPlatformType,
        asicType);
  }

  // In dual stage mode, also add L2 fabric switches (fabricLevel = 2) so that
  // numFabricLevels() returns 2 and the system recognizes this as a dual stage
  // configuration.
  if (isDualStage3Q2QMode()) {
    for (int i = 0; i < kNumL2FabricSwitches; i++) {
      int64_t l2SwitchId =
          kL2FabricSwitchIdStart + (i * utility::kSwitchIdIncrement);
      utility::addFabricDsfNode(
          config,
          l2SwitchId,
          getFabricSwitchName(l2SwitchId),
          utility::kL2FabricLevel,
          fabricPlatformType,
          asicType);
    }
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
    auto* fabricLinkMonMgr = getSw()->getFabricLinkMonitoringManager();
    auto initialStats = utility::collectFabricLinkMonitoringStats(
        fabricLinkMonMgr, fabricPorts, numPortsToCheck);

    WITH_RETRIES({
      getSw()->updateStats();
      EXPECT_EVENTUALLY_TRUE(
          utility::allPortsHaveFabricLinkMonitoringCounterIncrements(
              fabricLinkMonMgr, fabricPorts, initialStats, numPortsToCheck))
          << "Waiting for fabric_link_monitoring_tx_packets/rx_packets "
          << "counters to increment by at least "
          << utility::kFabricLinkMonitoringMinCounterIncrement << " on all "
          << numPortsToCheck << " fabric ports";
    });
  };
  verifyAcrossWarmBoots(setup, verify);
}

} // namespace facebook::fboss
