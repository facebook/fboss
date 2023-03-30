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

#include <boost/range/combine.hpp>
#include <folly/IPAddress.h>

namespace facebook::fboss {

class HwPortBandwidthTest : public HwLinkStateDependentTest {
  cfg::SwitchConfig initialConfig() const override {
    auto cfg = utility::oneL3IntfConfig(
        getHwSwitch(),
        masterLogicalPortIds()[0],
        getAsic()->desiredLoopbackMode());

    if (isSupported(HwAsic::Feature::L3_QOS)) {
      auto streamType =
          *(getPlatform()
                ->getAsic()
                ->getQueueStreamTypes(cfg::PortType::INTERFACE_PORT)
                .begin());
      utility::addOlympicQueueConfig(
          &cfg, streamType, getPlatform()->getAsic());
      utility::addOlympicQosMaps(cfg, getPlatform()->getAsic());
    }

    return cfg;
  }

  void _configureBandwidth(
      cfg::SwitchConfig* config,
      uint32_t maxPps,
      uint32_t maxKbps) const {
    if (isSupported(HwAsic::Feature::SCHEDULER_PPS)) {
      auto& queue0 = config->portQueueConfigs()["queue_config"][kQueueId0()];
      queue0.portQueueRate() = cfg::PortQueueRate();
      queue0.portQueueRate()->pktsPerSec_ref() =
          utility::getRange(kMinPps(), maxPps);
    }
    auto& queue1 = config->portQueueConfigs()["queue_config"][kQueueId1()];
    queue1.portQueueRate() = cfg::PortQueueRate();
    queue1.portQueueRate()->kbitsPerSec_ref() =
        utility::getRange(kMinKbps(), maxKbps);
  }

  template <typename ECMP_HELPER>
  void disableTTLDecrements(const ECMP_HELPER& ecmpHelper) {
    for (const auto& nextHop : ecmpHelper.getNextHops()) {
      utility::disableTTLDecrements(
          getHwSwitch(), ecmpHelper.getRouterId(), nextHop);
    }
  }

  MacAddress dstMac() const {
    return utility::getFirstInterfaceMac(initialConfig());
  }

  void sendUdpPkt(uint8_t dscpVal, int payloadLen) {
    auto vlanId = utility::firstVlanID(initialConfig());
    auto srcMac = utility::MacAddressGenerator().get(dstMac().u64NBO() + 1);
    std::optional<std::vector<uint8_t>> payload = payloadLen
        ? std::vector<uint8_t>(payloadLen, 0xff)
        : std::optional<std::vector<uint8_t>>();
    auto txPacket = utility::makeUDPTxPacket(
        getHwSwitch(),
        vlanId,
        srcMac,
        dstMac(),
        folly::IPAddressV6("2620:0:1cfe:face:b00c::3"),
        folly::IPAddressV6("2620:0:1cfe:face:b00c::4"),
        8000,
        8001,
        static_cast<uint8_t>(dscpVal << 2),
        255 /* Hop limit */,
        payload);

    getHwSwitch()->sendPacketSwitchedSync(std::move(txPacket));
  }

  void sendUdpPkts(uint8_t dscpVal, int cnt = 256, int payloadLen = 0) {
    for (int i = 0; i < cnt; i++) {
      sendUdpPkt(dscpVal, payloadLen);
    }
  }

  void setupHelper() {
    auto kEcmpWidthForTest = 1;
    utility::EcmpSetupAnyNPorts6 ecmpHelper6{getProgrammedState(), dstMac()};
    resolveNeigborAndProgramRoutes(ecmpHelper6, kEcmpWidthForTest);
    disableTTLDecrements(ecmpHelper6);
  }

 protected:
  uint8_t kQueueId0() const {
    return 0;
  }

  uint8_t kQueueId0Dscp(const HwAsic* hwAsic) const {
    return utility::kOlympicQueueToDscp(hwAsic).at(kQueueId0()).front();
  }

  uint32_t kMinPps() const {
    return 0;
  }

  std::vector<uint32_t> kMaxPpsValues() const {
    return {100, 500, 1000, 100};
  }

  uint8_t kQueueId1() const {
    return 1;
  }

  uint8_t kQueueId1Dscp(const HwAsic* hwAsic) const {
    return utility::kOlympicQueueToDscp(hwAsic).at(kQueueId1()).front();
  }

  uint32_t kMinKbps() const {
    return 0;
  }

  std::vector<uint32_t> kMaxKbpsValues() const {
    return {100000, 500000, 1000000, 100000};
  }

  template <typename GetQueueOutCntT>
  bool verifyRateHelper(
      const std::string& testType,
      uint32_t maxRate,
      GetQueueOutCntT getQueueOutCntFunc) {
    const double kVariance = 0.20; // i.e. + or -20%
    const int kRunDuration = 10;

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
    return lowCntPerSec <= currCntPerSec && currCntPerSec <= highCntPerSec;
  }

  template <typename GetQueueOutCntT>
  void verifyRateHelperWithRetries(
      const std::string& testType,
      uint32_t maxRate,
      GetQueueOutCntT getQueueOutCntFunc) {
    bool rateIsCorrect = false;
    int retries = 5;
    while (retries--) {
      rateIsCorrect = verifyRateHelper(testType, maxRate, getQueueOutCntFunc);
      if (rateIsCorrect) {
        break;
      }
      XLOG(DBG2) << " Retrying ...";
      sleep(5);
    }

    EXPECT_TRUE(rateIsCorrect);
  }

  template <typename GetQueueOutCntT>
  int64_t getCurCntPerSec(GetQueueOutCntT getQueueOutCntFunc) {
    const int kRunDuration = 10;
    auto beforeQueueOutCnt = getQueueOutCntFunc();
    sleep(kRunDuration);
    auto afterQueueOutCnt = getQueueOutCntFunc();

    return (afterQueueOutCnt - beforeQueueOutCnt) / kRunDuration;
  }

  template <typename GetQueueOutCntT>
  void verifyRate(
      const std::string& testType,
      uint8_t dscpVal,
      uint32_t maxRate,
      GetQueueOutCntT getQueueOutCntFunc);

  template <typename GetQueueOutCntT>
  void verifyRateDynamicChanges(
      const std::string& testType,
      uint8_t dscpVal,
      GetQueueOutCntT getQueueOutCntFunc);

  void verifyQueueShaper();
  void verifyPortRateTraffic(cfg::PortSpeed portSpeed);
};
class HwPortBandwidthParamTest
    : public HwPortBandwidthTest,
      public testing::WithParamInterface<cfg::PortSpeed> {};

template <typename GetQueueOutCntT>
void HwPortBandwidthTest::verifyRate(
    const std::string& testType,
    uint8_t dscpVal,
    uint32_t maxRate,
    GetQueueOutCntT getQueueOutCntFunc) {
  if (!isSupported(HwAsic::Feature::L3_QOS)) {
    return;
  }

  auto setup = [=]() {
    auto newCfg{initialConfig()};
    _configureBandwidth(
        &newCfg, kMaxPpsValues().front(), kMaxKbpsValues().front());
    applyNewConfig(newCfg);

    setupHelper();
  };

  auto verify = [=]() {
    sendUdpPkts(dscpVal);
    EXPECT_TRUE(verifyRateHelper(testType, maxRate, getQueueOutCntFunc));
  };

  auto setupPostWB = [=]() {};

  auto verifyPostWB = [=]() {
    sendUdpPkts(dscpVal);
    EXPECT_TRUE(verifyRateHelper(testType, maxRate, getQueueOutCntFunc));

    // Put port in non-loopback mode to drain the traffic.
    // New SDK expects buffer to be empty during teardown.
    auto newCfg = utility::oneL3IntfConfig(
        getHwSwitch(), masterLogicalPortIds()[0], cfg::PortLoopbackMode::NONE);
    applyNewConfig(newCfg);
  };

  verifyAcrossWarmBoots(setup, verify, setupPostWB, verifyPostWB);
}

template <typename GetQueueOutCntT>
void HwPortBandwidthTest::verifyRateDynamicChanges(
    const std::string& testType,
    uint8_t dscpVal,
    GetQueueOutCntT getQueueOutCntFunc) {
  if (!isSupported(HwAsic::Feature::L3_QOS)) {
    return;
  }

  auto setup = [=]() { setupHelper(); };

  auto verify = [=]() {
    sendUdpPkts(dscpVal);

    // Rate before any bandwidth limit was configured
    auto beforeCntPerSec = getCurCntPerSec(getQueueOutCntFunc);

    const auto maxPpsValues = kMaxPpsValues();
    const auto maxKbpsValues = kMaxKbpsValues();
    for (auto maxPpsAndMaxKbps : boost::combine(maxPpsValues, maxKbpsValues)) {
      uint32_t maxPps = maxPpsAndMaxKbps.get<0>();
      uint32_t maxKbps = maxPpsAndMaxKbps.get<1>();
      XLOG(DBG1) << "Test: " << testType << " setting MaxPps: " << maxPps
                 << " MaxKbps: " << maxKbps;

      auto newCfg{initialConfig()};
      _configureBandwidth(&newCfg, maxPps, maxKbps);
      applyNewConfig(newCfg);

      verifyRateHelperWithRetries(
          testType, testType == "pps" ? maxPps : maxKbps, getQueueOutCntFunc);
    }

    // Clear bandwidth setting
    auto newCfg{initialConfig()};
    applyNewConfig(newCfg);

    // Verify if the rate is restored to pre bandwidth configuration
    verifyRateHelperWithRetries(testType, beforeCntPerSec, getQueueOutCntFunc);
  };

  verifyAcrossWarmBoots(setup, verify);
}

void HwPortBandwidthTest::verifyQueueShaper() {
  constexpr auto kMhnicPerHostBandwidthKbps{
      static_cast<uint64_t>(cfg::PortSpeed::TWENTYFIVEG) * 1000};

  auto setup = [=]() {
    auto newCfg{initialConfig()};
    utility::addQueueShaperConfig(
        &newCfg, kQueueId0(), 0, kMhnicPerHostBandwidthKbps);
    utility::addQueueBurstSizeConfig(
        &newCfg,
        kQueueId0(),
        utility::kQueueConfigBurstSizeMinKb,
        utility::kQueueConfigBurstSizeMaxKb);
    utility::addQueueWredConfig(
        &newCfg,
        kQueueId0(),
        utility::kQueueConfigAqmsWredThresholdMinMax,
        utility::kQueueConfigAqmsWredThresholdMinMax,
        utility::kQueueConfigAqmsWredDropProbability);
    utility::addQueueEcnConfig(
        &newCfg,
        kQueueId0(),
        utility::kQueueConfigAqmsEcnThresholdMinMax,
        utility::kQueueConfigAqmsEcnThresholdMinMax);
    applyNewConfig(newCfg);
    setupHelper();
  };

  auto verify = [=]() {
    constexpr auto kPayloadLength{1200};
    constexpr auto kWaitTimeForSpecificRate{30};
    auto pktsToSend =
        getHwSwitchEnsemble()->getMinPktsForLineRate(masterLogicalPortIds()[0]);
    sendUdpPkts(kQueueId0Dscp(getAsic()), pktsToSend, kPayloadLength);
    EXPECT_NO_THROW(getHwSwitchEnsemble()->waitForSpecificRateOnPort(
        masterLogicalPortIds()[0],
        kMhnicPerHostBandwidthKbps * 1000, // BW in bps
        kWaitTimeForSpecificRate));
  };

  verifyAcrossWarmBoots(setup, verify);
}

void HwPortBandwidthTest::verifyPortRateTraffic(cfg::PortSpeed portSpeed) {
  auto setup = [&]() {
    auto newCfg{initialConfig()};
    utility::configurePortGroup(
        *(getHwSwitchEnsemble()->getHwSwitch()),
        newCfg,
        portSpeed,
        getAllPortsInGroup(masterLogicalPortIds()[0]));
    XLOG(DBG0) << "Port " << masterLogicalPortIds()[0] << " speed set to "
               << static_cast<int>(portSpeed) << " bps";
    applyNewConfig(newCfg);
    setupHelper();

    auto pktsToSend =
        getHwSwitchEnsemble()->getMinPktsForLineRate(masterLogicalPortIds()[0]);
    sendUdpPkts(kQueueId0Dscp(getAsic()), pktsToSend);
  };

  auto verify = [&]() {
    EXPECT_NO_THROW(getHwSwitchEnsemble()->waitForLineRateOnPort(
        masterLogicalPortIds()[0]));
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

  verifyRate(
      "pps", kQueueId0Dscp(getAsic()), kMaxPpsValues().front(), getPackets);
}

TEST_F(HwPortBandwidthTest, VerifyKbps) {
  auto getKbits = [this]() {
    auto outBytes = getLatestPortStats(masterLogicalPortIds()[0])
                        .get_queueOutBytes_()
                        .at(kQueueId1());
    return (outBytes * 8) / 1000;
  };

  verifyRate(
      "kbps", kQueueId1Dscp(getAsic()), kMaxKbpsValues().front(), getKbits);
}

TEST_F(HwPortBandwidthTest, VerifyPpsDynamicChanges) {
  if (!isSupported(HwAsic::Feature::SCHEDULER_PPS)) {
    return;
  }
  auto getPackets = [this]() {
    return getLatestPortStats(masterLogicalPortIds()[0])
        .get_queueOutPackets_()
        .at(kQueueId0());
  };

  verifyRateDynamicChanges("pps", kQueueId0Dscp(getAsic()), getPackets);
}

TEST_F(HwPortBandwidthTest, VerifyKbpsDynamicChanges) {
  auto getKbits = [this]() {
    auto outBytes = getLatestPortStats(masterLogicalPortIds()[0])
                        .get_queueOutBytes_()
                        .at(kQueueId1());
    return (outBytes * 8) / 1000;
  };

  verifyRateDynamicChanges("kbps", kQueueId1Dscp(getAsic()), getKbits);
}

TEST_F(HwPortBandwidthTest, VerifyQueueShaper) {
  if (!isSupported(HwAsic::Feature::L3_QOS)) {
    return;
  }

  verifyQueueShaper();
}

TEST_P(HwPortBandwidthParamTest, VerifyPortRateTraffic) {
  cfg::PortSpeed portSpeed = static_cast<cfg::PortSpeed>(GetParam());
  auto platformPort =
      getHwSwitch()->getPlatform()->getPlatformPort(masterLogicalPortIds()[0]);
  auto profileID = platformPort->getProfileIDBySpeedIf(portSpeed);
  if (!profileID.has_value()) {
    XLOG(DBG0) << "No profile supporting speed " << static_cast<int>(portSpeed)
               << " for the this platform, skipping test!";
#if defined(GTEST_SKIP)
    GTEST_SKIP();
#endif
    return;
  }
  verifyPortRateTraffic(portSpeed);
}
INSTANTIATE_TEST_CASE_P(
    HwPortBandwidthTest,
    HwPortBandwidthParamTest,
    ::testing::Values(
        cfg::PortSpeed::FORTYG,
        cfg::PortSpeed::HUNDREDG,
        cfg::PortSpeed::TWOHUNDREDG));
} // namespace facebook::fboss
