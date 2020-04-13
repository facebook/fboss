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
#include "fboss/agent/test/EcmpSetupHelper.h"

#include <chrono>
#include <thread>

using namespace std::chrono_literals;

namespace facebook::fboss {

class HwQosMapTest : public HwLinkStateDependentTest {
 protected:
  cfg::SwitchConfig initialConfig() const override {
    auto cfg = utility::oneL3IntfConfig(
        getHwSwitch(), masterLogicalPortIds()[0], cfg::PortLoopbackMode::MAC);

    /*
     * N.B., this needs to be in initial config as in one platform, we have
     * to program qos maps before we program l3 interfaces.
     * Even if we enforce that ordering in SaiSwitch, we must still send the
     * qos maps in the same delta as the config with the interface, which in
     * this case is the initialConfig application
     */
    cfg::QosMap qosMap;
    qosMap.dscpMaps.resize(8);
    for (auto i = 0; i < 8; i++) {
      qosMap.dscpMaps[i].internalTrafficClass = i;
      for (auto j = 0; j < 8; j++) {
        qosMap.dscpMaps[i].fromDscpToTrafficClass.push_back(8 * i + j);
      }
    }
    for (auto i = 0; i < 8; i++) {
      qosMap.trafficClassToQueueId.emplace(i, i);
    }
    cfg.qosPolicies.resize(1);
    cfg.qosPolicies[0].name = "qp";
    cfg.qosPolicies[0].qosMap_ref() = qosMap;

    cfg::TrafficPolicyConfig dataPlaneTrafficPolicy;
    dataPlaneTrafficPolicy.defaultQosPolicy_ref() = "qp";
    cfg.dataPlaneTrafficPolicy_ref() = dataPlaneTrafficPolicy;

    return cfg;
  }

  void verifyDscpQueueMapping() {
    if (!isSupported(HwAsic::Feature::L3_QOS)) {
      return;
    }

    auto setup = [=]() {
      auto config = initialConfig();
      utility::EcmpSetupAnyNPorts4 ecmpHelper(
          getProgrammedState(), RouterID(0));
      applyNewState(ecmpHelper.setupECMPForwarding(
          ecmpHelper.resolveNextHops(getProgrammedState(), 1), 1));
    };

    auto verify = [=]() {
      sendPktAndVerifyQueue(0, 0);
      sendPktAndVerifyQueue(8, 1);
      sendPktAndVerifyQueue(16, 2);
      sendPktAndVerifyQueue(24, 3);
      sendPktAndVerifyQueue(32, 4);
      sendPktAndVerifyQueue(40, 5);
      sendPktAndVerifyQueue(48, 6);
      sendPktAndVerifyQueue(56, 7);
    };

    verifyAcrossWarmBoots(setup, verify);
  }

 private:
  void sendPktAndVerifyQueue(uint8_t dscp, int queueId) {
    auto vlanId = VlanID(initialConfig().vlanPorts[0].vlanID);
    auto intfMac = utility::getInterfaceMac(getProgrammedState(), vlanId);
    auto txPacket = utility::makeUDPTxPacket(
        getHwSwitch(),
        vlanId,
        intfMac, // src mac
        intfMac, // dst mac
        // folly::IPAddressV6("2620:0:1cfe:face:b00c::3"), // src ip
        // folly::IPAddressV6("2620:0:1cfe:face:b00c::4"), // dst ip
        folly::IPAddressV4("10.10.10.10"), // src ip
        folly::IPAddressV4("20.20.20.20"), // dst ip
        8000, // l4 src port
        8001, // l4 dst port
        dscp, // v4 api populates dscp bits directly, no bit shift
        255 // ttl
    );
    auto portStatsBefore = getLatestPortStats(masterLogicalPortIds()[0]);
    auto pktsBefore = getPortOutPkts(portStatsBefore);

    // XXX hack to check the queue 0 counters!
    auto queuePacketsBefore = portStatsBefore.queueOutPackets_[queueId];
    auto queueZeroPacketsBefore = portStatsBefore.queueOutPackets_[0];
    XLOGF(
        INFO,
        "port pkts before: {}, queue {} pkts before: {}, queue 0 pkts before: {}",
        pktsBefore,
        queueId,
        queuePacketsBefore,
        queueZeroPacketsBefore);
    getHwSwitch()->sendPacketSwitchedSync(std::move(txPacket));
    auto portStatsAfter = getLatestPortStats(masterLogicalPortIds()[0]);
    auto pktsAfter = getPortOutPkts(portStatsAfter);
    auto queuePacketsAfter = portStatsAfter.queueOutPackets_[queueId];
    auto queueZeroPacketsAfter = portStatsAfter.queueOutPackets_[0];
    XLOGF(
        INFO,
        "port pkts after: {}, queue {} pkts after: {}, queue 0 pkts after: {}",
        pktsAfter,
        queueId,
        queuePacketsAfter,
        queueZeroPacketsAfter);
    // Note, on some platforms, due to how loopbacked packets are pruned from
    // being broadcast, they will appear more than once on a queue counter,
    // so we can only check that the counter went up, not that it went up by
    // exactly one.
    EXPECT_GT(queuePacketsAfter, queuePacketsBefore);
  }
};

TEST_F(HwQosMapTest, VerifyDscpQueueMapping) {
  verifyDscpQueueMapping();
}

} // namespace facebook::fboss
