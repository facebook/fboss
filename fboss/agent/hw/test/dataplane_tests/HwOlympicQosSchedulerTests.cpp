/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/hw/test/HwLinkStateDependentTest.h"
#include "fboss/agent/hw/test/HwPortUtils.h"
#include "fboss/agent/hw/test/HwTestPacketUtils.h"
#include "fboss/agent/hw/test/TrafficPolicyUtils.h"
#include "fboss/agent/hw/test/dataplane_tests/HwTestOlympicUtils.h"
#include "fboss/agent/hw/test/dataplane_tests/HwTestQosUtils.h"
#include "fboss/agent/test/EcmpSetupHelper.h"

#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/test/ResourceLibUtil.h"

#include <folly/IPAddress.h>

namespace {
// Packets processed by WRR queues should be in proportion to their weights
void verifyWRRHelper(
    const std::map<int16_t, int64_t>& queueStats,
    int maxWeightQueueId,
    const std::map<int, uint8_t>& wrrQueueToWeight) {
  /*
   * The normalized stats (stats/weight) for every queue should be identical
   * (within certain percentage variance).
   *
   * We compare normalized stats for every queue against normalized stats for
   * queue with max weight (or better precesion).
   */
  const double kVariance = 0.05; // i.e. + or -5%

  auto maxWeightQueueBytes = queueStats.find(maxWeightQueueId)->second;
  auto maxWeightQueueWeight = wrrQueueToWeight.at(maxWeightQueueId);
  auto maxWeightQueueNormalizedBytes =
      maxWeightQueueBytes / maxWeightQueueWeight;
  auto lowMaxWeightQueueNormalizedBytes =
      maxWeightQueueNormalizedBytes * (1 - kVariance);
  auto highMaxWeightQueueNormalizedBytes =
      maxWeightQueueNormalizedBytes * (1 + kVariance);

  XLOG(DBG0) << "Max weight:: QueueId: " << maxWeightQueueId
             << " stat: " << maxWeightQueueBytes
             << " weight: " << static_cast<int>(maxWeightQueueWeight)
             << " normalized (stat/weight): " << maxWeightQueueNormalizedBytes
             << " low normalized: " << lowMaxWeightQueueNormalizedBytes
             << " high normalized: " << highMaxWeightQueueNormalizedBytes;

  for (const auto& queueStat : queueStats) {
    auto currQueueId = queueStat.first;
    auto currQueueBytes = queueStat.second;
    if (wrrQueueToWeight.find(currQueueId) == wrrQueueToWeight.end()) {
      EXPECT_EQ(0, currQueueBytes);
      continue;
    }
    auto currQueueWeight = wrrQueueToWeight.at(currQueueId);
    auto currQueueNormalizedBytes = currQueueBytes / currQueueWeight;

    XLOG(DBG0) << "Curr queue :: QueueId: " << currQueueId
               << " stat: " << currQueueBytes
               << " weight: " << static_cast<int>(currQueueWeight)
               << " normalized (stat/weight): " << currQueueNormalizedBytes;

    EXPECT_TRUE(
        lowMaxWeightQueueNormalizedBytes < currQueueNormalizedBytes &&
        currQueueNormalizedBytes < highMaxWeightQueueNormalizedBytes);
  }
}
// Only trafficQueueId should have traffic
void verifySPHelper(
    const std::map<int16_t, int64_t>& queueStats,
    int trafficQueueId) {
  XLOG(DBG0) << "trafficQueueId: " << trafficQueueId;
  for (const auto& queueStat : queueStats) {
    auto queueId = queueStat.first;
    auto statVal = queueStat.second;
    XLOG(DBG0) << "QueueId: " << queueId << " stats: " << statVal;
    EXPECT_TRUE(queueId != trafficQueueId ? statVal == 0 : statVal != 0);
  }
}
} // namespace

namespace facebook::fboss {

class HwOlympicQosSchedulerTest : public HwLinkStateDependentTest {
 private:
  cfg::SwitchConfig initialConfig() const override {
    auto cfg = utility::oneL3IntfConfig(
        getHwSwitch(), masterLogicalPortIds()[0], cfg::PortLoopbackMode::MAC);
    if (isSupported(HwAsic::Feature::L3_QOS)) {
      utility::addOlympicQueueConfig(&cfg);
      utility::addOlympicQosMaps(cfg);
    }
    return cfg;
  }
  MacAddress dstMac() const {
    auto vlanId = utility::firstVlanID(initialConfig());
    return utility::getInterfaceMac(getProgrammedState(), vlanId);
  }

  template <typename ECMP_HELPER>
  void setupECMPForwarding(const ECMP_HELPER& ecmpHelper, int ecmpWidth) {
    auto newState = ecmpHelper.setupECMPForwarding(
        ecmpHelper.resolveNextHops(getProgrammedState(), ecmpWidth), ecmpWidth);
    applyNewState(newState);
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
        static_cast<uint8_t>(dscpVal << 2)); // Trailing 2 bits are for ECN
  }

  void sendUdpPkt(uint8_t dscpVal) {
    getHwSwitch()->sendPacketSwitchedSync(createUdpPkt(dscpVal));
  }

  /*
   * QoS policies kick in only if there is congestion. Thus, in order to see
   * the effects of WRR/WRR weights/SP, we must inject enough traffic to cause
   * congestion. It turns out that injecting single packet of one dscp type
   * from each queue is not sufficient. Thus, we inject multiple packets.
   *
   * For different values of cnt (number pf UDP packets sent), here is a list
   * of packet rate (in Gbps) observed across different queues. Values are
   * approximate, but the values did not vary too much across multiple runs.
   *
   *     +---------+----------+----------+----------+----------+
   *     |   cnt   |  queue0  |  queue1  |  queue2  |  queue4  |
   *     +---------+----------+----------+----------+----------+
   *     |   1     |   0.8    |   0.8    |   0.8    |   0.8    |
   *     |   10    |   3.6    |   3.6    |   2.7    |   1.7    |
   *     |   20    |   3.3    |   5.8    |   1.8    |   1.1    |
   *     |   30    |   2.5    |   7.2    |   1.3    |   0.8    |
   *     |   50    |   1.6    |   8.9    |   0.9    |   0.5    |
   *     +---------+----------+----------+----------+----------+
   *
   * Observations:
   *  o For single packet, there is no congestion - sum of packet rate <
   *    line rate (10Gbps). As the number of packets per flow increases
   *    we start observing congestion (cnt = 10, 20, .. etc.).
   *  o WRR weights are honored when there is congestion.
   *  o At cnt = 10, there isn't enough traffic to cause congestion all the
   *    time. Thus, queue0 and queue1, with weights 15 and 80 respectively,
   *    have same packet rate, but that is more than queue2 and queue4 whose
   *    weights are 8 and 5 respectively.
   *  o As the number of packets per flow go up (cnt = 20, 30, 50), congestion
   *    becomes more likely, and thus the WRR weights are honored closer to the
   *    ration of weights.
   *  o At cnt = 50, there seems to be enough traffic for each flow to cause
   *    conestion all the time, and thus packet rate ration across any pair of
   *    queues matches their weight ratio.
   *
   *  cnt = 50 seems sufficient to check WRR weight ratio. To try to avoid the
   *  risk of test being flaky, we choose 2 * 50 number of packets for the
   *  test.
   */
  void sendUdpPkts(uint8_t dscpVal, int cnt = 100) {
    for (int i = 0; i < cnt; i++) {
      sendUdpPkt(dscpVal);
    }
  }

  /*
   * Pick one dscp value of each queue and inject packet.
   * It is impractical to try all dscp val permutations (too many), and
   * BcmOlympicTrafficTests already verify that for each possible dscp
   * value, packets go through appropriate Olympic queue.
   */
  void sendUdpPktsForAllQueues(const std::vector<int>& queueIds) {
    for (const auto& queueId : queueIds) {
      sendUdpPkts(utility::kOlympicQueueToDscp().at(queueId).front());
    }
  }

  void _setup(
      const utility::EcmpSetupAnyNPorts6& ecmpHelper6,
      const std::vector<int>& queueIds) {
    auto kEcmpWidthForTest = 1;
    setupECMPForwarding(ecmpHelper6, kEcmpWidthForTest);
    for (const auto& nextHop : ecmpHelper6.getNextHops()) {
      utility::disableTTLDecrements(
          getHwSwitch(),
          ecmpHelper6.getRouterId(),
          nextHop.intf,
          folly::IPAddress(nextHop.ip));
    }
  }

  void verifyWRRAndSP(const std::vector<int>& queueIds, int trafficQueueId) {
    if (!isSupported(HwAsic::Feature::L3_QOS)) {
      return;
    }

    utility::EcmpSetupAnyNPorts6 ecmpHelper6{getProgrammedState(), dstMac()};

    auto setup = [=]() { _setup(ecmpHelper6, queueIds); };

    auto verify = [=]() {
      sendUdpPktsForAllQueues(queueIds);
      clearPortStats();
      verifySPHelper(
          getHwSwitchEnsemble()
              ->getLatestPortStats(masterLogicalPortIds()[0])
              .queueOutPackets_,
          trafficQueueId);
    };

    verifyAcrossWarmBoots(setup, verify);
  }

  void clearPortStats() {
    auto ports = std::make_unique<std::vector<int32_t>>();
    ports->emplace_back((masterLogicalPortIds()[0]));
    getHwSwitch()->clearPortStats(ports);
  }

 protected:
  void verifyWRR();
  void verifySP();
  void verifyWRRAndICP();
  void verifyWRRAndNC();
};

void HwOlympicQosSchedulerTest::verifyWRR() {
  if (!isSupported(HwAsic::Feature::L3_QOS)) {
    return;
  }

  utility::EcmpSetupAnyNPorts6 ecmpHelper6{getProgrammedState(), dstMac()};

  auto setup = [=]() { _setup(ecmpHelper6, utility::kOlympicWRRQueueIds()); };

  auto verify = [=]() {
    sendUdpPktsForAllQueues(utility::kOlympicWRRQueueIds());
    clearPortStats();
    verifyWRRHelper(
        getHwSwitchEnsemble()
            ->getLatestPortStats(masterLogicalPortIds()[0])
            .queueOutPackets_,
        utility::getMaxWeightWRRQueue(utility::kOlympicWRRQueueToWeight()),
        utility::kOlympicWRRQueueToWeight());
  };

  verifyAcrossWarmBoots(setup, verify);
}

void HwOlympicQosSchedulerTest::verifySP() {
  if (!isSupported(HwAsic::Feature::L3_QOS)) {
    return;
  }

  utility::EcmpSetupAnyNPorts6 ecmpHelper6{getProgrammedState(), dstMac()};

  auto setup = [=]() { _setup(ecmpHelper6, utility::kOlympicSPQueueIds()); };

  auto verify = [=]() {
    sendUdpPktsForAllQueues(utility::kOlympicSPQueueIds());
    clearPortStats();
    verifySPHelper(
        getHwSwitchEnsemble()
            ->getLatestPortStats(masterLogicalPortIds()[0])
            .queueOutPackets_,
        utility::kOlympicHighestSPQueueId); // SP queue with highest queueId
                                            // should starve other SP queues
                                            // altogether
  };

  // TODO Post warmboot queue6 (SP queue with lower queueid) sees traffic
  // instead of queue7 (SP queue with higher queueid). Broadcom case
  // CS7388709 is tracking this.
  // verifyAcrossWarmBoots(setup, verify);
  setup();
  verify();
}

void HwOlympicQosSchedulerTest::verifyWRRAndICP() {
  verifyWRRAndSP(
      utility::kOlympicWRRAndICPQueueIds(),
      utility::kOlympicICPQueueId); // SP should starve WRR queues
                                    // altogether
}

void HwOlympicQosSchedulerTest::verifyWRRAndNC() {
  verifyWRRAndSP(
      utility::kOlympicWRRAndNCQueueIds(),
      utility::kOlympicNCQueueId); // SP should starve WRR queues altogether
}

TEST_F(HwOlympicQosSchedulerTest, VerifyWRR) {
  verifyWRR();
}

TEST_F(HwOlympicQosSchedulerTest, VerifySP) {
  verifySP();
}

TEST_F(HwOlympicQosSchedulerTest, VerifyWRRAndICP) {
  verifyWRRAndICP();
}

TEST_F(HwOlympicQosSchedulerTest, VerifyWRRAndNC) {
  verifyWRRAndNC();
}

} // namespace facebook::fboss
