// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/test/AgentHwTest.h"

#include "fboss/agent/packet/PktFactory.h"
#include "fboss/agent/test/EcmpSetupHelper.h"
#include "fboss/agent/test/utils/ConfigUtils.h"
#include "fboss/agent/test/utils/CoppTestUtils.h"
#include "fboss/agent/test/utils/OlympicTestUtils.h"
#include "fboss/agent/test/utils/QosTestUtils.h"

#include "fboss/agent/HwAsicTable.h"
#include "fboss/agent/TxPacket.h"

#include "fboss/agent/test/gen-cpp2/production_features_types.h"

#include <boost/range/combine.hpp>
#include <folly/logging/xlog.h>

namespace facebook::fboss {

class AgentPortBandwidthTest : public AgentHwTest {
 public:
  std::vector<production_features::ProductionFeature>
  getProductionFeaturesVerified() const override {
    return {
        production_features::ProductionFeature::L3_QOS,
        production_features::ProductionFeature::NIF_POLICER};
  }
  cfg::SwitchConfig initialConfig(
      const AgentEnsemble& ensemble) const override {
    auto config = utility::onePortPerInterfaceConfig(
        ensemble.getSw(),
        ensemble.masterLogicalPortIds(),
        true /*interfaceHasSubnet*/);
    CHECK(ensemble.getSw());
    CHECK(ensemble.getSw()->getHwAsicTable());
    auto asics = ensemble.getSw()->getHwAsicTable()->getL3Asics();
    CHECK(!asics.empty());
    utility::checkSameAsicType(asics);
    utility::addOlympicQueueConfig(&config, asics);
    utility::addOlympicQosMaps(config, asics);
    utility::setTTLZeroCpuConfig(asics, config);
    return config;
  }

  PortID getPort0(const AgentEnsemble& ensemble, SwitchID switchID) const {
    return ensemble.masterLogicalInterfacePortIds(switchID).at(0);
  }

  PortID getPort0(SwitchID switchID = SwitchID(0)) const {
    return getPort0(*getAgentEnsemble(), switchID);
  }

  const HwAsic* getHwAsic() {
    auto asics = getAgentEnsemble()->getSw()->getHwAsicTable()->getL3Asics();
    CHECK(!asics.empty());
    return utility::checkSameAndGetAsic(asics);
  }

  void _configureBandwidth(
      cfg::SwitchConfig* config,
      uint32_t maxPps,
      uint32_t maxKbps) const {
    if (isSupportedOnAllAsics(HwAsic::Feature::SCHEDULER_PPS)) {
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

  void disableTTLDecrements(
      const utility::EcmpSetupTargetedPorts6& ecmpHelper) {
    utility::ttlDecrementHandlingForLoopbackTraffic(
        getAgentEnsemble(),
        ecmpHelper.getRouterId(),
        ecmpHelper.nhop(PortDescriptor(getPort0())));
  }

  void setupHelper() {
    utility::EcmpSetupTargetedPorts6 ecmpHelper6{
        getProgrammedState(), dstMac()};
    const auto& portDesc = PortDescriptor(getPort0());
    auto placeholder = getProgrammedState();
    this->applyNewState(
        [&](const std::shared_ptr<SwitchState>& /*state*/) {
          return ecmpHelper6.resolveNextHops(placeholder, {portDesc});
        },
        "Resolve next hops");
    RoutePrefixV6 route{kDestIp(), 128};
    auto wrapper = getSw()->getRouteUpdater();
    ecmpHelper6.programRoutes(&wrapper, {portDesc}, {route});
    disableTTLDecrements(ecmpHelper6);
  }

  void sendUdpPkt(uint8_t dscpVal, int payloadLen) {
    auto vlanId = utility::firstVlanID(getProgrammedState());
    auto srcMac = utility::MacAddressGenerator().get(dstMac().u64NBO() + 1);
    std::optional<std::vector<uint8_t>> payload = payloadLen
        ? std::vector<uint8_t>(payloadLen, 0xff)
        : std::optional<std::vector<uint8_t>>();

    auto txPacket = utility::makeUDPTxPacket(
        getAgentEnsemble()->getSw(),
        vlanId,
        srcMac,
        dstMac(),
        folly::IPAddressV6("2620:0:1cfe:face:b00c::3"),
        kDestIp(),
        8000,
        8001,
        static_cast<uint8_t>(dscpVal << 2),
        255 /* Hop limit */,
        payload);

    std::optional<PortDescriptor> port{};
    if (getHwAsic()->getAsicType() == cfg::AsicType::ASIC_TYPE_CHENAB) {
      port = PortDescriptor(getPort0());
    }
    getAgentEnsemble()->sendPacketAsync(
        std::move(txPacket), port, std::nullopt);
  }

  void sendUdpPkts(uint8_t dscpVal, int cnt = 256, int payloadLen = 0) {
    for (int i = 0; i < cnt; i++) {
      sendUdpPkt(dscpVal, payloadLen);
    }
  }

  MacAddress dstMac() const {
    return utility::getFirstInterfaceMac(getProgrammedState());
  }

  folly::IPAddressV6 kDestIp() const {
    return folly::IPAddressV6("2620:0:1cfe:face:b00c::4");
  }

  uint32_t kMinPps() const {
    return 0;
  }

  uint32_t kMinKbps() const {
    return 0;
  }
  uint8_t kQueueId0() const {
    return 0;
  }

  uint8_t kQueueId1() const {
    return 1;
  }

  std::vector<uint32_t> kMaxPpsValues() const {
    return {100, 500, 1000, 100};
  }

  std::vector<uint32_t> kMaxKbpsValues() const {
    return {100000, 500000, 1000000, 100000};
  }

  uint8_t kQueueId0Dscp() const {
    return utility::kOlympicQueueToDscp().at(kQueueId0()).front();
  }

  uint8_t kQueueId1Dscp() const {
    return utility::kOlympicQueueToDscp().at(kQueueId1()).front();
  }

  template <typename GetQueueOutCntT>
  bool verifyRateHelper(
      const std::string& testType,
      uint32_t maxRate,
      GetQueueOutCntT getQueueOutCntFunc) {
    const double kVariance = 0.20; // i.e. + or -20%
    const int kRunDuration = 10;

    auto statsBefore = getLatestPortStats(getPort0());
    auto statsAfter = statsBefore;
    auto beforeQueueOutCnt = getQueueOutCntFunc(statsBefore);
    auto duration = kRunDuration;
    WITH_RETRIES({
      statsAfter = getLatestPortStats(getPort0());
      duration = *statsAfter.timestamp_() - *statsBefore.timestamp_();
      EXPECT_EVENTUALLY_TRUE(
          (*statsAfter.timestamp_() - *statsBefore.timestamp_()) >=
          kRunDuration);
    });
    auto afterQueueOutCnt = getQueueOutCntFunc(statsAfter);
    auto diffCnt = afterQueueOutCnt - beforeQueueOutCnt;
    auto currCntPerSec = diffCnt / duration;
    auto lowCntPerSec = maxRate * (1 - kVariance);
    auto highCntPerSec = maxRate * (1 + kVariance);

    XLOG(DBG1) << "Test: " << testType << " before cnt: " << beforeQueueOutCnt
               << " after cnt: " << afterQueueOutCnt << " diff cnt: " << diffCnt
               << " duration: " << duration
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
    WITH_RETRIES_N(25, {
      EXPECT_EVENTUALLY_TRUE(
          verifyRateHelper(testType, maxRate, getQueueOutCntFunc));
    });
  }

  template <typename GetQueueOutCntT>
  int64_t getCurCntPerSec(GetQueueOutCntT getQueueOutCntFunc) {
    const int kRunDuration = 10;
    auto statsBefore = getLatestPortStats(getPort0());
    auto beforeQueueOutCnt = getQueueOutCntFunc(statsBefore);
    auto statsAfter = statsBefore;
    WITH_RETRIES({
      statsAfter = getLatestPortStats(getPort0());
      EXPECT_EVENTUALLY_TRUE(
          (*statsAfter.timestamp_() - *statsBefore.timestamp_()) >=
          kRunDuration);
    });

    auto afterQueueOutCnt = getQueueOutCntFunc(statsAfter);

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

class AgentPortBandwidthPpsTest : public AgentPortBandwidthTest {
 public:
  std::vector<production_features::ProductionFeature>
  getProductionFeaturesVerified() const override {
    auto prodFeatures = AgentPortBandwidthTest::getProductionFeaturesVerified();
    prodFeatures.push_back(
        production_features::ProductionFeature::SCHEDULER_PPS);
    return prodFeatures;
  }
};

class AgentPortBandwidthParamTest
    : public AgentPortBandwidthTest,
      public testing::WithParamInterface<cfg::PortSpeed> {};

template <typename GetQueueOutCntT>
void AgentPortBandwidthTest::verifyRate(
    const std::string& testType,
    uint8_t dscpVal,
    uint32_t maxRate,
    GetQueueOutCntT getQueueOutCntFunc) {
  // runs on asics with HwAsic::Feature::L3_QOS

  auto setup = [=, this]() {
    auto newCfg{initialConfig(*(this->getAgentEnsemble()))};
    _configureBandwidth(
        &newCfg, kMaxPpsValues().front(), kMaxKbpsValues().front());
    applyNewConfig(newCfg);
    setupHelper();
  };

  auto verify = [=, this]() {
    sendUdpPkts(dscpVal);
    EXPECT_TRUE(verifyRateHelper(testType, maxRate, getQueueOutCntFunc));
  };

  auto setupPostWB = []() {};

  auto verifyPostWB = [=, this]() {
    sendUdpPkts(dscpVal);
    EXPECT_TRUE(verifyRateHelper(testType, maxRate, getQueueOutCntFunc));

    // Put port in non-loopback mode to drain the traffic.
    // New SDK expects buffer to be empty during teardown.
    auto newCfg = utility::onePortPerInterfaceConfig(
        getAgentEnsemble()->getSw(), masterLogicalPortIds());
    for (auto& portCfg : *newCfg.ports()) {
      portCfg.loopbackMode() = cfg::PortLoopbackMode::NONE;
    }
    applyNewConfig(newCfg);
  };

  verifyAcrossWarmBoots(setup, verify, setupPostWB, verifyPostWB);
}

template <typename GetQueueOutCntT>
void AgentPortBandwidthTest::verifyRateDynamicChanges(
    const std::string& testType,
    uint8_t dscpVal,
    GetQueueOutCntT getQueueOutCntFunc) {
  // runs on asics with HwAsic::Feature::L3_QOS

  auto setup = [=, this]() { setupHelper(); };

  auto verify = [=, this]() {
    sendUdpPkts(dscpVal);

    // Rate before any bandwidth limit was configured
    auto beforeCntPerSec = getCurCntPerSec(getQueueOutCntFunc);

    const auto maxPpsValues = kMaxPpsValues(); // 100
    const auto maxKbpsValues = kMaxKbpsValues();
    for (auto maxPpsAndMaxKbps : boost::combine(maxPpsValues, maxKbpsValues)) {
      uint32_t maxPps = maxPpsAndMaxKbps.get<0>();
      uint32_t maxKbps = maxPpsAndMaxKbps.get<1>();
      XLOG(DBG1) << "Test: " << testType << " setting MaxPps: " << maxPps
                 << " MaxKbps: " << maxKbps;

      auto newCfg{initialConfig(*(this->getAgentEnsemble()))};
      _configureBandwidth(&newCfg, maxPps, maxKbps);
      applyNewConfig(newCfg);
      verifyRateHelperWithRetries(
          testType, testType == "pps" ? maxPps : maxKbps, getQueueOutCntFunc);
    }

    // Clear bandwidth setting
    auto newCfg{initialConfig(*(this->getAgentEnsemble()))};
    applyNewConfig(newCfg);

    // Verify if the rate is restored to pre bandwidth configuration
    verifyRateHelperWithRetries(testType, beforeCntPerSec, getQueueOutCntFunc);
  };

  verifyAcrossWarmBoots(setup, verify);
}

void AgentPortBandwidthTest::verifyQueueShaper() {
  constexpr auto kMhnicPerHostBandwidthKbps{
      static_cast<uint64_t>(cfg::PortSpeed::TWENTYFIVEG) * 1000};

  auto setup = [=, this]() {
    auto newCfg{initialConfig(*(this->getAgentEnsemble()))};
    auto isVoq =
        (getSw()->getSwitchInfoTable().l3SwitchType() == cfg::SwitchType::VOQ);
    utility::addQueueShaperConfig(
        &newCfg, kQueueId0(), 0, kMhnicPerHostBandwidthKbps);
    utility::addQueueBurstSizeConfig(
        &newCfg,
        kQueueId0(),
        utility::kQueueConfigBurstSizeMinKb,
        utility::kQueueConfigBurstSizeMaxKb);
    utility::addQueueWredConfig(
        &newCfg,
        this->getAgentEnsemble()->getL3Asics(),
        kQueueId0(),
        utility::kQueueConfigAqmsWredThresholdMinMax,
        utility::kQueueConfigAqmsWredThresholdMinMax,
        utility::kQueueConfigAqmsWredDropProbability,
        isVoq);
    utility::addQueueEcnConfig(
        &newCfg,
        this->getAgentEnsemble()->getL3Asics(),
        kQueueId0(),
        utility::kQueueConfigAqmsEcnThresholdMinMax,
        utility::kQueueConfigAqmsEcnThresholdMinMax,
        isVoq);
    applyNewConfig(newCfg);
    setupHelper();
  };

  auto verify = [=, this]() {
    constexpr auto kPayloadLength{1200};
    constexpr auto kWaitTimeForSpecificRate{30};
    auto pktsToSend = getAgentEnsemble()->getMinPktsForLineRate(getPort0());
    sendUdpPkts(kQueueId0Dscp(), pktsToSend, kPayloadLength);
    EXPECT_NO_THROW(getAgentEnsemble()->waitForSpecificRateOnPort(
        getPort0(),
        kMhnicPerHostBandwidthKbps * 1000, // BW in bps
        kWaitTimeForSpecificRate));
    // This means the queue rate is >= kMhnicPerHostBandwidthKbps, now
    // confirm that we are not exceeding an upper limit. This is hard to
    // generalize, so lets keep this at 4% greater than the desired rate.
    const int kTrafficRateCheckDurationSec{5};
    const double kAcceptableTrafficRateDeltaPct{4};
    auto stats1 = getLatestPortStats(getPort0());
    sleep(kTrafficRateCheckDurationSec);
    auto stats2 = getLatestPortStats(getPort0());
    auto trafficRate = getAgentEnsemble()->getTrafficRate(
        stats1, stats2, kTrafficRateCheckDurationSec);
    // Make sure that the port rate is not exceeding expected rate by 4%
    uint64_t allowedMaxTrafficRate =
        (1 + kAcceptableTrafficRateDeltaPct / 100) *
        kMhnicPerHostBandwidthKbps * 1000;
    XLOG(DBG0) << "Shaper rate : " << kMhnicPerHostBandwidthKbps * 1000
               << " bps. Rate seen on port: " << trafficRate
               << " bps. Max acceptable rate: " << allowedMaxTrafficRate
               << " bps!";
    EXPECT_LT(trafficRate, allowedMaxTrafficRate);
  };

  verifyAcrossWarmBoots(setup, verify);
}

void AgentPortBandwidthTest::verifyPortRateTraffic(cfg::PortSpeed portSpeed) {
  auto setup = [&]() {
    auto port = getProgrammedState()->getPorts()->getNodeIf(getPort0());
    if (port->getSpeed() != portSpeed ||
        getHwAsic()->getAsicType() != cfg::AsicType::ASIC_TYPE_CHENAB) {
      auto newCfg{initialConfig(*(this->getAgentEnsemble()))};
      utility::configurePortGroup(
          getAgentEnsemble()->getPlatformMapping(),
          getAgentEnsemble()->getSw()->getPlatformSupportsAddRemovePort(),
          newCfg,
          portSpeed,
          utility::getAllPortsInGroup(
              getAgentEnsemble()->getPlatformMapping(), getPort0()));
      XLOG(DBG0) << "Port " << getPort0() << " speed set to "
                 << static_cast<int>(portSpeed) << " bps";
      applyNewConfig(newCfg);
    }
    setupHelper();

    auto pktsToSend = getAgentEnsemble()->getMinPktsForLineRate(getPort0());
    sendUdpPkts(kQueueId0Dscp(), pktsToSend);
  };

  auto verify = [&]() {
    EXPECT_NO_THROW(getAgentEnsemble()->waitForLineRateOnPort(getPort0()));
  };

  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(AgentPortBandwidthTest, VerifyKbps) {
  auto getKbits = [this](const HwPortStats& stats) {
    auto outBytes = stats.get_queueOutBytes_().at(kQueueId1());
    return (outBytes * 8) / 1000;
  };

  verifyRate("kbps", kQueueId1Dscp(), kMaxKbpsValues().front(), getKbits);
}

TEST_F(AgentPortBandwidthTest, VerifyKbpsDynamicChanges) {
  auto getKbits = [this](const HwPortStats& stats) {
    auto outBytes = stats.get_queueOutBytes_().at(kQueueId1());
    return (outBytes * 8) / 1000;
  };

  verifyRateDynamicChanges("kbps", kQueueId1Dscp(), getKbits);
}

TEST_F(AgentPortBandwidthTest, VerifyQueueShaper) {
  // runs on all platforms that support L3_QOS
  verifyQueueShaper();
}

TEST_P(AgentPortBandwidthParamTest, VerifyPortRateTraffic) {
  cfg::PortSpeed portSpeed = static_cast<cfg::PortSpeed>(GetParam());
  auto profileID =
      getAgentEnsemble()->getSw()->getPlatformMapping()->getProfileIDBySpeedIf(
          getPort0(), portSpeed);
  if (!profileID.has_value()) {
    XLOG(DBG0) << "No profile supporting speed " << static_cast<int>(portSpeed)
               << " for the this platform, skipping test!";
    GTEST_SKIP();
  }
  verifyPortRateTraffic(portSpeed);
}

TEST_F(AgentPortBandwidthPpsTest, VerifyPps) {
  auto getPackets = [this](const HwPortStats& stats) {
    return stats.get_queueOutPackets_().at(kQueueId0());
  };

  verifyRate("pps", kQueueId0Dscp(), kMaxPpsValues().front(), getPackets);
}

TEST_F(AgentPortBandwidthPpsTest, VerifyPpsDynamicChanges) {
  auto getPackets = [this](const HwPortStats& stats) {
    return stats.get_queueOutPackets_().at(kQueueId0());
  };

  verifyRateDynamicChanges("pps", kQueueId0Dscp(), getPackets);
}

INSTANTIATE_TEST_CASE_P(
    AgentPortBandwidthTest,
    AgentPortBandwidthParamTest,
    ::testing::Values(
        cfg::PortSpeed::FORTYG,
        cfg::PortSpeed::HUNDREDG,
        cfg::PortSpeed::TWOHUNDREDG,
        cfg::PortSpeed::FOURHUNDREDG));

} // namespace facebook::fboss
