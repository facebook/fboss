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
#include "fboss/agent/test/ResourceLibUtil.h"

#include <chrono>
#include <thread>

using namespace std::chrono_literals;

namespace facebook::fboss {

class HwOlympicQosTests : public HwLinkStateDependentTest {
 protected:
  void SetUp() override {
    HwLinkStateDependentTest::SetUp();
    helper_ = std::make_unique<utility::EcmpSetupAnyNPorts6>(
        getProgrammedState(), RouterID(0));
  }
  cfg::SwitchConfig initialConfig() const override {
    auto cfg = utility::onePortPerVlanConfig(
        getHwSwitch(), masterLogicalPortIds(), cfg::PortLoopbackMode::MAC);
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

  void verifyDscpQueueMapping(bool frontPanel) {
    if (!isSupported(HwAsic::Feature::L3_QOS)) {
      return;
    }

    auto setup = [=]() {
      applyNewState(helper_->setupECMPForwarding(
          helper_->resolveNextHops(getProgrammedState(), 2), kEcmpWidth));
    };

    auto verify = [=]() {
      for (const auto& q2dscps : utility::kOlympicQueueToDscp()) {
        auto [q, dscps] = q2dscps;
        for (auto dscp : dscps) {
          sendPacketAndVerifyQueue(dscp, q, frontPanel);
        }
      }
    };

    verifyAcrossWarmBoots(setup, verify);
  }

 private:
  void sendPacket(uint8_t dscp, bool frontPanel) {
    auto vlanId = VlanID(*initialConfig().vlanPorts[0].vlanID_ref());
    auto intfMac = utility::getInterfaceMac(getProgrammedState(), vlanId);
    auto srcMac = utility::MacAddressGenerator().get(intfMac.u64NBO() + 1);
    auto txPacket = utility::makeUDPTxPacket(
        getHwSwitch(),
        vlanId,
        srcMac, // src mac
        intfMac, // dst mac
        folly::IPAddressV6("2620:0:1cfe:face:b00c::3"), // src ip
        folly::IPAddressV6("2620:0:1cfe:face:b00c::4"), // dst ip
        8000, // l4 src port
        8001, // l4 dst port
        dscp << 2, // shifted by 2 bits for ECN
        255 // ttl
    );
    // port is in LB mode, so it will egress and immediately loop back.
    // Since it is not re-written, it should hit the pipeline as if it
    // ingressed on the port, and be properly queued.
    if (frontPanel) {
      auto outPort = helper_->ecmpPortDescriptorAt(kEcmpWidth).phyPortID();
      getHwSwitchEnsemble()->ensureSendPacketOutOfPort(
          std::move(txPacket), outPort);
    } else {
      getHwSwitchEnsemble()->ensureSendPacketSwitched(std::move(txPacket));
    }
  }

  void sendPacketAndVerifyQueue(uint8_t dscp, int queueId, bool frontPanel) {
    auto portId = helper_->ecmpPortDescriptorAt(0).phyPortID();
    auto portStatsBefore = getLatestPortStats(portId);
    auto queuePacketsBefore = portStatsBefore.queueOutPackets__ref()[queueId];
    sendPacket(dscp, frontPanel);
    auto portStatsAfter = getLatestPortStats(portId);
    auto queuePacketsAfter = portStatsAfter.queueOutPackets__ref()[queueId];
    // Note, on some platforms, due to how loopbacked packets are pruned from
    // being broadcast, they will appear more than once on a queue counter,
    // so we can only check that the counter went up, not that it went up by
    // exactly one.
    EXPECT_GT(queuePacketsAfter, queuePacketsBefore);
  }

  static inline constexpr auto kEcmpWidth = 1;
  std::unique_ptr<utility::EcmpSetupAnyNPorts6> helper_;
};

// Verify that traffic arriving on a front panel port is qos mapped to the
// correct queue for each olympic dscp value.
TEST_F(HwOlympicQosTests, VerifyDscpQueueMappingFrontPanel) {
  verifyDscpQueueMapping(true);
}

// Verify that traffic originating on the CPU is qos mapped to the correct
// queue for each olympic dscp value.
TEST_F(HwOlympicQosTests, VerifyDscpQueueMappingCpu) {
  verifyDscpQueueMapping(false);
}

} // namespace facebook::fboss
