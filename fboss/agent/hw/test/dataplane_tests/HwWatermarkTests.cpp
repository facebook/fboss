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

#include <folly/IPAddress.h>

namespace facebook::fboss {

class HwWatermarkTest : public HwLinkStateDependentTest {
 private:
  cfg::SwitchConfig initialConfig() const override {
    auto cfg = utility::oneL3IntfConfig(
        getHwSwitch(), masterLogicalPortIds()[0], cfg::PortLoopbackMode::MAC);
    utility::addOlympicQosMaps(cfg);
    return cfg;
  }

  MacAddress kDstMac() const {
    return getPlatform()->getLocalMac();
  }

  void sendUdpPkt(uint8_t dscpVal) {
    auto vlanId = utility::firstVlanID(initialConfig());
    auto intfMac = utility::getInterfaceMac(getProgrammedState(), vlanId);

    auto txPacket = utility::makeUDPTxPacket(
        getHwSwitch(),
        vlanId,
        intfMac,
        intfMac,
        folly::IPAddressV6("2620:0:1cfe:face:b00c::3"),
        folly::IPAddressV6("2620:0:1cfe:face:b00c::4"),
        8000,
        8001,
        // Trailing 2 bits are for ECN
        static_cast<uint8_t>(dscpVal << 2),
        255,
        std::vector<uint8_t>(6000, 0xff));
    getHwSwitch()->sendPacketSwitchedSync(std::move(txPacket));
  }

  void
  assertWatermark(PortID port, int queueId, bool expectZero, int retries = 1) {
    EXPECT_TRUE(gotExpectedWatermark(port, queueId, expectZero, retries));
  }

  bool
  gotExpectedWatermark(PortID port, int queueId, bool expectZero, int retries) {
    do {
      auto queueWaterMarks = *getHwSwitchEnsemble()
                                  ->getLatestPortStats(port)
                                  .queueWatermarkBytes__ref();
      XLOG(DBG0) << "queueId: " << queueId
                 << " Watermark: " << queueWaterMarks[queueId];
      auto watermarkAsExpected = (expectZero && !queueWaterMarks[queueId]) ||
          (!expectZero && queueWaterMarks[queueId]);
      if (!retries || watermarkAsExpected) {
        return true;
      }
      XLOG(DBG0) << " Retry ...";
      sleep(1);
    } while (--retries > 0);
    XLOG(INFO) << " Did not get expected watermark value";
    return false;
  }

  bool gotExpectedDeviceWatermark(bool expectZero, int retries) {
    do {
      getHwSwitchEnsemble()->getLatestPortStats(masterLogicalPortIds()[0]);
      auto deviceWatermarkBytes =
          getHwSwitchEnsemble()->getHwSwitch()->getDeviceWatermarkBytes();
      XLOG(DBG0) << "Device watermark bytes: " << deviceWatermarkBytes;
      auto watermarkAsExpected = (expectZero && !deviceWatermarkBytes) ||
          (!expectZero && deviceWatermarkBytes);
      if (watermarkAsExpected) {
        return true;
      }
      XLOG(DBG0) << " Retry ...";
      sleep(1);
    } while (--retries > 0);

    XLOG(INFO) << " Did not get expected device watermark value";
    return false;
  }

 protected:
  /*
   * In practice, sending single packet usually (but not always) returned BST
   * value > 0 (usually 2, but  other times it was 0). Thus, send a large
   * number of packets to try to avoid the risk of flakiness.
   */
  void sendUdpPkts(uint8_t dscpVal, int cnt = 100) {
    for (int i = 0; i < cnt; i++) {
      sendUdpPkt(dscpVal);
    }
  }
  template <typename ECMP_HELPER>
  std::shared_ptr<SwitchState> setupECMPForwarding(
      const ECMP_HELPER& ecmpHelper) {
    auto kEcmpWidthForTest = 1;
    auto newState = ecmpHelper.setupECMPForwarding(
        ecmpHelper.resolveNextHops(getProgrammedState(), kEcmpWidthForTest),
        kEcmpWidthForTest);
    applyNewState(newState);
    return getProgrammedState();
  }
  void assertDeviceWatermark(bool expectZero, int retries = 1) {
    EXPECT_TRUE(gotExpectedDeviceWatermark(expectZero, retries));
  }
  void runTest(int queueId) {
    if (!isSupported(HwAsic::Feature::L3_QOS)) {
      return;
    }

    auto setup = [this]() {
      utility::EcmpSetupAnyNPorts6 ecmpHelper6{getProgrammedState()};
      setupECMPForwarding(ecmpHelper6);
    };
    auto verify = [this, queueId]() {
      auto dscpsForQueue = utility::kOlympicQueueToDscp().find(queueId)->second;
      sendUdpPkts(dscpsForQueue[0]);
      // Assert non zero watermark
      assertWatermark(masterLogicalPortIds()[0], queueId, false);
      // Assert zero watermark
      assertWatermark(masterLogicalPortIds()[0], queueId, true, 5);
    };

    verifyAcrossWarmBoots(setup, verify);
  }
};

TEST_F(HwWatermarkTest, VerifyDefaultQueue) {
  runTest(utility::kOlympicSilverQueueId);
}

TEST_F(HwWatermarkTest, VerifyNonDefaultQueue) {
  runTest(utility::kOlympicGoldQueueId);
}

// TODO - merge device watermark checking into the tests
// above once all platforms support device watermarks
TEST_F(HwWatermarkTest, VerifyDeviceWatermark) {
  auto setup = [this]() {
    utility::EcmpSetupAnyNPorts6 ecmpHelper6{getProgrammedState()};
    setupECMPForwarding(ecmpHelper6);
  };
  auto verify = [this]() {
    sendUdpPkts(0);
    // Assert non zero watermark
    assertDeviceWatermark(false);
    // Assert zero watermark
    assertDeviceWatermark(true, 5);
  };
  verifyAcrossWarmBoots(setup, verify);
}

} // namespace facebook::fboss
