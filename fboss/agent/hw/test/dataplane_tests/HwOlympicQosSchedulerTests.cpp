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
#include "fboss/agent/state/Port.h"
#include "fboss/agent/test/ResourceLibUtil.h"

#include <folly/IPAddress.h>

namespace {
auto constexpr kRetries = 10;
auto constexpr kEcmpWidthForTest = 1;
} // namespace
namespace facebook::fboss {

class HwOlympicQosSchedulerTest : public HwLinkStateDependentTest {
 private:
  cfg::SwitchConfig initialConfig() const override {
    auto cfg = utility::onePortPerInterfaceConfig(
        getHwSwitch(),
        masterLogicalPortIds(),
        getAsic()->desiredLoopbackModes());
    auto streamType = *(getPlatform()
                            ->getAsic()
                            ->getQueueStreamTypes(cfg::PortType::INTERFACE_PORT)
                            .begin());
    utility::addOlympicQueueConfig(&cfg, streamType, getPlatform()->getAsic());
    utility::addOlympicQosMaps(cfg, getPlatform()->getAsic());
    return cfg;
  }
  MacAddress dstMac() const {
    return utility::getFirstInterfaceMac(getProgrammedState());
  }
  PortID outPort() const {
    utility::EcmpSetupAnyNPorts6 ecmpHelper6{getProgrammedState()};
    return ecmpHelper6.nhop(0).portDesc.phyPortID();
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

  void sendUdpPkt(uint8_t dscpVal, bool frontPanel = true) {
    utility::EcmpSetupAnyNPorts6 ecmpHelper6{getProgrammedState()};
    if (frontPanel) {
      getHwSwitch()->sendPacketOutOfPortAsync(
          createUdpPkt(dscpVal),
          ecmpHelper6.ecmpPortDescriptorAt(kEcmpWidthForTest).phyPortID());
    } else {
      getHwSwitch()->sendPacketSwitchedSync(createUdpPkt(dscpVal));
    }
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
  void sendUdpPkts(uint8_t dscpVal, int cnt, bool frontPanel = true) {
    for (int i = 0; i < cnt; i++) {
      sendUdpPkt(dscpVal, frontPanel);
    }
  }

  /*
   * Pick one dscp value of each queue and inject packet.
   * It is impractical to try all dscp val permutations (too many), and
   * BcmOlympicTrafficTests already verify that for each possible dscp
   * value, packets go through appropriate Olympic queue.
   */
  void sendUdpPktsForAllQueues(
      const std::vector<int>& queueIds,
      const std::map<int, std::vector<uint8_t>>& queueToDscp,
      bool frontPanel = true) {
    // Higher speed ports need more packets to reach line rate
    auto pktsToSend = getHwSwitchEnsemble()->getMinPktsForLineRate(outPort());
    for (const auto& queueId : queueIds) {
      sendUdpPkts(queueToDscp.at(queueId).front(), pktsToSend, frontPanel);
    }
  }

  void _setup(
      const utility::EcmpSetupAnyNPorts6& ecmpHelper6,
      const std::vector<int>& queueIds) {
    resolveNeigborAndProgramRoutes(ecmpHelper6, kEcmpWidthForTest);
    utility::ttlDecrementHandlingForLoopbackTraffic(
        getHwSwitch(), ecmpHelper6.getRouterId(), ecmpHelper6.nhop(0));
  }

  void verifyWRRAndSP(const std::vector<int>& queueIds, int trafficQueueId) {
    utility::EcmpSetupAnyNPorts6 ecmpHelper6{getProgrammedState(), dstMac()};
    auto setup = [=]() { _setup(ecmpHelper6, queueIds); };

    auto verify = [=]() {
      sendUdpPktsForAllQueues(
          queueIds, utility::kOlympicQueueToDscp(getAsic()));
      EXPECT_TRUE(verifySPHelper(trafficQueueId));
    };

    verifyAcrossWarmBoots(setup, verify);
  }

  void _verifyDscpQueueMappingHelper(
      const std::map<int, std::vector<uint8_t>>& queueToDscp) {
    auto portId = outPort();
    auto portStatsBefore = getLatestPortStats(portId);
    for (const auto& q2dscps : queueToDscp) {
      for (auto dscp : q2dscps.second) {
        sendUdpPkt(dscp);
      }
    }
    EXPECT_TRUE(utility::verifyQueueMappings(
        portStatsBefore, queueToDscp, getHwSwitchEnsemble(), portId));
  }

 protected:
  void verifyWRR();
  void verifySP(bool frontPanelTraffic = true);
  void verifyWRRAndICP();
  void verifyWRRAndNC();

  void verifyWRRToAllSPDscpToQueue();
  void verifyWRRToAllSPTraffic();

 private:
  bool verifyWRRHelper(
      int maxWeightQueueId,
      const std::map<int, uint8_t>& wrrQueueToWeight);
  bool verifySPHelper(int trafficQueueId);
};

// Packets processed by WRR queues should be in proportion to their weights
bool HwOlympicQosSchedulerTest::verifyWRRHelper(
    int maxWeightQueueId,
    const std::map<int, uint8_t>& wrrQueueToWeight) {
  /*
   * The normalized stats (stats/weight) for every queue should be identical
   * (within certain percentage variance).
   *
   * We compare normalized stats for every queue against normalized stats for
   * queue with max weight (or better precesion).
   */
  const double kVariance = 0.10; // i.e. + or -10%
  auto portId = outPort();
  getHwSwitchEnsemble()->waitForLineRateOnPort(portId);
  auto retries = kRetries;
  while (retries--) {
    auto queueStatsBefore = *getLatestPortStats(portId).queueOutPackets_();
    sleep(1);
    auto queueStatsAfter = *getLatestPortStats(portId).queueOutPackets_();
    auto maxWeightQueueBytes = queueStatsAfter.find(maxWeightQueueId)->second -
        queueStatsBefore.find(maxWeightQueueId)->second;
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
    auto distributionOk = true;
    for (const auto& queueStat : queueStatsAfter) {
      auto currQueueId = queueStat.first;
      auto currQueueBytes = queueStat.second - queueStatsBefore[currQueueId];
      if (wrrQueueToWeight.find(currQueueId) == wrrQueueToWeight.end()) {
        if (currQueueBytes) {
          distributionOk = false;
          break;
        }
        continue;
      }
      auto currQueueWeight = wrrQueueToWeight.at(currQueueId);
      auto currQueueNormalizedBytes = currQueueBytes / currQueueWeight;

      XLOG(DBG0) << "Curr queue :: QueueId: " << currQueueId
                 << " stat: " << currQueueBytes
                 << " weight: " << static_cast<int>(currQueueWeight)
                 << " normalized (stat/weight): " << currQueueNormalizedBytes;

      if (!(lowMaxWeightQueueNormalizedBytes < currQueueNormalizedBytes &&
            currQueueNormalizedBytes < highMaxWeightQueueNormalizedBytes)) {
        distributionOk = false;
        break;
      }
    }
    if (distributionOk) {
      return true;
    }
    XLOG(DBG2) << " Retrying ...";
  }
  return false;
}

// Only trafficQueueId should have traffic
bool HwOlympicQosSchedulerTest::verifySPHelper(int trafficQueueId) {
  XLOG(DBG0) << "trafficQueueId: " << trafficQueueId;
  auto portId = outPort();
  getHwSwitchEnsemble()->waitForLineRateOnPort(portId);
  auto retries = kRetries;
  while (retries--) {
    auto distributionOk = true;
    auto queueStatsBefore = *getLatestPortStats(portId).queueOutPackets_();
    sleep(1);
    auto queueStatsAfter = *getLatestPortStats(portId).queueOutPackets_();
    for (const auto& queueStat : queueStatsAfter) {
      auto queueId = queueStat.first;
      auto statVal = queueStat.second - queueStatsBefore[queueId];
      XLOG(DBG0) << "QueueId: " << queueId << " stats: " << statVal;
      distributionOk =
          (queueId != trafficQueueId ? statVal == 0 : statVal != 0);
      if (!distributionOk) {
        break;
      }
    }
    if (distributionOk) {
      return true;
    }
    XLOG(DBG2) << " Retrying ...";
  }
  return false;
}

void HwOlympicQosSchedulerTest::verifyWRR() {
  utility::EcmpSetupAnyNPorts6 ecmpHelper6{getProgrammedState(), dstMac()};

  auto setup = [=]() {
    _setup(ecmpHelper6, utility::kOlympicWRRQueueIds(getAsic()));
  };

  auto verify = [=]() {
    sendUdpPktsForAllQueues(
        utility::kOlympicWRRQueueIds(getAsic()),
        utility::kOlympicQueueToDscp(getAsic()));
    EXPECT_TRUE(verifyWRRHelper(
        utility::getMaxWeightWRRQueue(
            utility::kOlympicWRRQueueToWeight(getAsic())),
        utility::kOlympicWRRQueueToWeight(getAsic())));
  };

  verifyAcrossWarmBoots(setup, verify);
}

void HwOlympicQosSchedulerTest::verifySP(bool frontPanelTraffic) {
  utility::EcmpSetupAnyNPorts6 ecmpHelper6{getProgrammedState(), dstMac()};

  auto setup = [=]() {
    _setup(ecmpHelper6, utility::kOlympicSPQueueIds(getAsic()));
  };

  auto verify = [=]() {
    sendUdpPktsForAllQueues(
        utility::kOlympicSPQueueIds(getAsic()),
        utility::kOlympicQueueToDscp(getAsic()),
        frontPanelTraffic);
    EXPECT_TRUE(verifySPHelper(
        // SP queue with highest queueId
        // should starve other SP queues
        // altogether
        utility::getOlympicQueueId(getAsic(), utility::OlympicQueueType::NC)));
  };

  verifyAcrossWarmBoots(setup, verify);
}

void HwOlympicQosSchedulerTest::verifyWRRAndICP() {
  verifyWRRAndSP(
      utility::kOlympicWRRAndICPQueueIds(getAsic()),
      utility::getOlympicQueueId(
          getAsic(), utility::OlympicQueueType::ICP)); // SP should starve WRR
                                                       // queues altogether
}

void HwOlympicQosSchedulerTest::verifyWRRAndNC() {
  verifyWRRAndSP(
      utility::kOlympicWRRAndNCQueueIds(getAsic()),
      utility::getOlympicQueueId(
          getAsic(), utility::OlympicQueueType::NC)); // SP should starve WRR
                                                      // queues altogether
}

void HwOlympicQosSchedulerTest::verifyWRRToAllSPDscpToQueue() {
  utility::EcmpSetupAnyNPorts6 ecmpHelper6{getProgrammedState(), dstMac()};

  auto setup = [=]() {
    resolveNeigborAndProgramRoutes(ecmpHelper6, kEcmpWidthForTest);
  };

  auto verify = [=]() {
    _verifyDscpQueueMappingHelper(utility::kOlympicQueueToDscp(getAsic()));
  };

  auto setupPostWarmboot = [=]() {
    auto newCfg{initialConfig()};
    auto streamType = *(getPlatform()
                            ->getAsic()
                            ->getQueueStreamTypes(cfg::PortType::INTERFACE_PORT)
                            .begin());
    utility::addOlympicAllSPQueueConfig(&newCfg, streamType, getAsic());
    utility::addOlympicV2QosMaps(newCfg, getAsic());
    applyNewConfig(newCfg);
  };

  auto verifyPostWarmboot = [=]() {
    _verifyDscpQueueMappingHelper(utility::kOlympicV2QueueToDscp(getAsic()));
  };

  verifyAcrossWarmBoots(setup, verify, setupPostWarmboot, verifyPostWarmboot);
}

void HwOlympicQosSchedulerTest::verifyWRRToAllSPTraffic() {
  utility::EcmpSetupAnyNPorts6 ecmpHelper6{getProgrammedState(), dstMac()};

  auto setup = [=]() {
    _setup(ecmpHelper6, utility::kOlympicWRRQueueIds(getAsic()));
  };

  auto verify = [=]() {};

  auto setupPostWarmboot = [=]() {
    auto newCfg{initialConfig()};
    auto streamType = *(getPlatform()
                            ->getAsic()
                            ->getQueueStreamTypes(cfg::PortType::INTERFACE_PORT)
                            .begin());
    utility::addOlympicAllSPQueueConfig(&newCfg, streamType, getAsic());
    utility::addOlympicV2QosMaps(newCfg, getAsic());
    applyNewConfig(newCfg);
  };

  auto verifyPostWarmboot = [=]() {
    sendUdpPktsForAllQueues(
        utility::kOlympicAllSPQueueIds(getAsic()),
        utility::kOlympicV2QueueToDscp(getAsic()));
    EXPECT_TRUE(verifySPHelper(
        // SP queue with highest queueId
        // should starve other SP queues
        // altogether
        utility::kOlympicAllSPHighestSPQueueId));
  };

  verifyAcrossWarmBoots(setup, verify, setupPostWarmboot, verifyPostWarmboot);
}

TEST_F(HwOlympicQosSchedulerTest, VerifyWRR) {
  verifyWRR();
}

TEST_F(HwOlympicQosSchedulerTest, VerifySP) {
  verifySP();
}

/*
 * This test asserts that CPU injected traffic from a higher priority
 * queue will preempt traffic from a lower priority queue. We only
 * test for CPU traffic explicitly as VerifySP above already
 * tests front panel traffic.
 */
TEST_F(HwOlympicQosSchedulerTest, VerifySPPreemptionCPUTraffic) {
  auto spQueueIds = utility::kOlympicSPQueueIds(getAsic());
  auto getQueueIndex = [&](int queueId) {
    for (auto i = 0; i < spQueueIds.size(); ++i) {
      if (spQueueIds[i] == queueId) {
        return i;
      }
    }
    throw FbossError("Could not find queueId: ", queueId);
  };
  // Assert that ICP comes before NC in the queueIds array.
  // We will send traffic to all queues in order. So for
  // preemption we want lower pri (ICP) queue to go before
  // higher pri queue (NC).
  ASSERT_LT(
      getQueueIndex(
          getOlympicQueueId(getAsic(), utility::OlympicQueueType::ICP)),
      getQueueIndex(
          getOlympicQueueId(getAsic(), utility::OlympicQueueType::NC)));

  verifySP(false /*frontPanelTraffic*/);
}

TEST_F(HwOlympicQosSchedulerTest, VerifyWRRAndICP) {
  verifyWRRAndICP();
}

TEST_F(HwOlympicQosSchedulerTest, VerifyWRRAndNC) {
  verifyWRRAndNC();
}

TEST_F(HwOlympicQosSchedulerTest, VerifyWRRToAllSPDscpToQueue) {
  verifyWRRToAllSPDscpToQueue();
}

TEST_F(HwOlympicQosSchedulerTest, VerifyWRRToAllSPTraffic) {
  verifyWRRToAllSPTraffic();
}

} // namespace facebook::fboss
