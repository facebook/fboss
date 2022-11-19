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
#include "fboss/agent/hw/test/dataplane_tests/HwTestQosUtils.h"
#include "fboss/agent/test/EcmpSetupHelper.h"

#include <folly/IPAddress.h>

namespace facebook::fboss {

// This test simply checks if the loop back mode is working correctly.
// Some of our hw tests rely on the looped back packets (e.g. Ecn and Wred)
// When srcMac == dstMac, brcm-sai could drop the packets, thus causing
// confusions in these tests (whether it's implementation issue or packets being
// dropped).
class HwLoopBackTest : public HwLinkStateDependentTest {
 private:
  cfg::SwitchConfig initialConfig() const override {
    return utility::oneL3IntfConfig(
        getHwSwitch(),
        masterLogicalPortIds()[0],
        getAsic()->desiredLoopbackMode());
  }

  folly::MacAddress getIntfMac() const {
    return utility::getFirstInterfaceMac(getProgrammedState());
  }

  void sendPkt(bool frontPanel, uint8_t ttl) {
    auto vlanId = utility::firstVlanID(initialConfig());
    auto intfMac = utility::getFirstInterfaceMac(getProgrammedState());
    auto txPacket = utility::makeUDPTxPacket(
        getHwSwitch(),
        vlanId,
        intfMac,
        intfMac,
        folly::IPAddressV6("2620:0:1cfe:face:b00c::3"),
        folly::IPAddressV6("2620:0:1cfe:face:b00c::4"),
        8000,
        8001,
        0,
        ttl);

    if (frontPanel) {
      getHwSwitchEnsemble()->ensureSendPacketOutOfPort(
          std::move(txPacket), masterLogicalPortIds()[0]);
    } else {
      getHwSwitchEnsemble()->ensureSendPacketSwitched(std::move(txPacket));
    }
  }

  static inline constexpr auto pktTtl = 255;

 protected:
  void runTest(bool frontPanel) {
    auto setup = [=]() {
      auto kEcmpWidthForTest = 1;
      utility::EcmpSetupAnyNPorts6 ecmpHelper6{
          getProgrammedState(), getIntfMac()};
      resolveNeigborAndProgramRoutes(ecmpHelper6, kEcmpWidthForTest);
    };
    auto verify = [=]() {
      auto beforePortStats = getLatestPortStats(masterLogicalPortIds()[0]);
      sendPkt(frontPanel, pktTtl);
      auto afterPortStats = getLatestPortStats(masterLogicalPortIds()[0]);
      // For packets going out to front panel, they would not go through the
      // routing logic the very first time (but directly looped back).
      // Therefore, the counter would plus one compared to the cpu port.
      if (frontPanel) {
        EXPECT_EQ(
            getPortOutPkts(afterPortStats) - getPortOutPkts(beforePortStats),
            pktTtl);
      } else {
        EXPECT_EQ(
            getPortOutPkts(afterPortStats) - getPortOutPkts(beforePortStats),
            pktTtl - 1);
      }
    };

    verifyAcrossWarmBoots(setup, verify);
  }
};

TEST_F(HwLoopBackTest, VerifyLoopBackFrontPanel) {
  runTest(true);
}

TEST_F(HwLoopBackTest, VerifyLoopBackCpu) {
  runTest(false);
}

} // namespace facebook::fboss
