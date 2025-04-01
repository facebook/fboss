// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <array>
#include <cstdint>
#include <functional>
#include <map>
#include <vector>

#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/packet/EthHdr.h"
#include "fboss/agent/packet/IPv6Hdr.h"
#include "fboss/agent/packet/PktFactory.h"
#include "fboss/agent/packet/TCPHeader.h"
#include "fboss/agent/test/AgentHwTest.h"
#include "fboss/agent/test/EcmpSetupHelper.h"
#include "fboss/agent/test/utils/AqmTestUtils.h"
#include "fboss/agent/test/utils/AsicUtils.h"
#include "fboss/agent/test/utils/ConfigUtils.h"
#include "fboss/agent/test/utils/CoppTestUtils.h"
#include "fboss/agent/test/utils/NetworkAITestUtils.h"
#include "fboss/agent/test/utils/OlympicTestUtils.h"
#include "fboss/agent/test/utils/PortStatsTestUtils.h"
#include "fboss/agent/test/utils/QosTestUtils.h"
#include "fboss/lib/CommonUtils.h"

namespace {
constexpr uint8_t kECT1{1}; // ECN capable transport(1), ECT(1)
constexpr uint8_t kECT0{2}; // ECN capable transport(0), ECT(0)
constexpr uint8_t kNotECT{0}; // Not ECN-Capable Transport, Not-ECT
constexpr auto kDefaultTxPayloadBytes{7000};

struct AqmTestStats {
  uint64_t wredDroppedPackets;
  uint64_t outEcnCounter;
  uint64_t outPackets;
};
} // namespace

namespace facebook::fboss {
class AgentAqmTest : public AgentHwTest {
 public:
  cfg::SwitchConfig initialConfig(
      const AgentEnsemble& ensemble) const override {
    cfg::SwitchConfig config = utility::onePortPerInterfaceConfig(
        ensemble.getSw(), ensemble.masterLogicalPortIds());
    if (ensemble.getHwAsicTable()->isFeatureSupportedOnAllAsic(
            HwAsic::Feature::L3_QOS)) {
      utility::addOlympicQosMaps(config, ensemble.getL3Asics());
    }
    utility::setTTLZeroCpuConfig(ensemble.getL3Asics(), config);
    return config;
  }

  std::vector<production_features::ProductionFeature>
  getProductionFeaturesVerified() const override {
    return {
        production_features::ProductionFeature::ECN,
        production_features::ProductionFeature::WRED};
  }

 protected:
  uint8_t kSilverDscp() const {
    return 0;
  }

  uint8_t kDscp() const {
    return 5;
  }

  folly::IPAddressV6 kSrcIp() const {
    return folly::IPAddressV6("2620:0:1cfe:face:b00c::3");
  }

  folly::IPAddressV6 kDestIp() const {
    return folly::IPAddressV6("2620:0:1cfe:face:b00c::4");
  }

  bool isEct(const uint8_t ecnVal) {
    return ecnVal != kNotECT;
  }

  folly::MacAddress getIntfMac() const {
    return utility::getMacForFirstInterfaceWithPorts(getProgrammedState());
  }

  void sendPkt(
      uint8_t dscpVal,
      const uint8_t ecnVal,
      int payloadLen = kDefaultTxPayloadBytes,
      int ttl = 255,
      std::optional<PortID> outPort = std::nullopt) {
    dscpVal = static_cast<uint8_t>(dscpVal << 2);
    dscpVal |= ecnVal;

    auto vlanId = utility::firstVlanIDWithPorts(getProgrammedState());
    auto intfMac = getIntfMac();
    auto srcMac = utility::MacAddressGenerator().get(intfMac.u64NBO() + 1);
    auto txPacket = utility::makeTCPTxPacket(
        getSw(),
        vlanId,
        srcMac,
        intfMac,
        kSrcIp(),
        kDestIp(),
        8000,
        8001,
        dscpVal,
        ttl,
        std::vector<uint8_t>(payloadLen, 0xff));

    if (outPort) {
      getSw()->sendPacketOutOfPortAsync(std::move(txPacket), *outPort);
    } else {
      getSw()->sendPacketSwitchedAsync(std::move(txPacket));
    }
  }

  /*
   * For congestion detection queue length of minLength = 128, and maxLength =
   * 128, a packet count of 128 has been enough to cause ECN marking. Inject
   * 128 * 2 packets to avoid test noise.
   */
  void sendPkts(
      uint8_t dscpVal,
      const uint8_t ecnVal,
      int cnt = 256,
      int payloadLen = kDefaultTxPayloadBytes,
      int ttl = 255,
      std::optional<PortID> outPort = std::nullopt) {
    for (int i = 0; i < cnt; i++) {
      sendPkt(dscpVal, ecnVal, payloadLen, ttl, outPort);
    }
  }

  cfg::SwitchConfig configureQueue2WithAqmThreshold(
      bool enableWred,
      bool enableEcn) const {
    auto config = utility::onePortPerInterfaceConfig(
        getSw(), masterLogicalPortIds(), true /*interfaceHasSubnet*/);
    if (getAgentEnsemble()->getHwAsicTable()->isFeatureSupportedOnAllAsic(
            HwAsic::Feature::L3_QOS)) {
      auto hwAsic =
          utility::checkSameAndGetAsic(getAgentEnsemble()->getL3Asics());
      auto streamType =
          *hwAsic->getQueueStreamTypes(cfg::PortType::INTERFACE_PORT).begin();
      if (isDualStage3Q2QQos()) {
        utility::addNetworkAIQueueConfig(
            &config,
            streamType,
            cfg::QueueScheduling::WEIGHTED_ROUND_ROBIN,
            hwAsic,
            enableWred,
            enableEcn);
      } else {
        utility::addOlympicQueueConfig(
            &config, getAgentEnsemble()->getL3Asics(), enableWred, enableEcn);
        if (hwAsic->getSwitchType() == cfg::SwitchType::VOQ) {
          utility::addVoqAqmConfig(
              &config, streamType, hwAsic, enableWred, enableEcn);
        }
      }
      utility::addOlympicQosMaps(config, getAgentEnsemble()->getL3Asics());
    }
    utility::setTTLZeroCpuConfig(getAgentEnsemble()->getL3Asics(), config);
    return config;
  }

  void setupEcmpTraffic() {
    utility::EcmpSetupTargetedPorts6 ecmpHelper{
        getProgrammedState(), getIntfMac()};
    const auto& portDesc = PortDescriptor(masterLogicalInterfacePortIds()[0]);
    applyNewState([&ecmpHelper, &portDesc](std::shared_ptr<SwitchState> in) {
      return ecmpHelper.resolveNextHops(in, {portDesc});
    });
    RoutePrefixV6 route{kDestIp(), 128};
    auto wrapper = getSw()->getRouteUpdater();
    ecmpHelper.programRoutes(&wrapper, {portDesc}, {route});
    utility::ttlDecrementHandlingForLoopbackTraffic(
        getAgentEnsemble(),
        ecmpHelper.getRouterId(),
        ecmpHelper.getNextHops()[0]);
  }

  /*
   * ECN/WRED traffic is always sent on specific queues, identified by queueId.
   * However, AQM stats are collected either from queue or from port depending
   * on test requirement, specified using useQueueStatsForAqm.
   */
  void extractAqmTestStats(
      const HwPortStats& portStats,
      const uint8_t queueId,
      bool useQueueStatsForAqm,
      AqmTestStats& stats) {
    if (useQueueStatsForAqm) {
      if (isSupportedOnAllAsics(HwAsic::Feature::QUEUE_ECN_COUNTER)) {
        stats.outEcnCounter +=
            portStats.queueEcnMarkedPackets_().value().find(queueId)->second;
      }
      stats.wredDroppedPackets +=
          portStats.queueWredDroppedPackets_().value().find(queueId)->second;
    } else {
      stats.outEcnCounter += folly::copy(portStats.outEcnCounter_().value());
      stats.wredDroppedPackets +=
          folly::copy(portStats.wredDroppedPackets_().value());
    }
    // Always populate outPackets
    stats.outPackets += utility::getPortOutPkts(portStats);
  }

  // For VoQ systems, WRED stat is collected from sysPorts and
  // outpackets is from egress.
  void extractAqmTestStats(
      const HwSysPortStats& sysPortStats,
      const HwPortStats& portStats,
      const uint8_t& queueId,
      AqmTestStats& stats) const {
    stats.wredDroppedPackets +=
        sysPortStats.queueWredDroppedPackets_().value().find(queueId)->second;
    stats.outPackets += portStats.queueOutPackets_().value().at(queueId);
  }

  template <typename StatsT>
  uint64_t extractQueueWatermarkStats(
      const StatsT& stats,
      const uint8_t& queueId) {
    return stats.get_queueWatermarkBytes_().find(queueId)->second;
  }

  /*
   * Collect stats which are needed for AQM tests. AQM specific stats
   * collected are as below:
   * WRED: Per queue available for all platforms, per port available for
   *       non-voq switches,
   * ECN : Per egress queue available for VoQ and TAJO platforms, per
   *       port available for non-voq switches.
   *
   * For non-voq switches, egress queue watermarks is a good indication
   * of peak queue usage, which can tell us if ECN marking / WRED should
   * have happened, however, for VoQ switches, queue watermarks are needed
   * from VoQs instead.
   */
  AqmTestStats getAqmTestStats(
      const uint8_t ecnVal,
      const PortID& portId,
      const uint8_t& queueId,
      const bool useQueueStatsForAqm) {
    AqmTestStats stats{};
    uint64_t queueWatermark{};
    const auto switchType =
        utility::checkSameAndGetAsic(getAgentEnsemble()->getL3Asics())
            ->getSwitchType();
    // Always collect port stats!
    auto portStats = getLatestPortStats(portId);
    if (isEct(ecnVal) || switchType != cfg::SwitchType::VOQ) {
      // Get ECNs marked packet stats for VoQ/non-voq switches and
      // watermarks for non-voq switches.
      extractAqmTestStats(portStats, queueId, useQueueStatsForAqm, stats);
      if (switchType != cfg::SwitchType::VOQ) {
        queueWatermark = extractQueueWatermarkStats(portStats, queueId);
      }
    }
    if (switchType == cfg::SwitchType::VOQ) {
      // Gets watermarks + WRED drops in case of non-ECN traffic and
      // watermarks for ECN traffic for VoQ switches.
      auto sysPortId = getSystemPortID(
          portId,
          getProgrammedState(),
          scopeResolver().scope(portId).switchId());
      auto sysPortStats = getLatestSysPortStats(sysPortId);
      extractAqmTestStats(sysPortStats, portStats, queueId, stats);
      queueWatermark = extractQueueWatermarkStats(sysPortStats, queueId);
    }
    XLOG(DBG0) << "Queue " << static_cast<int>(queueId)
               << ", watermark: " << queueWatermark
               << ", Out packets: " << stats.outPackets
               << ", WRED drops: " << stats.wredDroppedPackets
               << ", ECN marked: " << stats.outEcnCounter;
    return stats;
  }

  // The enableWred/ enableEcn params are used to specify the test should
  // use WRED/ECN config or not. This helps validate functionality in AI
  // network which uses ECN alone, the case with both ECN and WRED as in
  // front end network or with just WRED.
  void runTest(const uint8_t ecnVal, bool enableWred, bool enableEcn) {
    if (!isSupportedOnAllAsics(HwAsic::Feature::L3_QOS)) {
      GTEST_SKIP();
      return;
    }

    auto kQueueId = utility::getOlympicQueueId(utility::OlympicQueueType::ECN1);
    // For VoQ switch, AQM stats are collected from queue!
    auto useQueueStatsForAqm =
        utility::checkSameAndGetAsic(getAgentEnsemble()->getL3Asics())
            ->getSwitchType() == cfg::SwitchType::VOQ;
    auto statsIncremented = [this](
                                const AqmTestStats& aqmStats, uint8_t ecnVal) {
      auto increment =
          isEct(ecnVal) ? aqmStats.outEcnCounter : aqmStats.wredDroppedPackets;
      return increment > 0;
    };

    auto setup = [&]() {
      applyNewConfig(configureQueue2WithAqmThreshold(enableWred, enableEcn));
      setupEcmpTraffic();
      if (isEct(ecnVal)) {
        sendPkt(kDscp(), ecnVal);
        WITH_RETRIES({
          auto aqmStats = getAqmTestStats(
              ecnVal,
              masterLogicalInterfacePortIds()[0],
              kQueueId,
              useQueueStatsForAqm);
          // Assert that ECT capable packets are not counted by port ECN
          // counter when there is no congestion!
          EXPECT_EVENTUALLY_FALSE(statsIncremented(aqmStats, ecnVal));
        });
      }
    };

    auto verify = [&]() {
      const int kNumPacketsToSend = getAgentEnsemble()->getMinPktsForLineRate(
          masterLogicalInterfacePortIds()[0]);
      sendPkts(kDscp(), ecnVal, kNumPacketsToSend);
      /*
       * Need traffic loop to build up for ECN/WRED to show up for some
       * platforms. However, we cannot expect traffic to reach line rate
       * in WRED config cases. There can also be a delay before stats are
       * synced. So, add enough retries to avoid flakiness.
       */
      WITH_RETRIES_N_TIMED(10, std::chrono::milliseconds(1000), {
        auto aqmStats = getAqmTestStats(
            ecnVal,
            masterLogicalInterfacePortIds()[0],
            kQueueId,
            useQueueStatsForAqm);
        EXPECT_EVENTUALLY_TRUE(statsIncremented(aqmStats, ecnVal));
      });
    };

    verifyAcrossWarmBoots(setup, verify);
  }

  void queueWredThresholdSetup(
      cfg::SwitchConfig& config,
      std::span<const int> queueIds) {
    auto asic = utility::checkSameAndGetAsic(getAgentEnsemble()->getL3Asics());
    bool isVoq = asic->getSwitchType() == cfg::SwitchType::VOQ;
    for (int queueId : queueIds) {
      utility::addQueueWredConfig(
          config,
          getAgentEnsemble()->getL3Asics(),
          queueId,
          utility::kQueueConfigAqmsWredThresholdMinMax,
          utility::kQueueConfigAqmsWredThresholdMinMax,
          utility::kQueueConfigAqmsWredDropProbability,
          isVoq);
    }
  }

  void queueEcnThresholdSetup(
      cfg::SwitchConfig& config,
      std::span<const int> queueIds) {
    auto asic = utility::checkSameAndGetAsic(getAgentEnsemble()->getL3Asics());
    bool isVoq = asic->getSwitchType() == cfg::SwitchType::VOQ;
    for (int queueId : queueIds) {
      utility::addQueueEcnConfig(
          config,
          getAgentEnsemble()->getL3Asics(),
          queueId,
          utility::kQueueConfigAqmsEcnThresholdMinMax,
          utility::kQueueConfigAqmsEcnThresholdMinMax,
          isVoq);
    }
  }

  /*
   * This function validates the ECN/WRED thresholds configured on a queue.
   * If more AQM configurations are added, this function needs to reassess
   * whether thresholds are supported and the logic is applicable.
   */
  void validateAqmThresholds(
      const uint8_t ecnCodePoint,
      int thresholdBytes,
      int expectedMarkedOrDroppedPacketCount,
      const std::optional<
          std::function<void(AqmTestStats&, AqmTestStats&, int)>>&
          verifyPacketCountFn = std::nullopt,
      const std::optional<std::function<
          void(cfg::SwitchConfig&, std::vector<int>, const int txPacketLen)>>&
          setupFn = std::nullopt,
      int maxQueueFillLevel = 0) {
    [[maybe_unused]] int kQueueId =
        utility::getOlympicQueueId(utility::OlympicQueueType::SILVER);
    // Good to keep the payload size such that the whole packet with headers
    // can fit in a single buffer in ASIC to keep computation simple and
    // accurate.
    constexpr int kPayloadLength{30};
    const int kTxPacketLen =
        kPayloadLength + EthHdr::SIZE + IPv6Hdr::size() + TCPHeader::size();
    const std::vector<const HwAsic*> asics{getAgentEnsemble()->getL3Asics()};
    auto asic = utility::checkSameAndGetAsic(asics);
    // The ECN/WRED threshold are rounded down for TAJO as opposed to being
    // rounded up to the next cell size for Broadcom.
    bool roundUp = asic->getAsicType() != cfg::AsicType::ASIC_TYPE_EBRO;

    if (expectedMarkedOrDroppedPacketCount == 0 && maxQueueFillLevel > 0) {
      // The expectedMarkedOrDroppedPacketCount is not set, instead, it needs
      // to be computed based on the maxQueueFillLevel specified as param!
      expectedMarkedOrDroppedPacketCount =
          (maxQueueFillLevel -
           utility::getRoundedBufferThreshold(asic, thresholdBytes, roundUp)) /
          utility::getEffectiveBytesPerPacket(asic, kTxPacketLen);
    }

    // Send enough packets such that the queue gets filled up to the
    // configured ECN/WRED threshold, then send a fixed number of
    // additional packets to get marked / dropped.
    auto ceilFn = [](int a, int b) -> int { return a / b + (a % b != 0); };
    [[maybe_unused]] int numPacketsToSend =
        ceilFn(
            utility::getRoundedBufferThreshold(asic, thresholdBytes, roundUp),
            utility::getEffectiveBytesPerPacket(asic, kTxPacketLen)) +
        expectedMarkedOrDroppedPacketCount;

    auto setup = [&]() {
      cfg::SwitchConfig config{initialConfig(*getAgentEnsemble())};
      utility::addOlympicQueueConfig(&config, asics);
      // Configure both WRED and ECN thresholds
      queueEcnThresholdSetup(config, std::array{kQueueId});
      queueWredThresholdSetup(config, std::array{kQueueId});
      // Include any config setup needed per test case
      if (setupFn.has_value()) {
        (*setupFn)(config, {kQueueId}, kTxPacketLen);
      }
      applyNewConfig(config);

      // No traffic loop needed, so send traffic to a different MAC
      int kEcmpWidthForTest{1};
      utility::EcmpSetupAnyNPorts6 ecmpHelper6{
          getProgrammedState(),
          utility::MacAddressGenerator().get(getIntfMac().u64NBO() + 10)};
      resolveNeighborAndProgramRoutes(ecmpHelper6, kEcmpWidthForTest);
    };

    auto verify = [&]() {
      XLOG(DBG3) << "Rounded threshold: "
                 << utility::getRoundedBufferThreshold(
                        asic, thresholdBytes, roundUp)
                 << ", effective bytes per pkt: "
                 << utility::getEffectiveBytesPerPacket(asic, kTxPacketLen)
                 << ", kTxPacketLen: " << kTxPacketLen
                 << ", pkts to send: " << numPacketsToSend
                 << ", expected marked/dropped pkts: "
                 << expectedMarkedOrDroppedPacketCount;

      auto sendPackets = [&](const PortID& /* port */, int numPacketsToSend) {
        // Single port config, traffic gets forwarded out of the same!
        sendPkts(
            utility::kOlympicQueueToDscp().at(kQueueId).front(),
            ecnCodePoint,
            numPacketsToSend,
            kPayloadLength,
            /*ttl=*/255,
            masterLogicalInterfacePortIds()[1]);
      };

      // Send traffic with queue buildup and get the stats at the start!
      HwPortStats beforePortStats = utility::sendPacketsWithQueueBuildup(
          sendPackets,
          getAgentEnsemble(),
          masterLogicalInterfacePortIds()[0],
          numPacketsToSend);

      AqmTestStats before{};
      extractAqmTestStats(
          beforePortStats, kQueueId, false /*useQueueStatsForAqm*/, before);

      // For ECN all packets are sent out, for WRED, account for drops!
      [[maybe_unused]] const uint64_t kExpectedOutPackets = isEct(ecnCodePoint)
          ? numPacketsToSend
          : numPacketsToSend - expectedMarkedOrDroppedPacketCount;
    };

    verifyAcrossWarmBoots(setup, verify);
  }

  void runPerQueueWredDropStatsTest() {
    const std::array<int, 3> wredQueueIds = {
        utility::getOlympicQueueId(utility::OlympicQueueType::SILVER),
        utility::getOlympicQueueId(utility::OlympicQueueType::GOLD),
        utility::getOlympicQueueId(utility::OlympicQueueType::ECN1)};

    auto setup = [&]() {
      const AgentEnsemble& ensemble = *getAgentEnsemble();
      const std::vector<const HwAsic*> asics = ensemble.getL3Asics();
      cfg::SwitchConfig config = initialConfig(ensemble);
      if (ensemble.getHwAsicTable()->isFeatureSupportedOnAllAsic(
              HwAsic::Feature::L3_QOS)) {
        utility::addOlympicQueueConfig(&config, asics);
      }
      bool isVoq = utility::checkSameAndGetAsic(asics)->getSwitchType() ==
          cfg::SwitchType::VOQ;
      for (int queueId : wredQueueIds) {
        utility::addQueueWredConfig(
            config,
            asics,
            queueId,
            utility::kQueueConfigAqmsWredThresholdMinMax,
            utility::kQueueConfigAqmsWredThresholdMinMax,
            utility::kQueueConfigAqmsWredDropProbability,
            isVoq);
      }
      applyNewConfig(config);
      setupEcmpTraffic();
    };

    auto verify = [&]() {
      // Using delta stats in this function, so get the stats before starting
      const HwPortStats beforeStats =
          getLatestPortStats(masterLogicalInterfacePortIds()[0]);
      const std::map<int16_t, int64_t>& beforeDroppedPacketsMap =
          *beforeStats.queueWredDroppedPackets_();

      // Send traffic to all queues
      constexpr int kNumPacketsToSend{1000};
      for (int queueId : wredQueueIds) {
        sendPkts(
            utility::kOlympicQueueToDscp().at(queueId).front(),
            false,
            kNumPacketsToSend);
      }

      WITH_RETRIES({
        // Current map of queue to WRED dropped packets
        const std::map<std::int16_t, std::int64_t> droppedPacketsMap =
            *getLatestPortStats(masterLogicalPortIds())
                 .find(masterLogicalInterfacePortIds()[0])
                 ->second.queueWredDroppedPackets_();
        for (int queueId : wredQueueIds) {
          int64_t wredDrops = droppedPacketsMap.find(queueId)->second -
              beforeDroppedPacketsMap.find(queueId)->second;
          XLOG(DBG3) << "Queue : " << queueId << ", wredDrops : " << wredDrops;
          EXPECT_EVENTUALLY_GT(wredDrops, 0);
        }
      });
    };

    verifyAcrossWarmBoots(setup, verify);
  }
};

class AgentAqmWredDropTest : public AgentAqmTest {
 public:
  cfg::SwitchConfig initialConfig(
      const AgentEnsemble& ensemble) const override {
    cfg::SwitchConfig config = AgentAqmTest::initialConfig(ensemble);
    if (ensemble.getHwAsicTable()->isFeatureSupportedOnAllAsic(
            HwAsic::Feature::L3_QOS)) {
      cfg::StreamType streamType =
          *utility::checkSameAndGetAsic(ensemble.getL3Asics())
               ->getQueueStreamTypes(cfg::PortType::INTERFACE_PORT)
               .begin();
      utility::addQueueWredDropConfig(
          &config, streamType, ensemble.getL3Asics());
      // For VoQ switches, add AQM config to VoQ as well.
      auto asic = utility::checkSameAndGetAsic(ensemble.getL3Asics());
      if (asic->getSwitchType() == cfg::SwitchType::VOQ) {
        utility::addVoqAqmConfig(
            &config,
            streamType,
            asic,
            true /*addWredConfig*/,
            false /*addEcnConfig*/);
      }
    }
    return config;
  }

  void runWredDropTest() {
    auto setup = [=, this]() { setupEcmpTraffic(); };
    auto verify = [=, this]() {
      // Send packets to queue0 and queue2 (both configured to the same
      // weight). Queue0 is configured with 0% drop probability and queue2 is
      // configured with 5% drop probability.
      sendPkts(kDscp(), false);
      sendPkts(kSilverDscp(), false);

      auto getQueueWatermarkBytes = [&](const auto& portStats, int queueId) {
        auto queueWatermark =
            portStats.get_queueWatermarkBytes_().find(queueId)->second;
        XLOG(DBG0) << "Queue " << queueId << " watermark : " << queueWatermark;
        return queueWatermark;
      };

      constexpr auto queue2Id = 2;
      constexpr auto queue0Id = 0;
      uint64_t queue0Watermark{0};
      uint64_t queue2Watermark{0};
      WITH_RETRIES({
        auto portStats = getLatestPortStats(masterLogicalInterfacePortIds()[0]);
        queue0Watermark = getQueueWatermarkBytes(portStats, queue0Id);
        queue2Watermark = getQueueWatermarkBytes(portStats, queue2Id);
        EXPECT_EVENTUALLY_GT(queue0Watermark, 0);
        EXPECT_EVENTUALLY_GT(queue2Watermark, 0);
      });

      // Queue0 watermark should be higher than queue2 since it drops less
      // packets.
      XLOG(DBG0) << "Expecting queue 0 watermark " << queue0Watermark
                 << " larger than queue 2 watermark : " << queue2Watermark;
      EXPECT_TRUE(queue0Watermark > queue2Watermark);
    };

    verifyAcrossWarmBoots(setup, verify);
  }
};

TEST_F(AgentAqmTest, verifyEct0) {
  runTest(kECT0, true /* enableWred */, true /* enableEcn */);
}

TEST_F(AgentAqmTest, verifyEct1) {
  runTest(kECT1, true /* enableWred */, true /* enableEcn */);
}

TEST_F(AgentAqmTest, verifyEcnWithoutWredConfig) {
  runTest(kECT1, false /* enableWred */, true /* enableEcn */);
}

TEST_F(AgentAqmTest, verifyWredWithoutEcnConfig) {
  runTest(kNotECT, true /* enableWred */, false /* enableEcn */);
}

TEST_F(AgentAqmTest, verifyWred) {
  runTest(kNotECT, true /* enableWred */, true /* enableEcn */);
}

TEST_F(AgentAqmWredDropTest, verifyWredDrop) {
  runWredDropTest();
}

TEST_F(AgentAqmTest, verifyPerQueueWredDropStats) {
  runPerQueueWredDropStatsTest();
}

} // namespace facebook::fboss
