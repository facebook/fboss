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
#include "fboss/agent/hw/test/HwTestPacketUtils.h"
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
    if (isSupported(HwAsic::Feature::L3_QOS)) {
      addQosMap(&cfg);
      auto streamType =
          *(getPlatform()->getAsic()->getQueueStreamTypes(false).begin());
      addQueueConfig(&cfg, streamType);
    }
    return cfg;
  }
  void setup(std::vector<uint8_t> dscpsToSend) {
    utility::EcmpSetupAnyNPorts6 helper(getProgrammedState(), dstMac());
    auto constexpr kEcmpWidth = 1;
    applyNewState(helper.setupECMPForwarding(
        helper.resolveNextHops(getProgrammedState(), kEcmpWidth), kEcmpWidth));
    for (const auto& nextHop : helper.getNextHops()) {
      utility::disableTTLDecrements(
          getHwSwitch(), helper.getRouterId(), nextHop);
    }
    for (auto dscp : dscpsToSend) {
      sendUdpPkts(dscp);
    }
    getHwSwitchEnsemble()->waitForLineRateOnPort(masterLogicalPortIds()[0]);
    getHwSwitchEnsemble()->getLatestPortStats(masterLogicalPortIds()[0]);
  }
  void verify(int lowPriQueue, int highPriQueue) {
    auto queueWaterMarks = *getHwSwitchEnsemble()
                                ->getLatestPortStats(masterLogicalPortIds()[0])
                                .queueWatermarkBytes__ref();
    auto lowPriWatermark = queueWaterMarks[lowPriQueue];
    auto highPriWatermark = queueWaterMarks[highPriQueue];
    XLOG(INFO) << " Low pri queue ( " << lowPriQueue
               << " ) watermark: " << lowPriWatermark << " High pri queue ( "
               << highPriQueue << " ) watermark: " << highPriWatermark;
    // Even with line rate traffic, its hard to create contention in MMU
    // buffer, and thus we get roughly similar watermark values. To create
    // contention, we would need to create enough bursts of traffic such that
    // the MMU is forced to throttle back the low pri queue. So as a compromise
    // we do the next best thing - compare that the high pri queue watermark
    // is at least as high as the low pri one. This lets us at least verify
    // the config application with MMU tuning and warm boot. Although to be
    // fair, in absence of precise data plane side effects we can't truly
    // conclude on the tuning effects.
    EXPECT_GE(highPriWatermark, lowPriWatermark);
  }

 private:
  void sendUdpPkts(uint8_t dscpVal, int cnt = 100) {
    for (int i = 0; i < cnt; i++) {
      sendUdpPkt(dscpVal);
    }
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
        std::vector<uint8_t>(1200, 0xff));
  }

  void sendUdpPkt(uint8_t dscpVal) {
    getHwSwitch()->sendPacketSwitchedSync(createUdpPkt(dscpVal));
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
    queue1.reservedBytes_ref() = 9984;
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
      if (PortID(*port.logicalID_ref()) == masterLogicalPortIds()[0]) {
        port.portQueueConfigName_ref() = "queue_config";
      }
    }
  }
};

TEST_F(HwMmuTuningTest, verifyReservedBytesTuning) {
  if (!isSupported(HwAsic::Feature::L3_QOS)) {
#if defined(GTEST_SKIP)
    GTEST_SKIP();
#endif
    return;
  }
  verifyAcrossWarmBoots(
      [this]() {
        setup({0, 1});
      },
      [this]() { verify(0, 1); });
}

TEST_F(HwMmuTuningTest, verifyScalingFactorTuning) {
  if (!isSupported(HwAsic::Feature::L3_QOS)) {
#if defined(GTEST_SKIP)
    GTEST_SKIP();
#endif
    return;
  }
  verifyAcrossWarmBoots(
      [this]() {
        setup({2, 3});
      },
      [this]() { verify(2, 3); });
}
} // namespace facebook::fboss
