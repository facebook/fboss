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
#include "fboss/agent/hw/bcm/BcmError.h"
#include "fboss/agent/hw/bcm/tests/BcmLinkStateDependentTests.h"
#include "fboss/agent/hw/bcm/tests/BcmPortUtils.h"
#include "fboss/agent/hw/bcm/tests/BcmTestStatUtils.h"
#include "fboss/agent/hw/bcm/tests/dataplane_tests/BcmQosUtils.h"
#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/hw/test/HwTestPacketUtils.h"
#include "fboss/agent/test/EcmpSetupHelper.h"

#include <folly/IPAddress.h>

namespace {
constexpr uint8_t kDefaultQueue = 0;
constexpr uint8_t kTestingQueue = 7;
} // namespace

namespace facebook::fboss {

class BcmSendPacketToQueueTest : public BcmLinkStateDependentTests {
 protected:
  cfg::SwitchConfig initialConfig() const override {
    auto cfg = utility::oneL3IntfConfig(
        getHwSwitch(), masterLogicalPortIds()[0], cfg::PortLoopbackMode::MAC);
    return cfg;
  }

  void checkSendPacket(std::optional<uint8_t> ucQueue, bool isOutOfPort);
};

void BcmSendPacketToQueueTest::checkSendPacket(
    std::optional<uint8_t> ucQueue,
    bool isOutOfPort) {
  if (!getPlatform()->isCosSupported()) {
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
        getHwSwitch()->sendPacketOutOfPortSync(std::move(pkt), port, *ucQueue);
      } else {
        getHwSwitch()->sendPacketOutOfPortSync(std::move(pkt), port);
      }
    } else {
      getHwSwitch()->sendPacketSwitchedSync(std::move(pkt));
    }

    auto afterOutPkts =
        getLatestPortStats(port).get_queueOutPackets_().at(queueID);

    EXPECT_EQ(1, afterOutPkts - beforeOutPkts);
  };

  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(BcmSendPacketToQueueTest, SendPacketOutOfPortToUCQueue) {
  checkSendPacket(kTestingQueue, true);
}

TEST_F(BcmSendPacketToQueueTest, SendPacketOutOfPortToDefaultUCQueue) {
  checkSendPacket(std::nullopt, true);
}

TEST_F(BcmSendPacketToQueueTest, SendPacketSwitchedToDefaultUCQueue) {
  checkSendPacket(std::nullopt, false);
}

} // namespace facebook::fboss
