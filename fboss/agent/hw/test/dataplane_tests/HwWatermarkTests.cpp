/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include <fb303/ServiceData.h>
#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/hw/test/HwLinkStateDependentTest.h"
#include "fboss/agent/hw/test/HwTestPacketUtils.h"
#include "fboss/agent/hw/test/dataplane_tests/HwTestOlympicUtils.h"
#include "fboss/agent/hw/test/dataplane_tests/HwTestQosUtils.h"
#include "fboss/agent/state/Port.h"
#include "fboss/agent/test/EcmpSetupHelper.h"

#include <folly/IPAddress.h>

namespace facebook::fboss {

class HwWatermarkTest : public HwLinkStateDependentTest {
 private:
  cfg::SwitchConfig initialConfig() const override {
    auto cfg =
        utility::onePortPerVlanConfig(getHwSwitch(), masterLogicalPortIds());
    if (isSupported(HwAsic::Feature::L3_QOS)) {
      auto streamType =
          *(getPlatform()->getAsic()->getQueueStreamTypes(false).begin());
      utility::addOlympicQueueConfig(
          &cfg, streamType, getPlatform()->getAsic());
      utility::addOlympicQosMaps(cfg);
    }
    return cfg;
  }

  MacAddress kDstMac() const {
    return getPlatform()->getLocalMac();
  }
  void sendUdpPkt(uint8_t dscpVal, const folly::IPAddressV6& dst) {
    auto vlanId = utility::firstVlanID(initialConfig());
    auto intfMac = utility::getInterfaceMac(getProgrammedState(), vlanId);
    auto srcMac = utility::MacAddressGenerator().get(intfMac.u64NBO() + 1);

    auto txPacket = utility::makeUDPTxPacket(
        getHwSwitch(),
        vlanId,
        srcMac,
        intfMac,
        folly::IPAddressV6("2620:0:1cfe:face:b00c::3"),
        dst,
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
      auto portName =
          getProgrammedState()->getPorts()->getPort(port)->getName();
      XLOG(DBG0) << "Port: " << portName << " queueId: " << queueId
                 << " Watermark: " << queueWaterMarks[queueId];

      auto watermarkAsExpected = (expectZero && !queueWaterMarks[queueId]) ||
          (!expectZero && queueWaterMarks[queueId]);
      if (watermarkAsExpected) {
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
  std::map<PortID, folly::IPAddressV6> getPort2DstIp() const {
    return {
        {masterLogicalPortIds()[0], kDestIp1()},
        {masterLogicalPortIds()[1], kDestIp2()},
    };
  }

 protected:
  folly::IPAddressV6 kDestIp1() const {
    return folly::IPAddressV6("2620:0:1cfe:face:b00c::4");
  }
  folly::IPAddressV6 kDestIp2() const {
    return folly::IPAddressV6("2620:0:1cfe:face:b00c::5");
  }

  /*
   * In practice, sending single packet usually (but not always) returned BST
   * value > 0 (usually 2, but  other times it was 0). Thus, send a large
   * number of packets to try to avoid the risk of flakiness.
   */
  void
  sendUdpPkts(uint8_t dscpVal, const folly::IPAddressV6& dstIp, int cnt = 100) {
    for (int i = 0; i < cnt; i++) {
      sendUdpPkt(dscpVal, dstIp);
    }
  }
  void programRoutes() {
    for (auto portAndIp : getPort2DstIp()) {
      auto portDesc = PortDescriptor(portAndIp.first);
      utility::EcmpSetupTargetedPorts6 ecmpHelper6{getProgrammedState()};
      applyNewState(
          ecmpHelper6.resolveNextHops(getProgrammedState(), {portDesc}));
      ecmpHelper6.programRoutes(
          getRouteUpdater(),
          {portDesc},
          {Route<folly::IPAddressV6>::Prefix{portAndIp.second, 128}});
    }
  }

  void _setup(bool disableTtlDecrement = false) {
    auto vlanId = utility::firstVlanID(initialConfig());
    auto intfMac = utility::getInterfaceMac(getProgrammedState(), vlanId);
    auto kEcmpWidthForTest = 1;
    utility::EcmpSetupAnyNPorts6 ecmpHelper6{getProgrammedState(), intfMac};
    resolveNeigborAndProgramRoutes(ecmpHelper6, kEcmpWidthForTest);
    if (disableTtlDecrement) {
      utility::disableTTLDecrements(
          getHwSwitch(),
          ecmpHelper6.getRouterId(),
          ecmpHelper6.getNextHops()[0]);
    }
  }

  void assertDeviceWatermark(bool expectZero, int retries = 1) {
    EXPECT_TRUE(gotExpectedDeviceWatermark(expectZero, retries));
  }
  void runTest(int queueId) {
    if (!isSupported(HwAsic::Feature::L3_QOS)) {
      return;
    }

    auto setup = [this]() { programRoutes(); };
    auto verify = [this, queueId]() {
      auto dscpsForQueue = utility::kOlympicQueueToDscp().find(queueId)->second;
      for (auto portAndIp : getPort2DstIp()) {
        auto portName = getProgrammedState()
                            ->getPorts()
                            ->getPort(portAndIp.first)
                            ->getName();
        sendUdpPkts(dscpsForQueue[0], portAndIp.second);
        // Assert non zero watermark
        assertWatermark(portAndIp.first, queueId, false);
        // Assert zero watermark
        assertWatermark(portAndIp.first, queueId, true, 5);
        auto counters = fb303::fbData->getRegexCounters({folly::sformat(
            "buffer_watermark_ucast.{}.queue{}.*.p100.60", portName, queueId)});
        EXPECT_EQ(1, counters.size());
        // Unfortunately since  we use quantile stats, which compute
        // a MAX over a period, we can't really assert on the exact
        // value, just on its presence
      }
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
  auto setup = [this]() { _setup(); };
  auto verify = [this]() {
    sendUdpPkts(0, kDestIp1());
    // Assert non zero watermark
    assertDeviceWatermark(false);
    // Assert zero watermark
    assertDeviceWatermark(true, 5);
    auto counters =
        fb303::fbData->getSelectedCounters({"buffer_watermark_device.p100.60"});
    EXPECT_EQ(1, counters.size());
    // Unfortunately since  we use quantile stats, which compute
    // a MAX over a period, we can't really assert on the exact
    // value, just on its presence
  };
  verifyAcrossWarmBoots(setup, verify);
}

} // namespace facebook::fboss
