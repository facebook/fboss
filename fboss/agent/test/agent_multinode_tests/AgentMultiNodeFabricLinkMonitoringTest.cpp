// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#ifndef IS_OSS

#include "fboss/agent/AgentConfig.h"
#include "fboss/agent/AgentFeatures.h"
#include "fboss/agent/test/agent_multinode_tests/AgentMultiNodeDsfUtils.h"
#include "fboss/agent/test/agent_multinode_tests/AgentMultiNodeTest.h"
#include "fboss/agent/test/agent_multinode_tests/AgentMultiNodeUtils.h"

#include <gtest/gtest.h>

namespace facebook::fboss {

namespace {

// SystemPortOffset is added to fabric port ID to derive the SystemPortId.
// Different offsets ensure appropriate systemPortId ranges for each mode.
constexpr int kFabricLinkMonitoringSystemPortOffsetSingleStage = -480;
constexpr int kFabricLinkMonitoringSystemPortOffsetDualStage = -1015;

// Constants for fabric link monitoring verification
constexpr int kWarmupDurationSeconds = 30;
constexpr int kVerificationDurationSeconds = 60;

// Minimum RX and TX counter increment required to consider a port as verified
constexpr int64_t kMinRxTxCounterIncrement = 3;

// Type alias for fabric link monitoring stats
using SwitchFb303Counters =
    std::map<std::string, std::map<std::string, int64_t>>;

// Helper function to check port counters and update portsToMonitor
void updatePortsToMonitorBasedOnCounters(
    const std::vector<std::string>& switchesToCheck,
    const SwitchFb303Counters& baselineFb303Counters,
    std::map<std::string, std::set<std::string>>& portsToMonitor) {
  auto currentFb303Counters =
      utility::collectSwAgentFb303Counters(switchesToCheck);

  for (auto& [switchName, portsRemaining] : portsToMonitor) {
    auto baselineIt = baselineFb303Counters.find(switchName);
    if (baselineIt == baselineFb303Counters.end()) {
      continue;
    }
    auto currentIt = currentFb303Counters.find(switchName);
    if (currentIt == currentFb303Counters.end()) {
      continue;
    }
    auto& baselineFb303 = baselineIt->second;
    auto& currFb303 = currentIt->second;

    // Check each port and remove from monitoring if both TX and RX
    // incremented by >= kMinRxTxCounterIncrement
    std::set<std::string> portsToRemove;
    for (const auto& portName : portsRemaining) {
      auto baselineTx = utility::getFb303CounterValue(
          baselineFb303, portName, "fabric_link_monitoring_tx_packets");
      auto currentTx = utility::getFb303CounterValue(
          currFb303, portName, "fabric_link_monitoring_tx_packets");
      auto txIncrement = currentTx - baselineTx;

      auto baselineRx = utility::getFb303CounterValue(
          baselineFb303, portName, "fabric_link_monitoring_rx_packets");
      auto currentRx = utility::getFb303CounterValue(
          currFb303, portName, "fabric_link_monitoring_rx_packets");
      auto rxIncrement = currentRx - baselineRx;

      XLOG(DBG2) << "Switch: " << switchName << " Port: " << portName
                 << " baseline_tx: " << baselineTx
                 << " current_tx: " << currentTx
                 << " tx_increment: " << txIncrement
                 << " baseline_rx: " << baselineRx
                 << " current_rx: " << currentRx
                 << " rx_increment: " << rxIncrement;

      // Both TX and RX must have incremented by at least
      // kMinRxTxCounterIncrement
      if (txIncrement >= kMinRxTxCounterIncrement &&
          rxIncrement >= kMinRxTxCounterIncrement) {
        XLOG(DBG2) << "Switch: " << switchName << " Port: " << portName
                   << " verified with TX increment of " << txIncrement
                   << " and RX increment of " << rxIncrement;
        portsToRemove.insert(portName);
      }
    }

    // Remove verified ports from monitoring
    for (const auto& port : portsToRemove) {
      portsRemaining.erase(port);
    }
  }

  // Remove switches with no ports remaining to monitor
  for (auto it = portsToMonitor.begin(); it != portsToMonitor.end();) {
    if (it->second.empty()) {
      it = portsToMonitor.erase(it);
    } else {
      ++it;
    }
  }

  // Log current status
  if (!portsToMonitor.empty()) {
    XLOG(DBG2) << "Ports still pending verification:";
    for (const auto& [switchName, ports] : portsToMonitor) {
      XLOG(DBG2) << "  Switch: " << switchName << " - "
                 << folly::join(", ", ports);
    }
  }
}

// Verify counters increment by at least kMinRxTxCounterIncrement for all VoQ
// switch fabric ports
bool verifyFabricLinkMonitoringCountersIncrementing(
    const std::unique_ptr<utility::TopologyInfo>& topologyInfo) {
  XLOG(DBG2) << "Verifying TX and RX counters increment by at least "
             << kMinRxTxCounterIncrement << " for all VoQ switch fabric ports";

  // Get RDSW switches and their connected fabric ports based on topology
  auto rdswToConnectedPorts =
      utility::getRdswConnectedFabricPorts(topologyInfo);

  if (rdswToConnectedPorts.empty()) {
    XLOG(ERR) << "No RDSW switches with connected fabric ports found";
    return false;
  }

  // Build list of switches to collect counters from
  std::vector<std::string> switchesToCheck;
  switchesToCheck.reserve(rdswToConnectedPorts.size());
  for (const auto& [switchName, _] : rdswToConnectedPorts) {
    switchesToCheck.push_back(switchName);
  }

  // Wait for all switches to have actively incrementing counters before
  // capturing the baseline. Remote switches may have recently restarted
  // and need time for fabric link monitoring to become fully active.
  XLOG(DBG2) << "Waiting for all switches to have incrementing counters "
             << "before capturing baseline...";
  auto warmupCounters = utility::collectSwAgentFb303Counters(switchesToCheck);
  WITH_RETRIES_N_TIMED(
      kWarmupDurationSeconds, std::chrono::milliseconds(1000), {
        auto currentCounters =
            utility::collectSwAgentFb303Counters(switchesToCheck);
        bool allIncrementing = true;
        for (const auto& [switchName, ports] : rdswToConnectedPorts) {
          for (const auto& portName : ports) {
            auto warmupTx = utility::getFb303CounterValue(
                warmupCounters[switchName],
                portName,
                "fabric_link_monitoring_tx_packets");
            auto currentTx = utility::getFb303CounterValue(
                currentCounters[switchName],
                portName,
                "fabric_link_monitoring_tx_packets");
            if (currentTx <= warmupTx) {
              allIncrementing = false;
              break;
            }
          }
          if (!allIncrementing) {
            break;
          }
        }
        if (allIncrementing) {
          XLOG(DBG2) << "All switches have incrementing counters";
        }
        EXPECT_EVENTUALLY_TRUE(allIncrementing);
      });

  // Collect baseline TX and RX counters for all ports
  XLOG(DBG2) << "Capturing baseline counters after warmup";
  auto baselineFb303Counters =
      utility::collectSwAgentFb303Counters(switchesToCheck);

  // Track ports that still need to be verified (TX and RX increment >= 3)
  // Map of switch -> set of port names that haven't yet incremented by 3
  std::map<std::string, std::set<std::string>> portsToMonitor;
  for (const auto& [switchName, connectedPorts] : rdswToConnectedPorts) {
    portsToMonitor[switchName] = connectedPorts;
  }

  WITH_RETRIES_N_TIMED(
      kVerificationDurationSeconds, std::chrono::milliseconds(1000), {
        updatePortsToMonitorBasedOnCounters(
            switchesToCheck, baselineFb303Counters, portsToMonitor);

        // Success when all ports have been verified
        EXPECT_EVENTUALLY_TRUE(portsToMonitor.empty());
      });

  // If we get here and portsToMonitor is not empty, log the failures
  if (!portsToMonitor.empty()) {
    XLOG(ERR) << "Fabric link monitoring TX/RX counter verification failed for "
                 "the following ports:";
    for (const auto& [switchName, ports] : portsToMonitor) {
      XLOG(ERR) << "  Switch: " << switchName
                << " - Ports: " << folly::join(", ", ports);
    }
    return false;
  }

  XLOG(DBG2)
      << "All VoQ switch fabric ports verified with TX and RX increment >= "
      << kMinRxTxCounterIncrement;
  return true;
}

} // namespace

class AgentMultiNodeFabricLinkMonitoringTest : public AgentMultiNodeTest {
 public:
  cfg::SwitchConfig initialConfig(
      const AgentEnsemble& ensemble) const override {
    // Load config from file instead of calling parent's initialConfig(),
    // which uses getL3Asics() that fails for fabric switches.
    XLOG(DBG0) << "initialConfig() loading config from file " << FLAGS_config;

    auto agentConfig = AgentConfig::fromFile(FLAGS_config);
    auto config = *agentConfig->thrift.sw();

    // Check if this is a VoQ switch or Fabric switch
    bool isVoqSwitch = false;
    for (const auto& [switchId, hwAsic] :
         ensemble.getSw()->getHwAsicTable()->getHwAsics()) {
      if (hwAsic->getSwitchType() == cfg::SwitchType::VOQ) {
        isVoqSwitch = true;
        break;
      }
    }

    if (isVoqSwitch) {
      // Add fabric link monitoring system port offset for VoQ switches only
      // This setting is not applicable for fabric switches (FDSW/SDSW)
      config.switchSettings()->fabricLinkMonitoringSystemPortOffset() =
          isDualStage3Q2QMode()
          ? kFabricLinkMonitoringSystemPortOffsetDualStage
          : kFabricLinkMonitoringSystemPortOffsetSingleStage;
    }

    return config;
  }

 private:
  void setCmdLineFlagOverrides() const override {
    AgentMultiNodeTest::setCmdLineFlagOverrides();
    // This is needed on all DSF roles
    FLAGS_enable_fabric_link_monitoring = true;
  }
};

TEST_F(AgentMultiNodeFabricLinkMonitoringTest, verifyFabricLinkMonitoring) {
  runTestWithVerifyCluster([](const auto& topologyInfo) {
    if (topologyInfo->getTopologyType() ==
        utility::TopologyInfo::TopologyType::DSF) {
      // Verify fabric link monitoring is working by checking counters
      // are incrementing. Initially, txCount will be incrementing, but
      // once links are up and stable, rxCount will start incrementing.
      // Both tx and rx counts should increment together then on.
      return verifyFabricLinkMonitoringCountersIncrementing(topologyInfo);
    }
    // For non-DSF topologies, skip verification
    return true;
  });
}

} // namespace facebook::fboss

#endif // IS_OSS
