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

namespace facebook::fboss {

class HwMmuTuningTest : public HwLinkStateDependentTest {
 protected:
  cfg::SwitchConfig initialConfig() const override {
    auto cfg = utility::oneL3IntfConfig(
        getHwSwitch(), masterLogicalPortIds()[0], cfg::PortLoopbackMode::MAC);
    if (HwTest::isSupported(HwAsic::Feature::L3_QOS)) {
      addQosMap(&cfg);
      auto streamType =
          *(getPlatform()->getAsic()->getQueueStreamTypes(false).begin());
      addQueueConfig(&cfg, streamType);
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

    auto portStats =
        getHwSwitchEnsemble()->getLatestPortStats(masterLogicalPortIds()[0]);
    auto queueOutDiscardBytes = *portStats.queueOutDiscardBytes__ref();
    auto queueOutDiscardPackets = *portStats.queueOutDiscardPackets__ref();
    auto queueWaterMarks = *portStats.queueWatermarkBytes__ref();
    XLOG(INFO) << " Port discards: " << *portStats.outDiscards__ref()
               << " low pri queue discards, bytes: "
               << queueOutDiscardBytes[lowPriQueue]
               << " packets: " << queueOutDiscardPackets[lowPriQueue]
               << " high pri queue discards, bytes: "
               << queueOutDiscardBytes[highPriQueue]
               << " packets: " << queueOutDiscardPackets[highPriQueue];
    auto lowPriWatermark = queueWaterMarks[lowPriQueue];
    auto highPriWatermark = queueWaterMarks[highPriQueue];
    XLOG(INFO) << " Low pri queue ( " << lowPriQueue
               << " ) watermark: " << lowPriWatermark << " High pri queue ( "
               << highPriQueue << " ) watermark: " << highPriWatermark;
    EXPECT_GE(highPriWatermark, lowPriWatermark);
    EXPECT_GT(*portStats.outDiscards__ref(), 0);
    EXPECT_GT(queueOutDiscardBytes[lowPriQueue], 0);
    EXPECT_GT(queueOutDiscardBytes[highPriQueue], 0);
    EXPECT_GT(queueOutDiscardPackets[lowPriQueue], 0);
    EXPECT_GT(queueOutDiscardPackets[highPriQueue], 0);
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
      auto outDiscard = *portStatsIter->second.outDiscards__ref();
      XLOG(DBG0) << "Out discard : " << outDiscard;
      return outDiscard > 0;
    };

    EXPECT_TRUE(
        getHwSwitchEnsemble()->waitPortStatsCondition(countIncremented));
  }
  MacAddress dstMac() const {
    auto vlanId = utility::firstVlanID(initialConfig());
    return utility::getInterfaceMac(getProgrammedState(), vlanId);
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
    qosMap.dscpMaps_ref()->resize(queue2Dscp.size());
    ssize_t qosMapIdx = 0;
    for (const auto& q2dscps : queue2Dscp) {
      auto [q, dscps] = q2dscps;
      qosMap.dscpMaps_ref()[qosMapIdx].internalTrafficClass_ref() = q;
      for (auto dscp : dscps) {
        qosMap.dscpMaps_ref()[qosMapIdx]
            .fromDscpToTrafficClass_ref()
            ->push_back(dscp);
      }
      qosMap.trafficClassToQueueId_ref()->emplace(q, q);
      ++qosMapIdx;
    }
    cfg->qosPolicies_ref()->resize(1);
    cfg->qosPolicies_ref()[0].name_ref() = "qp";
    cfg->qosPolicies_ref()[0].qosMap_ref() = qosMap;

    cfg::TrafficPolicyConfig dataPlaneTrafficPolicy;
    dataPlaneTrafficPolicy.defaultQosPolicy_ref() = "qp";
    cfg->dataPlaneTrafficPolicy_ref() = dataPlaneTrafficPolicy;
    cfg::CPUTrafficPolicyConfig cpuConfig;
    cfg::TrafficPolicyConfig cpuTrafficPolicy;
    cpuTrafficPolicy.defaultQosPolicy_ref() = "qp";
    cpuConfig.trafficPolicy_ref() = cpuTrafficPolicy;
    cfg->cpuTrafficPolicy_ref() = cpuConfig;
  }

  void addQueueConfig(cfg::SwitchConfig* config, cfg::StreamType streamType)
      const {
    std::vector<cfg::PortQueue> portQueues;

    // Queue 0 and 1 tune reserved bytes
    cfg::PortQueue queue0;
    queue0.id_ref() = 0;
    queue0.name_ref() = "queue0";
    queue0.streamType_ref() = streamType;
    queue0.scheduling_ref() = cfg::QueueScheduling::WEIGHTED_ROUND_ROBIN;
    queue0.weight_ref() = 1;
    portQueues.push_back(queue0);

    cfg::PortQueue queue1;
    queue1.id_ref() = 1;
    queue1.name_ref() = "queue1";
    queue1.streamType_ref() = streamType;
    queue1.scheduling_ref() = cfg::QueueScheduling::WEIGHTED_ROUND_ROBIN;
    queue1.weight_ref() = 1;
    if (!getPlatform()->getAsic()->mmuQgroupsEnabled()) {
      queue1.reservedBytes_ref() = 9984;
    }
    portQueues.push_back(queue1);

    // Queue 2 and 3 tune scaling factor
    cfg::PortQueue queue2;
    queue2.id_ref() = 2;
    queue2.name_ref() = "queue2";
    queue2.streamType_ref() = streamType;
    queue2.scheduling_ref() = cfg::QueueScheduling::WEIGHTED_ROUND_ROBIN;
    queue2.weight_ref() = 1;
    queue2.scalingFactor_ref() = cfg::MMUScalingFactor::ONE;
    portQueues.push_back(queue2);

    cfg::PortQueue queue3;
    queue3.id_ref() = 3;
    queue3.name_ref() = "queue3";
    queue3.streamType_ref() = streamType;
    queue3.scheduling_ref() = cfg::QueueScheduling::WEIGHTED_ROUND_ROBIN;
    queue3.weight_ref() = 1;
    queue3.scalingFactor_ref() = cfg::MMUScalingFactor::EIGHT;
    portQueues.push_back(queue3);

    config->portQueueConfigs_ref()["queue_config"] = portQueues;
    for (auto& port : *config->ports_ref()) {
      port.portQueueConfigName_ref() = "queue_config";
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
