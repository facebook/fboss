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
#include "fboss/agent/hw/test/HwTestCoppUtils.h"
#include "fboss/agent/hw/test/HwTestPacketUtils.h"
#include "fboss/agent/hw/test/dataplane_tests/HwTestOlympicUtils.h"
#include "fboss/agent/hw/test/dataplane_tests/HwTestQosUtils.h"
#include "fboss/agent/test/EcmpSetupHelper.h"
#include "fboss/agent/test/ResourceLibUtil.h"

#include <folly/IPAddress.h>

namespace facebook::fboss {

class HwPortBandwidthTest : public HwLinkStateDependentTest {
  cfg::SwitchConfig initialConfig() const override {
    auto cfg = utility::oneL3IntfConfig(
        getHwSwitch(), masterLogicalPortIds()[0], cfg::PortLoopbackMode::MAC);

    if (isSupported(HwAsic::Feature::L3_QOS)) {
      auto streamType =
          *(getPlatform()->getAsic()->getQueueStreamTypes(false).begin());
      utility::addOlympicQueueConfig(&cfg, streamType);
      utility::addOlympicQosMaps(cfg);
      _configureBandwidth(&cfg);
    }

    return cfg;
  }

  void _configureBandwidth(cfg::SwitchConfig* config) const {
    if (isSupported(HwAsic::Feature::SCHEDULER_PPS)) {
      auto& queue0 =
          config->portQueueConfigs_ref()["queue_config"][kQueueId0()];
      queue0.portQueueRate_ref() = cfg::PortQueueRate();
      queue0.portQueueRate_ref()->set_pktsPerSec(
          utility::getRange(kMinPps(), kMaxPps()));
    }
    auto& queue1 = config->portQueueConfigs_ref()["queue_config"][kQueueId1()];
    queue1.portQueueRate_ref() = cfg::PortQueueRate();
    queue1.portQueueRate_ref()->set_kbitsPerSec(
        utility::getRange(kMinKbps(), kMaxKbps()));
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

  MacAddress dstMac() const {
    auto vlanId = utility::firstVlanID(initialConfig());
    return utility::getInterfaceMac(getProgrammedState(), vlanId);
  }

  void sendUdpPkt(uint8_t dscpVal) {
    auto vlanId = utility::firstVlanID(initialConfig());
    auto srcMac = utility::MacAddressGenerator().get(dstMac().u64NBO() + 1);
    auto txPacket = utility::makeUDPTxPacket(
        getHwSwitch(),
        vlanId,
        srcMac,
        dstMac(),
        folly::IPAddressV6("2620:0:1cfe:face:b00c::3"),
        folly::IPAddressV6("2620:0:1cfe:face:b00c::4"),
        8000,
        8001,
        static_cast<uint8_t>((dscpVal << 2)));

    getHwSwitch()->sendPacketSwitchedSync(std::move(txPacket));
  }

  void sendUdpPkts(uint8_t dscpVal, int cnt = 256) {
    for (int i = 0; i < cnt; i++) {
      sendUdpPkt(dscpVal);
    }
  }

 protected:
  uint8_t kQueueId0() const {
    return 0;
  }

  uint8_t kQueueId0Dscp() const {
    return utility::kOlympicQueueToDscp().at(kQueueId0()).front();
  }

  uint32_t kMinPps() const {
    return 0;
  }

  uint32_t kMaxPps() const {
    return 100;
  }

  uint8_t kQueueId1() const {
    return 1;
  }

  uint8_t kQueueId1Dscp() const {
    return utility::kOlympicQueueToDscp().at(kQueueId1()).front();
  }

  uint32_t kMinKbps() const {
    return 0;
  }

  uint32_t kMaxKbps() const {
    return 100000;
  }

  template <typename GetQueueOutCntT>
  void verifyRateHelper(
      const std::string& testType,
      uint8_t dscpVal,
      uint32_t maxRate,
      GetQueueOutCntT getQueueOutCntFunc);
};

template <typename GetQueueOutCntT>
void HwPortBandwidthTest::verifyRateHelper(
    const std::string& testType,
    uint8_t dscpVal,
    uint32_t maxRate,
    GetQueueOutCntT getQueueOutCntFunc) {
  if (!isSupported(HwAsic::Feature::L3_QOS)) {
    return;
  }

  auto setup = [=]() {
    auto kEcmpWidthForTest = 1;
    utility::EcmpSetupAnyNPorts6 ecmpHelper6{getProgrammedState(), dstMac()};
    setupECMPForwarding(ecmpHelper6, kEcmpWidthForTest);
    disableTTLDecrements(ecmpHelper6);
  };

  auto verify = [=]() {
    const double kVariance = 0.20; // i.e. + or -20%
    const int kRunDuration = 10;

    sendUdpPkts(dscpVal);

    auto beforeQueueOutCnt = getQueueOutCntFunc();
    sleep(kRunDuration);
    auto afterQueueOutCnt = getQueueOutCntFunc();

    auto diffCnt = afterQueueOutCnt - beforeQueueOutCnt;
    auto currCntPerSec = diffCnt / kRunDuration;
    auto lowCntPerSec = maxRate * (1 - kVariance);
    auto highCntPerSec = maxRate * (1 + kVariance);

    XLOG(DBG1) << "Test: " << testType << " before cnt: " << beforeQueueOutCnt
               << " after cnt: " << afterQueueOutCnt << " diff cnt: " << diffCnt
               << " duration: " << kRunDuration
               << " currCntPerSec: " << currCntPerSec
               << " lowCntPerSec: " << lowCntPerSec
               << " highCntPerSec: " << highCntPerSec;

    /*
     * In practice, if no pps is configured, with above flood, the
     * packets are received at a rate > 2.5Gbps
     */
    EXPECT_TRUE(
        lowCntPerSec <= currCntPerSec && currCntPerSec <= highCntPerSec);
  };

  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(HwPortBandwidthTest, VerifyPps) {
  if (!isSupported(HwAsic::Feature::SCHEDULER_PPS)) {
    return;
  }
  auto getPackets = [this]() {
    return getLatestPortStats(masterLogicalPortIds()[0])
        .get_queueOutPackets_()
        .at(kQueueId0());
  };

  verifyRateHelper("pps", kQueueId0Dscp(), kMaxPps(), getPackets);
}

TEST_F(HwPortBandwidthTest, VerifyKbps) {
  auto getKbits = [this]() {
    auto outBytes = getLatestPortStats(masterLogicalPortIds()[0])
                        .get_queueOutBytes_()
                        .at(kQueueId1());
    return (outBytes * 8) / 1000;
  };

  verifyRateHelper("kbps", kQueueId1Dscp(), kMaxKbps(), getKbits);
}

} // namespace facebook::fboss
