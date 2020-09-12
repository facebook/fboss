/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/hw/test/HwLinkStateDependentTest.h"
#include "fboss/agent/hw/test/HwTestPacketUtils.h"
#include "fboss/agent/test/EcmpSetupHelper.h"

#include <folly/IPAddress.h>

namespace {
constexpr uint8_t kDefaultQueue = 0;
constexpr uint8_t kTestingQueue = 7;
} // namespace

namespace facebook::fboss {

class HwSendPacketToQueueTest : public HwLinkStateDependentTest {
 protected:
  cfg::SwitchConfig initialConfig() const override {
    auto cfg = utility::oneL3IntfConfig(
        getHwSwitch(), masterLogicalPortIds()[0], cfg::PortLoopbackMode::MAC);
    return cfg;
  }

  void checkSendPacket(std::optional<uint8_t> ucQueue, bool isOutOfPort);
};

void HwSendPacketToQueueTest::checkSendPacket(
    std::optional<uint8_t> ucQueue,
    bool isOutOfPort) {
  if (!isSupported(HwAsic::Feature::L3_QOS)) {
    return;
  }

  auto setup = [=]() {
    if (!isOutOfPort) {
      // need to set up ecmp for switching
      auto kEcmpWidthForTest = 1;
      utility::EcmpSetupAnyNPorts6 ecmpHelper6{getProgrammedState()};
      auto newState = ecmpHelper6.setupECMPForwarding(
          ecmpHelper6.resolveNextHops(getProgrammedState(), kEcmpWidthForTest),
          kEcmpWidthForTest);
      applyNewState(newState);
    }
  };

  auto verify = [=]() {
    const PortID port = PortID(masterLogicalPortIds()[0]);
    const uint8_t queueID = ucQueue ? *ucQueue : kDefaultQueue;

    auto beforeOutPkts =
        getLatestPortStats(port).get_queueOutPackets_().at(queueID);
    auto vlanId = utility::firstVlanID(initialConfig());
    auto intfMac = utility::getInterfaceMac(getProgrammedState(), vlanId);
    // packet format shouldn't be matter in this test
    auto pkt = utility::makeUDPTxPacket(
        getHwSwitch(),
        vlanId,
        intfMac,
        intfMac,
        folly::IPAddressV6("2620:0:1cfe:face:b00c::3"),
        folly::IPAddressV6("2620:0:1cfe:face:b00c::4"),
        8000,
        8001);

    if (isOutOfPort) {
      if (ucQueue) {
        getHwSwitchEnsemble()->ensureSendPacketOutOfPort(
            std::move(pkt), port, *ucQueue);
      } else {
        getHwSwitchEnsemble()->ensureSendPacketOutOfPort(std::move(pkt), port);
      }
    } else {
      getHwSwitchEnsemble()->ensureSendPacketSwitched(std::move(pkt));
    }

    auto afterOutPkts =
        getLatestPortStats(port).get_queueOutPackets_().at(queueID);

    /*
     * Once the packet egresses out of the asic, the packet will be looped back
     * with dmac as neighbor mac. This will certainly fail the my mac check.
     * Some asic vendors drop the packet right away in the pipeline whereas some
     * drop later in the pipeline after MMU once the packet is queueed. This
     * will cause the queue counters to increment more than once. Always check
     * if atleast 1 packet is received.
     */
    EXPECT_GE(afterOutPkts - beforeOutPkts, 1);
  };

  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(HwSendPacketToQueueTest, SendPacketOutOfPortToUCQueue) {
  checkSendPacket(kTestingQueue, true);
}

TEST_F(HwSendPacketToQueueTest, SendPacketOutOfPortToDefaultUCQueue) {
  checkSendPacket(std::nullopt, true);
}

TEST_F(HwSendPacketToQueueTest, SendPacketSwitchedToDefaultUCQueue) {
  checkSendPacket(std::nullopt, false);
}

} // namespace facebook::fboss
