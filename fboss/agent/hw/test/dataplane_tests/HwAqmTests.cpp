/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include <fboss/agent/hw/switch_asics/HwAsic.h>
#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/hw/test/HwLinkStateDependentTest.h"
#include "fboss/agent/hw/test/HwTestPacketUtils.h"
#include "fboss/agent/hw/test/HwTestPortUtils.h"
#include "fboss/agent/hw/test/TrafficPolicyUtils.h"
#include "fboss/agent/hw/test/dataplane_tests/HwTestAqmUtils.h"
#include "fboss/agent/hw/test/dataplane_tests/HwTestOlympicUtils.h"
#include "fboss/agent/hw/test/dataplane_tests/HwTestQosUtils.h"
#include "fboss/agent/packet/EthHdr.h"
#include "fboss/agent/packet/IPv6Hdr.h"
#include "fboss/agent/packet/TCPHeader.h"
#include "fboss/agent/test/EcmpSetupHelper.h"
#include "fboss/agent/test/ResourceLibUtil.h"

#include "fboss/lib/CommonUtils.h"

#include <folly/IPAddress.h>
#include <optional>

namespace {
struct AqmTestStats {
  uint64_t wredDroppedPackets;
  uint64_t outEcnCounter;
};

/*
 * Ensure that the number of dropped packets is as expected. Allow for
 * an error to account for more / less drops while its worked out.
 */
void verifyWredDroppedPacketCount(
    facebook::fboss::HwPortStats& after,
    facebook::fboss::HwPortStats& before,
    int expectedDroppedPkts) {
  constexpr auto kAcceptableError = 2;
  auto deltaWredDroppedPackets =
      *after.wredDroppedPackets_() - *before.wredDroppedPackets_();
  XLOG(DBG0) << "Delta WRED dropped pkts: " << deltaWredDroppedPackets;
  EXPECT_GT(deltaWredDroppedPackets, expectedDroppedPkts - kAcceptableError);
  EXPECT_LT(deltaWredDroppedPackets, expectedDroppedPkts + kAcceptableError);
}

/*
 * Due to the way packet marking happens, we might not have an accurate count,
 * but the number of packets marked will be >= to the expected marked packet
 * count.
 */
void verifyEcnMarkedPacketCount(
    facebook::fboss::HwPortStats& after,
    facebook::fboss::HwPortStats& before,
    int expectedMarkedPkts) {
  auto deltaEcnMarkedPackets =
      *after.outEcnCounter_() - *before.outEcnCounter_();
  auto deltaOutPackets = getPortOutPkts(after) - getPortOutPkts(before);
  XLOG(DBG0) << "Delta ECN marked pkts: " << deltaEcnMarkedPackets
             << ", delta out packets: " << deltaOutPackets;
  EXPECT_TRUE(deltaEcnMarkedPackets >= expectedMarkedPkts);
  EXPECT_GT(deltaOutPackets, deltaEcnMarkedPackets);
}
} // namespace

namespace facebook::fboss {

class HwAqmTest : public HwLinkStateDependentTest {
 private:
  static constexpr auto kDefaultTxPayloadBytes{7000};

  cfg::SwitchConfig initialConfig() const override {
    auto cfg = utility::onePortPerInterfaceConfig(
        getHwSwitch(),
        masterLogicalPortIds(),
        getAsic()->desiredLoopbackMode());
    if (isSupported(HwAsic::Feature::L3_QOS)) {
      auto streamType =
          *(getPlatform()
                ->getAsic()
                ->getQueueStreamTypes(cfg::PortType::INTERFACE_PORT)
                .begin());
      utility::addOlympicQueueConfig(
          &cfg, streamType, getPlatform()->getAsic());
      utility::addOlympicQosMaps(cfg);
    }
    return cfg;
  }

  cfg::SwitchConfig wredDropConfig() const {
    auto cfg = utility::onePortPerInterfaceConfig(
        getHwSwitch(),
        masterLogicalPortIds(),
        getAsic()->desiredLoopbackMode());
    if (isSupported(HwAsic::Feature::L3_QOS)) {
      auto streamType =
          *(getPlatform()
                ->getAsic()
                ->getQueueStreamTypes(cfg::PortType::INTERFACE_PORT)
                .begin());
      utility::addQueueWredDropConfig(
          &cfg, streamType, getPlatform()->getAsic());
      utility::addOlympicQosMaps(cfg);
    }
    return cfg;
  }

  cfg::SwitchConfig configureQueue2WithWredThreshold() const {
    auto cfg = utility::onePortPerInterfaceConfig(
        getHwSwitch(),
        masterLogicalPortIds(),
        getAsic()->desiredLoopbackMode());
    if (isSupported(HwAsic::Feature::L3_QOS)) {
      auto streamType =
          *(getPlatform()
                ->getAsic()
                ->getQueueStreamTypes(cfg::PortType::INTERFACE_PORT)
                .begin());
      utility::addOlympicQueueConfig(
          &cfg, streamType, getPlatform()->getAsic(), true /*add wred*/);
      utility::addOlympicQosMaps(cfg);
    }
    return cfg;
  }

  cfg::SwitchConfig multiplePortConfig() const {
    auto cfg = utility::multiplePortsPerIntfConfig(
        getHwSwitch(),
        masterLogicalPortIds(),
        getAsic()->desiredLoopbackMode());
    if (isSupported(HwAsic::Feature::L3_QOS)) {
      auto streamType =
          *(getPlatform()
                ->getAsic()
                ->getQueueStreamTypes(cfg::PortType::INTERFACE_PORT)
                .begin());
      utility::addOlympicQueueConfig(
          &cfg, streamType, getPlatform()->getAsic());
      utility::addOlympicQosMaps(cfg);
    }
    return cfg;
  }

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

  void sendPkt(
      uint8_t dscpVal,
      bool isEcn,
      bool ensure = false,
      int payloadLen = kDefaultTxPayloadBytes,
      int ttl = 255,
      std::optional<PortID> outPort = std::nullopt) {
    auto kECT1 = 0x01; // ECN capable transport ECT(1)

    dscpVal = static_cast<uint8_t>(dscpVal << 2);
    if (isEcn) {
      dscpVal |= kECT1;
    }

    auto vlanId = utility::firstVlanID(initialConfig());
    auto intfMac = getIntfMac();
    auto srcMac = utility::MacAddressGenerator().get(intfMac.u64NBO() + 1);
    auto txPacket = utility::makeTCPTxPacket(
        getHwSwitch(),
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
      getHwSwitch()->sendPacketOutOfPortSync(std::move(txPacket), *outPort);
    } else {
      if (ensure) {
        getHwSwitchEnsemble()->ensureSendPacketSwitched(std::move(txPacket));
      } else {
        getHwSwitch()->sendPacketSwitchedSync(std::move(txPacket));
      }
    }
  }

  /*
   * For congestion detection queue length of minLength = 128, and maxLength =
   * 128, a packet count of 128 has been enough to cause ECN marking. Inject
   * 128 * 2 packets to avoid test noise.
   */
  void sendPkts(
      uint8_t dscpVal,
      bool isEcn,
      int cnt = 256,
      int payloadLen = kDefaultTxPayloadBytes,
      int ttl = 255,
      std::optional<PortID> outPort = std::nullopt) {
    for (int i = 0; i < cnt; i++) {
      sendPkt(dscpVal, isEcn, false /* ensure */, payloadLen, ttl, outPort);
    }
  }
  folly::MacAddress getIntfMac() const {
    return utility::getFirstInterfaceMac(getProgrammedState());
  }

  void queueShaperAndBurstSetup(
      std::vector<int> queueIds,
      cfg::SwitchConfig& config,
      uint32_t minKbps,
      uint32_t maxKbps,
      uint32_t minBurstKb,
      uint32_t maxBurstKb) {
    for (auto queueId : queueIds) {
      utility::addQueueShaperConfig(&config, queueId, minKbps, maxKbps);
      utility::addQueueBurstSizeConfig(
          &config, queueId, minBurstKb, maxBurstKb);
    }
  }

  void queueEcnWredThresholdSetup(
      bool isEcn,
      std::vector<int> queueIds,
      cfg::SwitchConfig& cfg) {
    for (auto queueId : queueIds) {
      if (isEcn) {
        utility::addQueueEcnConfig(
            &cfg,
            queueId,
            utility::kQueueConfigAqmsEcnThresholdMinMax,
            utility::kQueueConfigAqmsEcnThresholdMinMax);
      } else {
        utility::addQueueWredConfig(
            &cfg,
            queueId,
            utility::kQueueConfigAqmsWredThresholdMinMax,
            utility::kQueueConfigAqmsWredThresholdMinMax,
            utility::kQueueConfigAqmsWredDropProbability);
      }
    }
  }

  void disableTTLDecrements(
      const utility::EcmpSetupTargetedPorts6& ecmpHelper) {
    utility::ttlDecrementHandlingForLoopbackTraffic(
        getHwSwitch(),
        ecmpHelper.getRouterId(),
        ecmpHelper.nhop(PortDescriptor(masterLogicalInterfacePortIds()[0])));
  }

  void setupEcmpTraffic() {
    utility::EcmpSetupTargetedPorts6 ecmpHelper{
        getProgrammedState(), getIntfMac()};
    const auto& portDesc = PortDescriptor(masterLogicalInterfacePortIds()[0]);
    applyNewState(ecmpHelper.resolveNextHops(getProgrammedState(), {portDesc}));
    RoutePrefixV6 route{kDestIp(), 128};
    ecmpHelper.programRoutes(getRouteUpdater(), {portDesc}, {route});
    disableTTLDecrements(ecmpHelper);
  }

 protected:
  /*
   * ECN/WRED traffic is always sent on specific queues, identified by queueId.
   * However, AQM stats are collected either from queue or from port depending
   * on test requirement, specified using useQueueStatsForAqm.
   */
  AqmTestStats extractAqmTestStats(
      const HwPortStats& portStats,
      const uint8_t queueId,
      bool useQueueStatsForAqm) {
    AqmTestStats stats{};
    if (useQueueStatsForAqm) {
      if (getPlatform()->getAsic()->isSupported(
              HwAsic::Feature::QUEUE_ECN_COUNTER)) {
        stats.outEcnCounter =
            portStats.get_queueEcnMarkedPackets_().find(queueId)->second;
      }
      stats.wredDroppedPackets =
          portStats.get_queueWredDroppedPackets_().find(queueId)->second;
    } else {
      stats.outEcnCounter = portStats.get_outEcnCounter_();
      stats.wredDroppedPackets = portStats.get_wredDroppedPackets_();
    }
    return stats;
  }

  // For VoQ, all stats are collected per queue.
  AqmTestStats extractAqmTestStats(
      const HwSysPortStats& portStats,
      const uint8_t& queueId) {
    AqmTestStats stats{};
    stats.wredDroppedPackets =
        portStats.get_queueWredDroppedPackets_().find(queueId)->second;
    return stats;
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
      const bool isEcn,
      const PortID& portId,
      const uint8_t& queueId,
      const bool useQueueStatsForAqm) {
    AqmTestStats stats{};
    uint64_t queueWatermark{};
    if (getPlatform()->getAsic()->getSwitchType() == cfg::SwitchType::VOQ) {
      // Gets watermarks + WRED drops in case of non-ECN traffic and
      // watermarks for ECN traffic for VoQ switches.
      auto sysPortId = getSystemPortID(portId, getProgrammedState());
      auto sysPortStats =
          getHwSwitchEnsemble()->getLatestSysPortStats(sysPortId);
      stats = extractAqmTestStats(sysPortStats, queueId);
      queueWatermark = extractQueueWatermarkStats(sysPortStats, queueId);
    }
    if (isEcn ||
        getPlatform()->getAsic()->getSwitchType() != cfg::SwitchType::VOQ) {
      // Get ECNs marked packet stats for VoQ/non-voq switches and
      // watermarks for non-voq switches.
      auto portStats = getHwSwitchEnsemble()->getLatestPortStats(portId);
      stats = extractAqmTestStats(portStats, queueId, useQueueStatsForAqm);
      if (getPlatform()->getAsic()->getSwitchType() != cfg::SwitchType::VOQ) {
        queueWatermark = extractQueueWatermarkStats(portStats, queueId);
      }
    }
    XLOG(DBG0) << "Queue " << static_cast<int>(queueId)
               << ", watermark: " << queueWatermark
               << ", WRED drops: " << stats.wredDroppedPackets
               << ", ECN marked: " << stats.outEcnCounter;
    return stats;
  }

  void runTest(bool isEcn) {
    if (!isSupported(HwAsic::Feature::L3_QOS)) {
#if defined(GTEST_SKIP)
      GTEST_SKIP();
#endif
      return;
    }

    constexpr auto kQueueId{utility::kOlympicEcn1QueueId};
    // For VoQ switch, AQM stats are collected from queue!
    auto useQueueStatsForAqm =
        getPlatform()->getAsic()->getSwitchType() == cfg::SwitchType::VOQ;
    auto statsIncremented = [](const AqmTestStats& aqmStats, bool isEcn) {
      auto increment =
          isEcn ? aqmStats.outEcnCounter : aqmStats.wredDroppedPackets;
      return increment > 0;
    };

    auto setup = [&]() {
      applyNewConfig(configureQueue2WithWredThreshold());
      setupEcmpTraffic();
      if (isEcn) {
        sendPkt(kDscp(), isEcn, true);
        auto aqmStats = getAqmTestStats(
            isEcn,
            masterLogicalInterfacePortIds()[0],
            kQueueId,
            useQueueStatsForAqm);
        // Assert that ECT capable packets are not counted by port ECN
        // counter when there is no congestion!
        EXPECT_FALSE(statsIncremented(aqmStats, isEcn));
      }
    };

    auto verify = [&]() {
      const int kNumPacketsToSend =
          getHwSwitchEnsemble()->getMinPktsForLineRate(
              masterLogicalInterfacePortIds()[0]);
      sendPkts(kDscp(), isEcn, kNumPacketsToSend);
      /*
       * Need traffic loop to build up for ECN/WRED to show up for some
       * platforms. However, we cannot expect traffic to reach line rate
       * in WRED config cases. There can also be a delay before stats are
       * synced. So, add enough retries to avoid flakiness.
       */
      WITH_RETRIES_N_TIMED(10, std::chrono::milliseconds(1000), {
        auto aqmStats = getAqmTestStats(
            isEcn,
            masterLogicalInterfacePortIds()[0],
            kQueueId,
            useQueueStatsForAqm);
        EXPECT_EVENTUALLY_TRUE(statsIncremented(aqmStats, isEcn));
      });
    };

    verifyAcrossWarmBoots(setup, verify);
  }

  void runWredDropTest() {
    if (!isSupported(HwAsic::Feature::L3_QOS)) {
#if defined(GTEST_SKIP)
      GTEST_SKIP();
#endif
      return;
    }
    auto setup = [=]() {
      applyNewConfig(wredDropConfig());
      setupEcmpTraffic();
    };
    auto verify = [=]() {
      // Send packets to queue0 and queue2 (both configured to the same weight).
      // Queue0 is configured with 0% drop probability and queue2 is configured
      // with 5% drop probability.
      sendPkts(kDscp(), false);
      sendPkts(kSilverDscp(), false);

      // Avoid flakiness
      sleep(1);

      constexpr auto queue2Id = 2;
      constexpr auto queue0Id = 0;

      // Verify queue2 watermark being updated.
      auto queueId = queue2Id;

      auto countIncremented = [&](const auto& newStats) {
        auto portStatsIter = newStats.find(masterLogicalInterfacePortIds()[0]);
        auto queueWatermark = portStatsIter->second.get_queueWatermarkBytes_()
                                  .find(queueId)
                                  ->second;
        XLOG(DBG0) << "Queue " << queueId << " watermark : " << queueWatermark;
        return queueWatermark > 0;
      };

      EXPECT_TRUE(
          getHwSwitchEnsemble()->waitPortStatsCondition(countIncremented));

      // Verify queue0 watermark being updated.
      queueId = queue0Id;
      EXPECT_TRUE(
          getHwSwitchEnsemble()->waitPortStatsCondition(countIncremented));

      auto watermarkBytes =
          getHwSwitchEnsemble()
              ->getLatestPortStats(masterLogicalInterfacePortIds()[0])
              .get_queueWatermarkBytes_();

      // Queue0 watermark should be higher than queue2 since it drops less
      // packets.
      auto queue0Watermark = watermarkBytes.find(queue0Id)->second;
      auto queue2Watermark = watermarkBytes.find(queue2Id)->second;

      XLOG(DBG0) << "Expecting queue 0 watermark " << queue0Watermark
                 << " larger than queue 2 watermark : " << queue2Watermark;
      EXPECT_TRUE(queue0Watermark > queue2Watermark);
    };

    verifyAcrossWarmBoots(setup, verify);
  }

  void waitForExpectedThresholdTestStats(
      bool isEcn,
      PortID port,
      const int queueId,
      const uint64_t expectedOutPkts,
      const uint64_t expectedDropPkts,
      HwPortStats& before) {
    auto waitForExpectedOutPackets = [&](const auto& newStats) {
      uint64_t outPackets{0}, wredDrops{0}, ecnMarking{0};
      auto portStatsIter = newStats.find(port);
      if (portStatsIter != newStats.end()) {
        auto portStats = portStatsIter->second;
        outPackets = (*portStats.queueOutPackets_())[queueId] -
            (*before.queueOutPackets_())[queueId];
        if (isEcn) {
          ecnMarking = *portStats.outEcnCounter_() - *before.outEcnCounter_();
        } else {
          wredDrops =
              *portStats.wredDroppedPackets_() - *before.wredDroppedPackets_();
        }
      }
      /*
       * Check for outpackets as expected and ensure that ECN or WRED
       * counters are incrementing too! In case of WRED, make sure that
       * dropped packets + out packets >= expected drop + out packets.
       */
      return (outPackets >= expectedOutPkts) &&
          ((isEcn && ecnMarking) ||
           (!isEcn &&
            (wredDrops + outPackets) >= (expectedOutPkts + expectedDropPkts)));
    };

    constexpr auto kNumRetries{5};
    getHwSwitchEnsemble()->waitPortStatsCondition(
        waitForExpectedOutPackets,
        kNumRetries,
        std::chrono::milliseconds(std::chrono::seconds(1)));
  }

  void validateEcnWredThresholds(
      bool isEcn,
      int thresholdBytes,
      int markedOrDroppedPacketCount,
      std::optional<std::function<void(HwPortStats&, HwPortStats&, int)>>
          verifyPacketCountFn = std::nullopt,
      std::optional<std::function<
          void(cfg::SwitchConfig&, std::vector<int>, const int txPacketLen)>>
          setupFn = std::nullopt,
      int maxQueueFillLevel = 0) {
    constexpr auto kQueueId{0};
    /*
     * Good to keep the payload size such that the whole packet with
     * headers can fit in a single buffer in ASIC to keep computation
     * simple and accurate.
     */
    constexpr auto kPayloadLength{30};
    const int kTxPacketLen =
        kPayloadLength + EthHdr::SIZE + IPv6Hdr::size() + TCPHeader::size();
    /*
     * The ECN/WRED threshold are rounded down for TAJO as opposed to
     * being rounded up to the next cell size for Broadcom.
     */
    bool roundUp = getAsic()->getAsicType() == cfg::AsicType::ASIC_TYPE_EBRO
        ? false
        : true;

    if (!markedOrDroppedPacketCount && maxQueueFillLevel) {
      /*
       * The markedOrDroppedPacketCount is not set, instead, it needs to be
       * computed based on the maxQueueFillLevel specified as param!
       */
      markedOrDroppedPacketCount =
          (maxQueueFillLevel -
           utility::getRoundedBufferThreshold(
               getHwSwitch(), thresholdBytes, roundUp)) /
          utility::getEffectiveBytesPerPacket(getHwSwitch(), kTxPacketLen);
    }

    /*
     * Send enough packets such that the queue gets filled up to the
     * configured ECN/WRED threshold, then send a fixed number of
     * additional packets to get marked / dropped.
     */
    int numPacketsToSend =
        ceil(
            (double)utility::getRoundedBufferThreshold(
                getHwSwitch(), thresholdBytes, roundUp) /
            utility::getEffectiveBytesPerPacket(getHwSwitch(), kTxPacketLen)) +
        markedOrDroppedPacketCount;
    XLOG(DBG3) << "Rounded threshold: "
               << utility::getRoundedBufferThreshold(
                      getHwSwitch(), thresholdBytes, roundUp)
               << ", effective bytes per pkt: "
               << utility::getEffectiveBytesPerPacket(
                      getHwSwitch(), kTxPacketLen)
               << ", kTxPacketLen: " << kTxPacketLen
               << ", pkts to send: " << numPacketsToSend;

    auto setup = [=]() {
      auto config{initialConfig()};
      queueEcnWredThresholdSetup(isEcn, {kQueueId}, config);
      queueEcnWredThresholdSetup(!isEcn, {kQueueId}, config);
      // Include any config setup needed per test case
      if (setupFn.has_value()) {
        (*setupFn)(config, {kQueueId}, kTxPacketLen);
      }
      applyNewConfig(config);

      // No traffic loop needed, so send traffic to a different MAC
      auto kEcmpWidthForTest = 1;
      utility::EcmpSetupAnyNPorts6 ecmpHelper6{
          getProgrammedState(),
          utility::MacAddressGenerator().get(getIntfMac().u64NBO() + 10)};
      resolveNeigborAndProgramRoutes(ecmpHelper6, kEcmpWidthForTest);
    };

    auto verify = [=]() {
      auto sendPackets = [=](PortID /* port */, int numPacketsToSend) {
        // Single port config, traffic gets forwarded out of the same!
        sendPkts(
            utility::kOlympicQueueToDscp().at(kQueueId).front(),
            isEcn,
            numPacketsToSend,
            kPayloadLength);
      };

      // Send traffic with queue buildup and get the stats at the start!
      auto before = utility::sendPacketsWithQueueBuildup(
          sendPackets,
          getHwSwitchEnsemble(),
          masterLogicalInterfacePortIds()[0],
          numPacketsToSend);

      // For ECN all packets are sent out, for WRED, account for drops!
      const uint64_t kDroppedPackets = isEcn ? 0 : markedOrDroppedPacketCount;
      const uint64_t kExpectedOutPackets = isEcn
          ? numPacketsToSend
          : numPacketsToSend - markedOrDroppedPacketCount;

      waitForExpectedThresholdTestStats(
          isEcn,
          masterLogicalInterfacePortIds()[0],
          kQueueId,
          kExpectedOutPackets,
          kDroppedPackets,
          before);
      auto after = getHwSwitchEnsemble()->getLatestPortStats(
          masterLogicalInterfacePortIds()[0]);
      auto deltaOutPackets = (*after.queueOutPackets_())[kQueueId] -
          (*before.queueOutPackets_())[kQueueId];
      /*
       * Might see more outPackets than expected due to
       * utility::sendPacketsWithQueueBuildup()
       */
      EXPECT_GE(deltaOutPackets, kExpectedOutPackets);
      XLOG(DBG0) << "Delta out pkts: " << deltaOutPackets;

      if (verifyPacketCountFn.has_value()) {
        (*verifyPacketCountFn)(after, before, markedOrDroppedPacketCount);
      }
    };

    verifyAcrossWarmBoots(setup, verify);
  }

  void runWredThresholdTest() {
    constexpr auto kDroppedPackets{50};
    constexpr auto kThresholdBytes{
        utility::kQueueConfigAqmsWredThresholdMinMax};
    validateEcnWredThresholds(
        false /* isEcn */,
        kThresholdBytes,
        kDroppedPackets,
        verifyWredDroppedPacketCount);
  }

  void runEcnThresholdTest() {
    constexpr auto kMarkedPackets{50};
    constexpr auto kThresholdBytes{utility::kQueueConfigAqmsEcnThresholdMinMax};
    /*
     * Broadcom platforms does ECN marking at the egress and not in
     * MMU. ECN mark/no-mark decision is refreshed periodically, like
     * for TH3/TH4 once in 0.5usec, TH in 1usec etc. This means, once
     * ECN threshold is exceeded, packets will continue to be marked
     * until the next refresh, which will result in more ECN marking
     * during this test. Hence, apply a shaper on the queue to ensure
     * packets are sent out one per 2 usec (500K pps), which is enough
     * spacing of packets egressing to have ECN accounting done
     * with minimal error. However, in prod devices, we should expect
     * to see this +/- error with ECN marking.
     */
    auto shaperSetup = [&](cfg::SwitchConfig& config,
                           std::vector<int> queueIds,
                           const int txPacketLen) {
      auto maxQueueShaperKbps = ceil(500000 * txPacketLen / 1000);
      queueShaperAndBurstSetup(
          queueIds,
          config,
          0,
          maxQueueShaperKbps,
          utility::kQueueConfigBurstSizeMinKb,
          utility::kQueueConfigBurstSizeMaxKb);
    };
    validateEcnWredThresholds(
        true /* isEcn */,
        kThresholdBytes,
        kMarkedPackets,
        verifyEcnMarkedPacketCount,
        shaperSetup);
  }

  void runPerQueueEcnMarkedStatsTest() {
    const auto portId = masterLogicalInterfacePortIds()[0];
    const int queueId = utility::kOlympicSilverQueueId;

    auto setup = [=]() {
      auto config{initialConfig()};
      queueEcnWredThresholdSetup(
          true, // isEcn
          {queueId},
          config);
      queueEcnWredThresholdSetup(
          false, // !isEcn
          {queueId},
          config);
      applyNewConfig(config);

      // Setup traffic loop
      setupEcmpTraffic();

      // Send traffic
      const int kNumPacketsToSend =
          getHwSwitchEnsemble()->getMinPktsForLineRate(portId);
      sendPkts(
          utility::kOlympicQueueToDscp().at(queueId).front(),
          true,
          kNumPacketsToSend);
    };

    auto verify = [=]() {
      getHwSwitchEnsemble()->waitForLineRateOnPort(portId);

      // Get stats to verify if additional packets are getting ECN marked
      auto before = getHwSwitchEnsemble()->getLatestPortStats(portId);

      auto verifyPerQueueEcnMarkedStats = [&](const auto& newStats) {
        auto portStatsIter = newStats.find(portId);
        auto after = portStatsIter->second;

        auto deltaQueueEcnMarkedPackets =
            after.queueEcnMarkedPackets_()->find(queueId)->second -
            before.queueEcnMarkedPackets_()->find(queueId)->second;

        if (deltaQueueEcnMarkedPackets == 0) {
          XLOG(DBG0) << "queue(" << queueId << "): No ECN marked packets seen!";

          // detail for debugging test failures
          auto deltaOutPackets = getPortOutPkts(after) - getPortOutPkts(before);
          auto deltaQueueOutPackets =
              after.queueOutPackets_()->find(queueId)->second -
              before.queueOutPackets_()->find(queueId)->second;
          auto deltaEcnMarkedPackets =
              *after.outEcnCounter_() - *before.outEcnCounter_();
          XLOG(DBG3) << "queue(" << queueId << "): delta/total"
                     << " EcnMarked: " << deltaQueueEcnMarkedPackets << "/"
                     << after.queueEcnMarkedPackets_()->find(queueId)->second
                     << " outPackets: " << deltaQueueOutPackets << "/"
                     << after.queueOutPackets_()->find(queueId)->second
                     << "; Port.EcnMarked: " << deltaEcnMarkedPackets << "/"
                     << *after.outEcnCounter_()
                     << ", Port.outPackets: " << deltaOutPackets << "/"
                     << getPortOutPkts(after);

          return false;
        }

        // ECN marked packets seen for the queue
        XLOG(DBG0) << "queue(" << queueId << "): " << deltaQueueEcnMarkedPackets
                   << " ECN marked packets seen!";
        return true;
      };

      EXPECT_TRUE(getHwSwitchEnsemble()->waitPortStatsCondition(
          verifyPerQueueEcnMarkedStats, 20, std::chrono::milliseconds(200)));
    };

    verifyAcrossWarmBoots(setup, verify);
  }

  void runPerQueueWredDropStatsTest() {
    const std::vector<int> wredQueueIds = {
        utility::kOlympicSilverQueueId,
        utility::kOlympicGoldQueueId,
        utility::kOlympicEcn1QueueId};

    auto setup = [=]() {
      auto config{initialConfig()};
      queueEcnWredThresholdSetup(false /* isEcn */, wredQueueIds, config);
      applyNewConfig(config);
      setupEcmpTraffic();
    };

    auto verify = [=]() {
      // Using delta stats in this function, so get the stats before starting
      auto beforeStats = getHwSwitchEnsemble()->getLatestPortStats(
          masterLogicalInterfacePortIds()[0]);

      // Send traffic to all queues
      constexpr auto kNumPacketsToSend{1000};
      for (auto queueId : wredQueueIds) {
        sendPkts(
            utility::kOlympicQueueToDscp().at(queueId).front(),
            false,
            kNumPacketsToSend);
      }

      auto wredDropCountIncremented = [&](const auto& newStats) {
        auto portStatsIter = newStats.find(masterLogicalInterfacePortIds()[0]);
        for (auto queueId : wredQueueIds) {
          auto wredDrops = portStatsIter->second.queueWredDroppedPackets_()
                               ->find(queueId)
                               ->second -
              (*beforeStats.queueWredDroppedPackets_())[queueId];
          XLOG(DBG3) << "Queue : " << queueId << ", wredDrops : " << wredDrops;
          if (wredDrops == 0) {
            XLOG(DBG0) << "Queue " << queueId << " not seeing WRED drops!";
            return false;
          }
        }
        // All queues are seeing WRED drops!
        XLOG(DBG0) << "WRED drops seen in all queues!";
        return true;
      };

      EXPECT_TRUE(getHwSwitchEnsemble()->waitPortStatsCondition(
          wredDropCountIncremented, 20, std::chrono::milliseconds(200)));
    };

    verifyAcrossWarmBoots(setup, verify);
  }

  void runEcnTrafficNoDropTest() {
    constexpr auto kThresholdBytes{utility::kQueueConfigAqmsEcnThresholdMinMax};
    std::optional<cfg::MMUScalingFactor> scalingFactor{std::nullopt};
    if (getHwSwitch()
            ->getPlatform()
            ->getAsic()
            ->scalingFactorBasedDynamicThresholdSupported()) {
      scalingFactor = cfg::MMUScalingFactor::ONE_16TH;
    }

    auto setupScalingFactor = [&](cfg::SwitchConfig& config,
                                  std::vector<int> /* queueIds */,
                                  const int /* txPktLen */) {
      if (scalingFactor.has_value()) {
        auto& queues = config.portQueueConfigs()["queue_config"];
        for (auto& queue : queues) {
          queue.scalingFactor() = scalingFactor.value();
        }
      }
    };

    auto queueFillMaxBytes =
        utility::getQueueLimitBytes(getHwSwitch(), scalingFactor);
    if (scalingFactor.has_value()) {
      /*
       * For platforms with dynamic alpha based buffer limits, account for
       * possible usage outside of the test and need to relax the limits being
       * checked for no drops. Hence checking for queue build up to 99.9% of
       * the possible depth.
       */
      queueFillMaxBytes = queueFillMaxBytes * 0.999;
    }
    validateEcnWredThresholds(
        true /* isEcn */,
        kThresholdBytes,
        0,
        std::nullopt /* verifyPacketCountFn */,
        setupScalingFactor,
        queueFillMaxBytes);
  }

  void runMultipleQueueDropLimitTest(bool isEcn) {
    if (!isSupported(HwAsic::Feature::L3_QOS)) {
#if defined(GTEST_SKIP)
      GTEST_SKIP();
#endif
      return;
    }
    // 12K limit of queue not dropping packets (only specific to tajo asic).
    auto numPacketsToSend = 12000;
    int kPayloadLength = 200;
    auto queueId = utility::kOlympicSilverQueueId;

    auto setup = [=]() { applyNewConfig(multiplePortConfig()); };
    auto verify = [=]() {
      // Only use 10 ports
      std::vector<PortID> ports = masterLogicalPortIds();
      ports.resize(10);

      auto before = getHwSwitchEnsemble()->getLatestPortStats(ports);

      // Disable TX to build up queue for each port
      for (auto const& port : ports) {
        utility::setPortTxEnable(
            getHwSwitchEnsemble()->getHwSwitch(), port, false);
      }

      // Send 12K packets to each port
      for (auto const& port : ports) {
        sendPkts(
            utility::kOlympicQueueToDscp().at(0).front(),
            isEcn,
            numPacketsToSend,
            kPayloadLength,
            0, // ttl
            port);
      }

      // Enable TX to let traffic egress
      for (auto const& port : ports) {
        utility::setPortTxEnable(
            getHwSwitchEnsemble()->getHwSwitch(), port, true);
      }

      // Check stats on each queue
      auto waitForQueueOutPackets = [&](const auto& newStats) {
        uint64_t totalQueueDiscards{0};
        for (auto const& port : ports) {
          auto portStatsIter = newStats.find(port);
          if (portStatsIter != newStats.end()) {
            auto portStats = portStatsIter->second;
            auto queueOut = portStats.queueOutPackets_()[queueId] -
                before[port].queueOutPackets_()[queueId];
            auto queueDiscards = portStats.queueOutDiscardPackets_()[queueId] -
                before[port].queueOutDiscardPackets_()[queueId];
            if (!queueOut && !queueDiscards) {
              // No stats on port available yet
              return false;
            }
            totalQueueDiscards += queueDiscards;
            if (queueDiscards) {
              XLOG(DBG2) << "Port " << port
                         << " has non-zero discards. ECN Marking: "
                         << *portStats.outEcnCounter__ref() -
                      *before[port].outEcnCounter__ref()
                         << " WRED DROP: "
                         << *portStats.wredDroppedPackets__ref() -
                      *before[port].wredDroppedPackets__ref()
                         << " WRED queue drops: "
                         << portStats.queueWredDroppedPackets_()[queueId] -
                      before[port].queueWredDroppedPackets_()[queueId]
                         << ", Queue out packets: "
                         << portStats.queueOutPackets_()[queueId] -
                      before[port].queueOutPackets_()[queueId]
                         << ", Congestion queue drops: "
                         << portStats.queueOutDiscardPackets_()[queueId] -
                      before[port].queueOutDiscardPackets_()[queueId];
            }
          }
        }
        return totalQueueDiscards == 0;
      };

      constexpr auto kNumRetries{3};
      EXPECT_TRUE(getHwSwitchEnsemble()->waitPortStatsCondition(
          waitForQueueOutPackets,
          kNumRetries,
          std::chrono::milliseconds(std::chrono::seconds(1))));
    };

    verifyAcrossWarmBoots(setup, verify);
  }
};

TEST_F(HwAqmTest, verifyEcn) {
  runTest(true);
}

TEST_F(HwAqmTest, verifyWred) {
  runTest(false);
}

TEST_F(HwAqmTest, verifyWredDrop) {
  runWredDropTest();
}

TEST_F(HwAqmTest, verifyWredThreshold) {
  runWredThresholdTest();
}

TEST_F(HwAqmTest, verifyPerQueueWredDropStats) {
  runPerQueueWredDropStatsTest();
}

TEST_F(HwAqmTest, verifyEcnTrafficNoDrop) {
  runEcnTrafficNoDropTest();
}

TEST_F(HwAqmTest, verifyEcnThreshold) {
  runEcnThresholdTest();
}

TEST_F(HwAqmTest, verifyPerQueueEcnMarkedStats) {
  runPerQueueEcnMarkedStatsTest();
}

} // namespace facebook::fboss
