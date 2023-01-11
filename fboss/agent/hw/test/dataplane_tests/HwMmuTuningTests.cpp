/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/hw/test/HwLinkStateDependentTest.h"
#include "fboss/agent/hw/test/HwTestCoppUtils.h"
#include "fboss/agent/hw/test/HwTestPacketUtils.h"
#include "fboss/agent/hw/test/HwTestPortUtils.h"
#include "fboss/agent/hw/test/dataplane_tests/HwTestOlympicUtils.h"
#include "fboss/agent/hw/test/dataplane_tests/HwTestQosUtils.h"
#include "fboss/agent/test/EcmpSetupHelper.h"
#include "fboss/agent/test/ResourceLibUtil.h"

DECLARE_bool(setup_for_warmboot);

namespace facebook::fboss {

class HwMmuTuningTest : public HwLinkStateDependentTest {
 protected:
  cfg::SwitchConfig initialConfig() const override {
    auto cfg = utility::oneL3IntfConfig(
        getHwSwitch(),
        masterLogicalPortIds()[0],
        getAsic()->desiredLoopbackMode());
    if (HwTest::isSupported(HwAsic::Feature::L3_QOS)) {
      addQosMap(&cfg);
      auto streamType =
          *(getPlatform()
                ->getAsic()
                ->getQueueStreamTypes(cfg::PortType::INTERFACE_PORT)
                .begin());
      addQueueConfig(
          &cfg,
          streamType,
          getPlatform()
              ->getAsic()
              ->scalingFactorBasedDynamicThresholdSupported());
    }
    utility::addCpuQueueConfig(cfg, getAsic());
    return cfg;
  }
  void setup() {
    utility::EcmpSetupAnyNPorts6 helper(getProgrammedState(), dstMac());
    auto constexpr kEcmpWidth = 1;
    resolveNeigborAndProgramRoutes(helper, kEcmpWidth);
    utility::setPortTxEnable(getHwSwitch(), masterLogicalPortIds()[0], false);
  }
  void verify(
      int16_t lowPriQueue,
      int16_t highPriQueue,
      uint8_t lowPriDscp,
      uint8_t highPriDscp) {
    // Send  MMU Size+ bytes. With port TX disabled, all these bytes will be
    // buffered in MMU. The higher pri queue should then endup using more of MMU
    // than lower pri queue.
    sendUdpPkts(lowPriDscp, highPriDscp);

    auto countIncremented = [&](const auto& newStats) {
      auto portStats = newStats.find(masterLogicalPortIds()[0])->second;
      auto queueOutDiscardBytes = *portStats.queueOutDiscardBytes_();
      auto queueOutDiscardPackets = *portStats.queueOutDiscardPackets_();
      auto queueWaterMarks = *portStats.queueWatermarkBytes_();
      XLOG(DBG2) << " Port discards: " << *portStats.outDiscards_()
                 << " low pri queue discards, bytes: "
                 << queueOutDiscardBytes[lowPriQueue]
                 << " packets: " << queueOutDiscardPackets[lowPriQueue]
                 << " high pri queue discards, bytes: "
                 << queueOutDiscardBytes[highPriQueue]
                 << " packets: " << queueOutDiscardPackets[highPriQueue];
      auto lowPriWatermark = queueWaterMarks[lowPriQueue];
      auto highPriWatermark = queueWaterMarks[highPriQueue];
      XLOG(DBG2) << " Low pri queue ( " << lowPriQueue
                 << " ) watermark: " << lowPriWatermark << " High pri queue ( "
                 << highPriQueue << " ) watermark: " << highPriWatermark;
      return highPriWatermark >= lowPriWatermark &&
          *portStats.outDiscards_() > 0 &&
          queueOutDiscardBytes[lowPriQueue] > 0 &&
          queueOutDiscardBytes[highPriQueue] > 0 &&
          queueOutDiscardPackets[lowPriQueue] > 0 &&
          queueOutDiscardPackets[highPriQueue] > 0;
    };
    // Retry 5 times, separated by 200 ms.
    EXPECT_TRUE(getHwSwitchEnsemble()->waitPortStatsCondition(
        countIncremented, 5, std::chrono::milliseconds(200)));
    // After verification, enable tx to let packets go through.
    // New SDK expects buffer to be empty during teardown.
    if (!FLAGS_setup_for_warmboot) {
      utility::setPortTxEnable(getHwSwitch(), masterLogicalPortIds()[0], true);
    }
  }

  bool isSupported() const {
    return HwTest::isSupported(HwAsic::Feature::L3_QOS) &&
        HwTest::isSupported(HwAsic::Feature::PORT_TX_DISABLE);
  }

 private:
  void sendUdpPkts(uint8_t lowPriDscp, uint8_t highPriDscp) {
    auto mmuSizeBytes = getPlatform()->getAsic()->getMMUSizeBytes();
    auto bytesSent = 0;
    // Send high pri DSCP packet followed by low pri DSCP packet
    // since in cases where MMU is split evenly (viz. tuning reserved
    // bytes on platforms that use queue groups), at the edge of MMU fill
    // it matters which packet fills up the MMU last. In the worst case
    // the low pri queue may endup getting 1 more packet then high pri
    // queue and tripping up the test.
    std::vector<uint8_t> dscpsToSend{highPriDscp, lowPriDscp};
    // Fill entire MMU and then some
    while (bytesSent < mmuSizeBytes + 20000) {
      for (auto dscp : dscpsToSend) {
        auto pkt = createUdpPkt(dscp);
        bytesSent += pkt->buf()->computeChainDataLength();
        getHwSwitch()->sendPacketSwitchedSync(std::move(pkt));
      }
    }

    // Block until packets sent and stats updated
    auto countIncremented = [&](const auto& newStats) {
      auto portStatsIter = newStats.find(masterLogicalPortIds()[0]);
      auto outDiscard = *portStatsIter->second.outDiscards_();
      XLOG(DBG0) << "Out discard : " << outDiscard;
      return outDiscard > 0;
    };

    EXPECT_TRUE(
        getHwSwitchEnsemble()->waitPortStatsCondition(countIncremented));
  }
  MacAddress dstMac() const {
    return utility::getFirstInterfaceMac(getProgrammedState());
  }
  std::unique_ptr<facebook::fboss::TxPacket> createUdpPkt(
      uint8_t dscpVal) const {
    auto srcMac = utility::MacAddressGenerator().get(dstMac().u64NBO() + 1);

    return utility::makeUDPTxPacket(
        getHwSwitch(),
        utility::firstVlanID(initialConfig()),
        srcMac,
        dstMac(),
        folly::IPAddressV6("2620:0:1cfe:face:b00c::3"),
        folly::IPAddressV6("2620:0:1cfe:face:b00c::4"),
        8000,
        8001,
        // Trailing 2 bits are for ECN
        static_cast<uint8_t>(dscpVal << 2),
        // Hop limit
        255,
        // Payload
        std::vector<uint8_t>(7000, 0xff));
  }

  void addQosMap(cfg::SwitchConfig* cfg) const {
    cfg::QosMap qosMap;
    std::map<int, std::vector<uint8_t>> queue2Dscp = {
        {0, {0}}, {1, {1}}, {2, {2}}, {3, {3}}};

    for (auto dscp = 4; dscp < 64; ++dscp) {
      queue2Dscp[0].push_back(dscp);
    }
    qosMap.dscpMaps()->resize(queue2Dscp.size());
    ssize_t qosMapIdx = 0;
    for (const auto& q2dscps : queue2Dscp) {
      auto [q, dscps] = q2dscps;
      qosMap.dscpMaps()[qosMapIdx].internalTrafficClass() = q;
      for (auto dscp : dscps) {
        qosMap.dscpMaps()[qosMapIdx].fromDscpToTrafficClass()->push_back(dscp);
      }
      qosMap.trafficClassToQueueId()->emplace(q, q);
      ++qosMapIdx;
    }
    cfg->qosPolicies()->resize(1);
    cfg->qosPolicies()[0].name() = "qp";
    cfg->qosPolicies()[0].qosMap() = qosMap;

    cfg::TrafficPolicyConfig dataPlaneTrafficPolicy;
    dataPlaneTrafficPolicy.defaultQosPolicy() = "qp";
    cfg->dataPlaneTrafficPolicy() = dataPlaneTrafficPolicy;
    cfg::CPUTrafficPolicyConfig cpuConfig;
    cfg::TrafficPolicyConfig cpuTrafficPolicy;
    cpuTrafficPolicy.defaultQosPolicy() = "qp";
    cpuConfig.trafficPolicy() = cpuTrafficPolicy;
    cfg->cpuTrafficPolicy() = cpuConfig;
  }

  void addQueueConfig(
      cfg::SwitchConfig* config,
      cfg::StreamType streamType,
      bool scalingFactorSupported) const {
    std::vector<cfg::PortQueue> portQueues;

    // Queue 0 and 1 tune reserved bytes
    cfg::PortQueue queue0;
    queue0.id() = 0;
    queue0.name() = "queue0";
    queue0.streamType() = streamType;
    queue0.scheduling() = cfg::QueueScheduling::WEIGHTED_ROUND_ROBIN;
    queue0.weight() = 1;
    portQueues.push_back(queue0);

    cfg::PortQueue queue1;
    queue1.id() = 1;
    queue1.name() = "queue1";
    queue1.streamType() = streamType;
    queue1.scheduling() = cfg::QueueScheduling::WEIGHTED_ROUND_ROBIN;
    queue1.weight() = 1;
    if (!getPlatform()->getAsic()->mmuQgroupsEnabled()) {
      queue1.reservedBytes() = 9984;
    }
    portQueues.push_back(queue1);

    // Queue 2 and 3 tune scaling factor
    cfg::PortQueue queue2;
    queue2.id() = 2;
    queue2.name() = "queue2";
    queue2.streamType() = streamType;
    queue2.scheduling() = cfg::QueueScheduling::WEIGHTED_ROUND_ROBIN;
    queue2.weight() = 1;
    if (scalingFactorSupported) {
      queue2.scalingFactor() = cfg::MMUScalingFactor::ONE;
    }
    portQueues.push_back(queue2);

    cfg::PortQueue queue3;
    queue3.id() = 3;
    queue3.name() = "queue3";
    queue3.streamType() = streamType;
    queue3.scheduling() = cfg::QueueScheduling::WEIGHTED_ROUND_ROBIN;
    queue3.weight() = 1;
    if (scalingFactorSupported) {
      queue3.scalingFactor() = cfg::MMUScalingFactor::EIGHT;
    }
    portQueues.push_back(queue3);

    config->portQueueConfigs()["queue_config"] = portQueues;
    for (auto& port : *config->ports()) {
      port.portQueueConfigName() = "queue_config";
    }
  }
};

TEST_F(HwMmuTuningTest, verifyReservedBytesTuning) {
  if (!isSupported()) {
#if defined(GTEST_SKIP)
    GTEST_SKIP();
#endif
    return;
  }
  verifyAcrossWarmBoots(
      [this]() { setup(); }, [this]() { verify(0, 1, 0, 1); });
}

TEST_F(HwMmuTuningTest, verifyScalingFactorTuning) {
  if (!isSupported()) {
#if defined(GTEST_SKIP)
    GTEST_SKIP();
#endif
    return;
  }
  verifyAcrossWarmBoots(
      [this]() { setup(); }, [this]() { verify(2, 3, 2, 3); });
}
} // namespace facebook::fboss
