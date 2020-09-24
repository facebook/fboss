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
#include "fboss/agent/hw/test/TrafficPolicyUtils.h"
#include "fboss/agent/hw/test/dataplane_tests/HwTestOlympicUtils.h"
#include "fboss/agent/hw/test/dataplane_tests/HwTestQosUtils.h"
#include "fboss/agent/test/EcmpSetupHelper.h"

#include <folly/IPAddress.h>

namespace facebook::fboss {

class HwAqmTest : public HwLinkStateDependentTest {
 private:
  cfg::SwitchConfig initialConfig() const override {
    auto cfg = utility::oneL3IntfConfig(
        getHwSwitch(), masterLogicalPortIds()[0], cfg::PortLoopbackMode::MAC);
    if (isSupported(HwAsic::Feature::L3_QOS)) {
      auto streamType =
          *(getPlatform()->getAsic()->getQueueStreamTypes(false).begin());
      utility::addOlympicQueueConfig(&cfg, streamType, true /*add wred*/);
      utility::addOlympicQosMaps(cfg);
    }
    return cfg;
  }

  uint8_t kDscp() const {
    return 5;
  }

  template <typename ECMP_HELPER>
  void setupECMPForwarding(const ECMP_HELPER& ecmpHelper, int ecmpWidth) {
    auto newState = ecmpHelper.setupECMPForwarding(
        ecmpHelper.resolveNextHops(getProgrammedState(), ecmpWidth), ecmpWidth);
    applyNewState(newState);
  }

  template <typename ECMP_HELPER>
  void disableTTLDecrements(const ECMP_HELPER& ecmpHelper) {
    for (const auto& nextHop : ecmpHelper.getNextHops()) {
      utility::disableTTLDecrements(
          getHwSwitch(), ecmpHelper.getRouterId(), nextHop);
    }
  }

  void sendUdpPkt(uint8_t dscpVal, bool isEcn) {
    auto kECT1 = 0x01; // ECN capable transport ECT(1)
    auto vlanId = utility::firstVlanID(initialConfig());
    auto intfMac = utility::getInterfaceMac(getProgrammedState(), vlanId);

    dscpVal = static_cast<uint8_t>(dscpVal << 2);
    if (isEcn) {
      dscpVal |= kECT1;
    }

    auto txPacket = utility::makeUDPTxPacket(
        getHwSwitch(),
        vlanId,
        intfMac,
        intfMac,
        folly::IPAddressV6("2620:0:1cfe:face:b00c::3"),
        folly::IPAddressV6("2620:0:1cfe:face:b00c::4"),
        8000,
        8001,
        dscpVal);

    getHwSwitch()->sendPacketSwitchedSync(std::move(txPacket));
  }

  /*
   * For congestion detection queue length of minLength = 128, and maxLength =
   * 128, a packet count of 128 has been enough to cause ECN marking. Inject
   * 128 * 2 packets to avoid test noise.
   */
  void sendUdpPkts(uint8_t dscpVal, bool isEcn, int cnt = 256) {
    for (int i = 0; i < cnt; i++) {
      sendUdpPkt(dscpVal, isEcn);
    }
  }

 protected:
  void runTest(bool isEcn) {
    if (!isSupported(HwAsic::Feature::L3_QOS)) {
#if defined(GTEST_SKIP)
      GTEST_SKIP();
#endif
      return;
    }
    auto setup = [=]() {
      auto kEcmpWidthForTest = 1;
      utility::EcmpSetupAnyNPorts6 ecmpHelper6{getProgrammedState(),
                                               getPlatform()->getLocalMac()};
      setupECMPForwarding(ecmpHelper6, kEcmpWidthForTest);
      disableTTLDecrements(ecmpHelper6);
    };
    auto verify = [=]() {
      sendUdpPkts(kDscp(), isEcn);
      auto portStats = getLatestPortStats(masterLogicalPortIds()[0]);
      auto increment = isEcn ? *portStats.outEcnCounter__ref()
                             : *portStats.wredDroppedPackets__ref();
      XLOG(DBG0) << (isEcn ? " ECN " : "WRED ") << " counter: " << increment;
      XLOG(DBG0) << "Queue watermark : "
                 << (*portStats.queueWatermarkBytes__ref())[2];
      EXPECT_GT(increment, 0);
    };

    verifyAcrossWarmBoots(setup, verify);
  }
};

TEST_F(HwAqmTest, verifyEcn) {
  runTest(true);
}

TEST_F(HwAqmTest, verifyWred) {
  runTest(false);
}
} // namespace facebook::fboss
