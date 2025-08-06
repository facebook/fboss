// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <chrono>
#include <map>
#include <memory>
#include <thread>
#include <vector>

#include "folly/container/F14Map.h"
#include "folly/executors/FunctionScheduler.h"
#include "gtest/gtest.h"

#include "fboss/agent/AgentFeatures.h"
#include "fboss/agent/HwSwitchThriftClientTable.h"
#include "fboss/agent/SwSwitch.h"
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/hw/gen-cpp2/hardware_stats_types.h"
#include "fboss/agent/if/gen-cpp2/FbossHwCtrlAsyncClient.h"
#include "fboss/agent/if/gen-cpp2/highfreq_types.h"
#include "fboss/agent/test/AgentEnsemble.h"
#include "fboss/agent/test/AgentHwTest.h"
#include "fboss/agent/test/utils/ConfigUtils.h"
#include "fboss/agent/test/utils/PfcTestUtils.h"
#include "fboss/agent/types.h"

namespace facebook::fboss {

class AgentHighFrequencyStatsTest : public AgentHwTest {
 public:
  void setCmdLineFlagOverrides() const override {
    AgentHwTest::setCmdLineFlagOverrides();
    FLAGS_enable_high_frequency_stats_polling = true;
  }

  cfg::SwitchConfig initialConfig(
      const AgentEnsemble& ensemble) const override {
    auto config = utility::onePortPerInterfaceConfig(
        ensemble.getSw(),
        ensemble.masterLogicalPortIds(),
        true /*interfaceHasSubnet*/);
    return config;
  }

  std::vector<ProductionFeature> getProductionFeaturesVerified()
      const override {
    return {
        ProductionFeature::PFC,
        ProductionFeature::HIGH_FREQUENCY_CPU_POLLING_STATS};
  }

 protected:
  void sendPfcFrame(const std::vector<PortID>& portIds, uint8_t classVector) {
    for (const PortID& portId : portIds) {
      getSw()->sendPacketOutOfPortAsync(
          utility::makePfcFramePacket(*getAgentEnsemble(), classVector),
          portId);
    }
  }
};

// Test the PFC counters for high frequency stats collection. Verify that the
// PFC counters are zero at cold boot. Set up PFC on two ports (logical
// interface ports 0 and 1). Set up a thrift client to call the high frequency
// stats collection API on the hw agent. Send PFC frames on both ports and run
// the high frequency stats collection job simultaneously. Call the get high
// frequency stats API using a port filter config for the ports with PFC. Verify
// that multiple time series stats are returned. Verify that the PFC counters
// are non-zero for all the ports and all the lossless PG IDs.
TEST_F(AgentHighFrequencyStatsTest, PfcTest) {
  std::vector<PortID> portIds{
      masterLogicalInterfacePortIds()[0], masterLogicalInterfacePortIds()[1]};
  std::vector<int> losslessPgIds{2};
  std::vector<int> lossyPgIds{0};

  auto setup = [&]() {
    cfg::SwitchConfig cfg = getAgentEnsemble()->getCurrentConfig();
    utility::setupPfcBuffers(
        getAgentEnsemble(), cfg, portIds, losslessPgIds, lossyPgIds);
    applyNewConfig(cfg);

    for (const auto& [portId, stats] : getLatestPortStats(portIds)) {
      for (const auto& [qos, value] : stats.inPfc_().value()) {
        EXPECT_EQ(value, 0);
      }
    }
  };

  auto verify = [&]() {
    // Collect PFC counters before test
    const std::map<PortID, HwPortStats> initialStats{
        getLatestPortStats(portIds)};

    apache::thrift::Client<FbossHwCtrl>* client =
        getSw()->getHwSwitchThriftClientTable()->getClient(SwitchID(0));

    const std::shared_ptr<SwitchState> state = getProgrammedState();
    ASSERT_TRUE(state);

    // Maintain a map of port ID to port name
    std::map<PortID, std::string> port2Name{};

    // Set up the high frequency stats collection config
    HighFrequencyStatsCollectionConfig config{};
    config.schedulerConfig() = HfSchedulerConfig();
    // Poll every 20ms for 200 ms. Note that the current minimum polling
    // interval is 20ms.
    const int64_t kStatsWaitDurationInMicroseconds = 20000;
    config.schedulerConfig()->statsWaitDurationInMicroseconds() =
        kStatsWaitDurationInMicroseconds;
    config.schedulerConfig()->statsCollectionDurationInMicroseconds() = 200000;
    config.statsConfig() = HfStatsConfig();
    config.statsConfig()->portStatsConfig() = HfPortStatsConfig();
    HfPortStatsCollectionConfig portStatsConfig{};
    portStatsConfig.includePfcRx() = true;
    portStatsConfig.includePfcTx() = true;
    portStatsConfig.includePgWatermark() = false;
    portStatsConfig.includeQueueWatermark() = false;
    folly::F14FastMap<std::string, HfPortStatsCollectionConfig> filterConfig{};
    for (const PortID& portId : portIds) {
      const std::shared_ptr<Port> port = state->getPort(portId);
      ASSERT_TRUE(port) << "No port found for portId: " << portId;
      filterConfig[port->getName()] = portStatsConfig;
      port2Name[portId] = port->getName();
    }
    config.statsConfig()->portStatsConfig()->filterConfig() = filterConfig;
    config.statsConfig()->includeDeviceWatermark() = false;

    // Send PFC frames on all ports every 2.5ms to guarantee about 8 PFC frames
    // per collection interval.
    folly::FunctionScheduler sendPfcScheduler;
    sendPfcScheduler.addFunction(
        [&]() { sendPfcFrame(portIds, 0xFF); },
        std::chrono::microseconds(2500),
        "sendPfc");
    sendPfcScheduler.setSteady(true);

    // Start the PFC frame sending thread and the high frequency stats
    // collection job simultaneously.
    sendPfcScheduler.start();
    client->sync_startHighFrequencyStatsCollection(config);

    // Wait for 4 seconds to ensure that the high frequency stats collection job
    // is complete and stop both the threads.
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    client->sync_stopHighFrequencyStatsCollection();
    sendPfcScheduler.shutdown();

    // Get the high frequency stats.
    GetHighFrequencyStatsOptions options{};
    options.limit() = 1024;
    std::vector<HwHighFrequencyStats> timeseriesStats{};
    client->sync_getHighFrequencyStats(timeseriesStats, options);

    ASSERT_GE(timeseriesStats.size(), 1);

    // Verify that the PFC counters are non-zero for all the ports and all the
    // lossless PG IDs.
    for (const auto& [portId, portName] : port2Name) {
      auto it = timeseriesStats.back().portStats()->find(portName);
      ASSERT_NE(it, timeseriesStats.back().portStats()->end())
          << "No port stats found for port: " << portName;
      const std::map<int16_t, HwHighFrequencyPfcStats>& pfcStats{
          it->second.pfcStats().value()};
      ASSERT_GE(pfcStats.size(), losslessPgIds.size());
      for (int pgId : losslessPgIds) {
        auto pfcIt = pfcStats.find(pgId);
        ASSERT_NE(pfcIt, pfcStats.end());
        EXPECT_GE(pfcIt->second.inPfc().value(), 1);
      }
    }
  };
  verifyAcrossWarmBoots(setup, verify);
}

// TODO(T233514960): Add test to verify polling regularly with increasing
// timestamps
// TODO(T233514960): Add test to verify polling polling with an accurate
// frequency
// TODO(T233514960): Add test to verify polling stops automatically after the
// stats collection duration
// TODO(T233514960): Add test to verify stop API ends stats polling

} // namespace facebook::fboss
