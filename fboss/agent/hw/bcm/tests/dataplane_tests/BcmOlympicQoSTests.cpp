/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/hw/bcm/BcmAclTable.h"
#include "fboss/agent/hw/bcm/BcmError.h"
#include "fboss/agent/hw/bcm/BcmHost.h"
#include "fboss/agent/hw/bcm/tests/BcmLinkStateDependentTests.h"
#include "fboss/agent/hw/bcm/tests/BcmPortUtils.h"
#include "fboss/agent/hw/bcm/tests/BcmTestStatUtils.h"
#include "fboss/agent/hw/bcm/tests/dataplane_tests/BcmQosUtils.h"
#include "fboss/agent/hw/bcm/tests/dataplane_tests/BcmTestOlympicUtils.h"
#include "fboss/agent/hw/test/HwTestPacketUtils.h"
#include "fboss/agent/hw/test/TrafficPolicyUtils.h"
#include "fboss/agent/test/EcmpSetupHelper.h"

#include "fboss/agent/hw/test/ConfigFactory.h"

#include <folly/IPAddress.h>

extern "C" {
#include <bcm/cosq.h>
#include <bcm/l3.h>
#include <bcm/stat.h>
#include <bcm/types.h>
}

namespace facebook::fboss {

class BcmOlympicQoSTest : public BcmLinkStateDependentTests {
 protected:
  cfg::SwitchConfig initialConfig() const override {
    auto cfg = utility::oneL3IntfConfig(
        getHwSwitch(), masterLogicalPortIds()[0], cfg::PortLoopbackMode::MAC);
    if (isSupported(HwAsic::Feature::L3_QOS)) {
      utility::addOlympicQueueConfig(&cfg);
    }
    return cfg;
  }

  MacAddress kDstMac() const {
    return getPlatform()->getLocalMac();
  }

  template <typename ECMP_HELPER>
  void setupECMPForwarding(const ECMP_HELPER& ecmpHelper, int ecmpWidth) {
    auto newState = ecmpHelper.setupECMPForwarding(
        ecmpHelper.resolveNextHops(getProgrammedState(), ecmpWidth), ecmpWidth);
    applyNewState(newState);
  }

  std::unique_ptr<facebook::fboss::TxPacket> createUdpPkt(
      uint8_t dscpVal) const {
    auto vlanId = utility::firstVlanID(initialConfig());
    auto intfMac = utility::getInterfaceMac(getProgrammedState(), vlanId);

    return utility::makeUDPTxPacket(
        getHwSwitch(),
        vlanId,
        intfMac,
        intfMac,
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
    for (const auto& nextHopIp : ecmpHelper6.getNextHops()) {
      utility::disableTTLDecrements(
          getHwSwitch(),
          ecmpHelper6.getRouterId(),
          folly::IPAddress(nextHopIp.ip));
    }
  }

  void verifyWRRAndSP(const std::vector<int>& queueIds, int trafficQueueId) {
    if (!isSupported(HwAsic::Feature::L3_QOS)) {
      return;
    }

    utility::EcmpSetupAnyNPorts6 ecmpHelper6{getProgrammedState(),
                                             getPlatform()->getLocalMac()};

    auto setup = [=]() { _setup(ecmpHelper6, queueIds); };

    auto verify = [=]() {
      sendUdpPktsForAllQueues(queueIds);
      utility::verifySPHelper(
          utility::clearAndGetQueueStats(
              getUnit(), masterLogicalPortIds()[0], queueIds),
          trafficQueueId);
    };

    verifyAcrossWarmBoots(setup, verify);
  }

  void verifyDscpQueueMapping();
  void verifyWRR();
  void verifySP();
  void verifyWRRAndPlatinum();
  void verifyWRRAndNC();
};

void BcmOlympicQoSTest::verifyDscpQueueMapping() {
  if (!isSupported(HwAsic::Feature::L3_QOS)) {
    return;
  }

  auto setup = [=]() {
    utility::EcmpSetupAnyNPorts6 ecmpHelper6{getProgrammedState()};
    auto kEcmpWidthForTest = 1;
    setupECMPForwarding(ecmpHelper6, kEcmpWidthForTest);
  };

  auto verify = [=]() {
    for (const auto& entry : utility::kOlympicQueueToDscp()) {
      auto queueId = entry.first;
      auto dscpVals = entry.second;

      for (const auto& dscpVal : dscpVals) {
        auto beforeQueueOutPkts = getLatestPortStats(masterLogicalPortIds()[0])
                                      .get_queueOutPackets_()
                                      .at(queueId);
        sendUdpPkt(dscpVal);
        auto afterQueueOutPkts = getLatestPortStats(masterLogicalPortIds()[0])
                                     .get_queueOutPackets_()
                                     .at(queueId);

        EXPECT_EQ(1, afterQueueOutPkts - beforeQueueOutPkts);
      }
    }
  };

  verifyAcrossWarmBoots(setup, verify);
}

void BcmOlympicQoSTest::verifyWRR() {
  if (!isSupported(HwAsic::Feature::L3_QOS)) {
    return;
  }

  utility::EcmpSetupAnyNPorts6 ecmpHelper6{getProgrammedState(),
                                           getPlatform()->getLocalMac()};

  auto setup = [=]() { _setup(ecmpHelper6, utility::kOlympicWRRQueueIds()); };

  auto verify = [=]() {
    sendUdpPktsForAllQueues(utility::kOlympicWRRQueueIds());
    utility::verifyWRRHelper(
        utility::clearAndGetQueueStats(
            getUnit(),
            masterLogicalPortIds()[0],
            utility::kOlympicWRRQueueIds()),
        utility::getMaxWeightWRRQueue(utility::kOlympicWRRQueueToWeight()),
        utility::kOlympicWRRQueueToWeight());
  };

  verifyAcrossWarmBoots(setup, verify);
}

void BcmOlympicQoSTest::verifySP() {
  if (!isSupported(HwAsic::Feature::L3_QOS)) {
    return;
  }

  utility::EcmpSetupAnyNPorts6 ecmpHelper6{getProgrammedState(),
                                           getPlatform()->getLocalMac()};

  auto setup = [=]() { _setup(ecmpHelper6, utility::kOlympicSPQueueIds()); };

  auto verify = [=]() {
    sendUdpPktsForAllQueues(utility::kOlympicSPQueueIds());
    utility::verifySPHelper(
        utility::clearAndGetQueueStats(
            getUnit(),
            masterLogicalPortIds()[0],
            utility::kOlympicSPQueueIds()),
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

void BcmOlympicQoSTest::verifyWRRAndPlatinum() {
  verifyWRRAndSP(
      utility::kOlympicWRRAndPlatinumQueueIds(),
      utility::kOlympicPlatinumQueueId); // SP should starve WRR queues
                                         // altogether
}

void BcmOlympicQoSTest::verifyWRRAndNC() {
  verifyWRRAndSP(
      utility::kOlympicWRRAndNCQueueIds(),
      utility::kOlympicNCQueueId); // SP should starve WRR queues altogether
}

class BcmOlympicQoSAclTest : public BcmOlympicQoSTest {
 protected:
  cfg::SwitchConfig initialConfig() const override {
    auto cfg = BcmOlympicQoSTest::initialConfig();
    if (isSupported(HwAsic::Feature::L3_QOS)) {
      utility::addOlympicAcls(&cfg);
    }
    return cfg;
  }
};

TEST_F(BcmOlympicQoSAclTest, VerifyDscpQueueMapping) {
  verifyDscpQueueMapping();
}

TEST_F(BcmOlympicQoSAclTest, VerifyWRR) {
  verifyWRR();
}

TEST_F(BcmOlympicQoSAclTest, VerifySP) {
  verifySP();
}

TEST_F(BcmOlympicQoSAclTest, VerifyWRRAndPlatinum) {
  verifyWRRAndPlatinum();
}

TEST_F(BcmOlympicQoSAclTest, VerifyWRRAndNC) {
  verifyWRRAndNC();
}

class BcmOlympicQoSMapTest : public BcmOlympicQoSTest {
 protected:
  cfg::SwitchConfig initialConfig() const override {
    auto cfg = BcmOlympicQoSTest::initialConfig();
    if (isSupported(HwAsic::Feature::L3_QOS)) {
      utility::addOlympicQoSMaps(&cfg);
    }
    return cfg;
  }
};

TEST_F(BcmOlympicQoSMapTest, VerifyDscpQueueMapping) {
  verifyDscpQueueMapping();
}

TEST_F(BcmOlympicQoSMapTest, VerifyWRR) {
  verifyWRR();
}

TEST_F(BcmOlympicQoSMapTest, VerifySP) {
  verifySP();
}

TEST_F(BcmOlympicQoSMapTest, VerifyWRRAndPlatinum) {
  verifyWRRAndPlatinum();
}

TEST_F(BcmOlympicQoSMapTest, VerifyWRRAndNC) {
  verifyWRRAndNC();
}

} // namespace facebook::fboss
