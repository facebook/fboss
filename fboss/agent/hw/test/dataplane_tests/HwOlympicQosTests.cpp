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
#include "fboss/agent/test/EcmpSetupHelper.h"

#include <chrono>
#include <thread>

using namespace std::chrono_literals;

namespace facebook::fboss {

class HwOlympicQosTests : public HwLinkStateDependentTest {
 protected:
  cfg::SwitchConfig initialConfig() const override {
    auto cfg = utility::oneL3IntfConfig(
        getHwSwitch(), masterLogicalPortIds()[0], cfg::PortLoopbackMode::MAC);
    /*
     * N.B., On one platform, we have to program qos maps before we program l3
     * interfaces. Even if we enforce that ordering in SaiSwitch, we must still
     * send the qos maps in the same delta as the config with the interface.
     *
     * Since we may want to vary the qos maps per test, we shouldn't program
     * l3 interfaces as part of initial config, and only together with the
     * qos maps.
     */
    utility::addOlympicQosMaps(cfg);
    return cfg;
  }

  void verifyDscpQueueMapping() {
    if (!isSupported(HwAsic::Feature::L3_QOS)) {
      return;
    }

    auto setup = [=]() {
      utility::EcmpSetupAnyNPorts6 ecmpHelper(
          getProgrammedState(), RouterID(0));
      applyNewState(ecmpHelper.setupECMPForwarding(
          ecmpHelper.resolveNextHops(getProgrammedState(), 1), 1));
    };

    auto verify = [=]() {
      for (const auto& q2dscps : utility::kOlympicQueueToDscp()) {
        auto [q, dscps] = q2dscps;
        for (auto dscp : dscps) {
          sendPacketAndVerifyQueue(dscp, q);
        }
      }
    };

    verifyAcrossWarmBoots(setup, verify);
  }

 private:
  void sendPacket(uint8_t dscp) {
    auto vlanId = VlanID(initialConfig().vlanPorts[0].vlanID);
    auto intfMac = utility::getInterfaceMac(getProgrammedState(), vlanId);
    auto txPacket = utility::makeUDPTxPacket(
        getHwSwitch(),
        vlanId,
        intfMac, // src mac
        intfMac, // dst mac
        folly::IPAddressV6("2620:0:1cfe:face:b00c::3"), // src ip
        folly::IPAddressV6("2620:0:1cfe:face:b00c::4"), // dst ip
        8000, // l4 src port
        8001, // l4 dst port
        dscp << 2, // shifted by 2 bits for ECN
        255 // ttl
    );
    getHwSwitch()->sendPacketSwitchedSync(std::move(txPacket));
  }

  void sendPacketAndVerifyQueue(uint8_t dscp, int queueId) {
    auto portStatsBefore = getLatestPortStats(masterLogicalPortIds()[0]);
    auto queuePacketsBefore = portStatsBefore.queueOutPackets_[queueId];
    sendPacket(dscp);
    auto portStatsAfter = getLatestPortStats(masterLogicalPortIds()[0]);
    auto queuePacketsAfter = portStatsAfter.queueOutPackets_[queueId];
    // Note, on some platforms, due to how loopbacked packets are pruned from
    // being broadcast, they will appear more than once on a queue counter,
    // so we can only check that the counter went up, not that it went up by
    // exactly one.
    EXPECT_GT(queuePacketsAfter, queuePacketsBefore);
  }
};

TEST_F(HwOlympicQosTests, VerifyDscpQueueMapping) {
  verifyDscpQueueMapping();
}

} // namespace facebook::fboss
