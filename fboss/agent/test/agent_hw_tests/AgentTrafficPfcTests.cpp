// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <folly/MapUtil.h>

#include "fboss/agent/TxPacket.h"
#include "fboss/agent/packet/PktFactory.h"
#include "fboss/agent/test/AgentHwTest.h"
#include "fboss/agent/test/EcmpSetupHelper.h"
#include "fboss/agent/test/utils/ConfigUtils.h"
#include "fboss/agent/test/utils/PfcTestUtils.h"
#include "fboss/agent/test/utils/QosTestUtils.h"
#include "fboss/lib/CommonUtils.h"

DEFINE_bool(
    skip_stop_pfc_test_traffic,
    false,
    "Skip stopping traffic after traffic test!");

namespace {

using facebook::fboss::utility::PfcBufferParams;
using facebook::fboss::utility::setupPfcBuffers;

static constexpr auto kGlobalSharedBytes{20000};
static constexpr auto kPgLimitBytes{2200};
static constexpr auto kPgResumeOffsetBytes{1800};
static constexpr auto kGlobalIngressEgressBufferPoolSize{
    5064760}; // Keep a high pool size for DNX
static constexpr auto kLosslessTrafficClass{2};
static constexpr auto kLosslessPriority{2};
static const std::vector<int> kLosslessPgIds{2, 3};
// Hardcoding register value to force PFC generation for port 2,
// kLosslessPriority for DNX. This needs to be modified if the
// port used in test or PFC priority changes! Details of how to
// compute the value to use for a port/priority is captured in
// CS00012321021. To summarize,
//    value = 1 << ((port_first_phy - core_first_phy)*8 + priority,
// with port_first_phy from "port management dump full port=<>",
// core_first_phy from "dnx data dump nif.phys.nof_phys_per_core".
static const std::
    map<std::tuple<facebook::fboss::cfg::AsicType, int, int>, std::string>
        kRegValToForcePfcTxForPriorityOnPortDnx = {
            {std::make_tuple(
                 facebook::fboss::cfg::AsicType::ASIC_TYPE_JERICHO2,
                 2,
                 2),
             "0x40000000000000000"},
            {std::make_tuple(
                 facebook::fboss::cfg::AsicType::ASIC_TYPE_JERICHO3,
                 8,
                 2),
             "4"},
};

struct TrafficTestParams {
  std::string name;
  PfcBufferParams buffer = PfcBufferParams{};
  bool expectDrop = false;
  bool scale = false;
};

std::tuple<int, int, int> getPfcTxRxXonHwPortStats(
    facebook::fboss::AgentEnsemble* ensemble,
    const facebook::fboss::HwPortStats& portStats,
    const int pfcPriority) {
  return {
      folly::get_default(portStats.get_outPfc_(), pfcPriority, 0),
      folly::get_default(portStats.get_inPfc_(), pfcPriority, 0),
      ensemble->getSw()->getHwAsicTable()->isFeatureSupportedOnAllAsic(
          facebook::fboss::HwAsic::Feature::PFC_XON_TO_XOFF_COUNTER)
          ? folly::get_default(portStats.get_inPfcXon_(), pfcPriority, 0)
          : 0};
}

void waitPfcCounterIncrease(
    facebook::fboss::AgentEnsemble* ensemble,
    const facebook::fboss::PortID& portId,
    const int pfcPriority) {
  int txPfcCtr = 0, rxPfcCtr = 0, rxPfcXonCtr = 0;

  WITH_RETRIES({
    auto portStats = ensemble->getLatestPortStats(portId);
    std::tie(txPfcCtr, rxPfcCtr, rxPfcXonCtr) =
        getPfcTxRxXonHwPortStats(ensemble, portStats, pfcPriority);
    XLOG(DBG0) << " Port: " << portId << " PFC TX/RX PFC/RX_PFC_XON "
               << txPfcCtr << "/" << rxPfcCtr << "/" << rxPfcXonCtr
               << ", priority: " << pfcPriority;

    EXPECT_EVENTUALLY_GT(txPfcCtr, 0);
    EXPECT_EVENTUALLY_GT(rxPfcCtr, 0);
    if (ensemble->getSw()->getHwAsicTable()->isFeatureSupportedOnAllAsic(
            facebook::fboss::HwAsic::Feature::PFC_XON_TO_XOFF_COUNTER)) {
      EXPECT_EVENTUALLY_GT(rxPfcXonCtr, 0);
    }
  });
}

void validatePfcCountersIncreased(
    facebook::fboss::AgentEnsemble* ensemble,
    const int pri,
    const std::vector<facebook::fboss::PortID>& portIds) {
  for (const auto& portId : portIds) {
    waitPfcCounterIncrease(ensemble, portId, pri);
  }
}

} // namespace

namespace facebook::fboss {

class AgentTrafficPfcTest : public AgentHwTest {
 public:
  void setCmdLineFlagOverrides() const override {
    AgentHwTest::setCmdLineFlagOverrides();
    // TODO: do the equivalent of FLAGS_mmu_lossless_mode = true;
    /*
     * Makes this flag available so that it can be used in early
     * stages of init to setup common buffer pool for specific
     * asics like Jericho2.
     */
    FLAGS_ingress_egress_buffer_pool_size = kGlobalIngressEgressBufferPoolSize;
  }

  cfg::SwitchConfig initialConfig(
      const AgentEnsemble& ensemble) const override {
    auto config = utility::onePortPerInterfaceConfig(
        ensemble.getSw(),
        ensemble.masterLogicalPortIds(),
        true /*interfaceHasSubnet*/);
    return config;
  }

  std::vector<production_features::ProductionFeature>
  getProductionFeaturesVerified() const override {
    return {production_features::ProductionFeature::PFC};
  }

  std::vector<PortID> portIdsForTest() {
    return {
        masterLogicalInterfacePortIds()[0], masterLogicalInterfacePortIds()[1]};
  }

  std::vector<folly::IPAddressV6> kDestIps() const {
    return {
        folly::IPAddressV6("2620:0:1cfe:face:b00c::4"),
        folly::IPAddressV6("2620:0:1cfe:face:b00c::5")};
  }

  folly::MacAddress getIntfMac() const {
    return utility::getFirstInterfaceMac(getProgrammedState());
  }

 protected:
  void setupEcmpTraffic(const std::vector<PortID>& portIds) {
    utility::EcmpSetupTargetedPorts6 ecmpHelper{
        getProgrammedState(), getIntfMac()};

    CHECK_EQ(portIds.size(), kDestIps().size());
    for (int i = 0; i < portIds.size(); ++i) {
      const PortDescriptor port(portIds[i]);
      RoutePrefixV6 route{kDestIps()[i], 128};

      applyNewState([&](const std::shared_ptr<SwitchState>& state) {
        return ecmpHelper.resolveNextHops(state, {port});
      });

      auto routeUpdater = getSw()->getRouteUpdater();
      ecmpHelper.programRoutes(&routeUpdater, {port}, {route});

      utility::ttlDecrementHandlingForLoopbackTraffic(
          getAgentEnsemble(), ecmpHelper.getRouterId(), ecmpHelper.nhop(port));
    }
  }

  void validateInitPfcCounters(
      const std::vector<PortID>& portIds,
      const int pfcPriority) {
    int txPfcCtr = 0, rxPfcCtr = 0, rxPfcXonCtr = 0;
    // no need to retry if looking for baseline counter
    for (const auto& portId : portIds) {
      auto portStats = getLatestPortStats(portId);
      auto ingressDropRaw = *portStats.inDiscardsRaw_();
      XLOG(DBG0) << " validateInitPfcCounters: Port: " << portId
                 << " IngressDropRaw: " << ingressDropRaw;
      EXPECT_TRUE(ingressDropRaw == 0);
      std::tie(txPfcCtr, rxPfcCtr, rxPfcXonCtr) =
          getPfcTxRxXonHwPortStats(getAgentEnsemble(), portStats, pfcPriority);
      EXPECT_TRUE((txPfcCtr == 0) && (rxPfcCtr == 0) && (rxPfcXonCtr == 0));
    }
  }

  void validateIngressDropCounters(const std::vector<PortID>& portIds) {
    WITH_RETRIES({
      for (const auto& portId : portIds) {
        auto portStats = getLatestPortStats(portId);
        auto ingressDropRaw = *portStats.inDiscardsRaw_();
        uint64_t ingressCongestionDiscards = 0;
        std::string ingressCongestionDiscardLog{};
        if (isSupportedOnAllAsics(
                HwAsic::Feature::INGRESS_PRIORITY_GROUP_DROPPED_PACKETS)) {
          ingressCongestionDiscards = *portStats.inCongestionDiscards_();
          ingressCongestionDiscardLog = " IngressCongestionDiscards: " +
              std::to_string(ingressCongestionDiscards);
        }
        XLOG(DBG0) << " validateIngressDropCounters: Port: " << portId
                   << " IngressDropRaw: " << ingressDropRaw
                   << ingressCongestionDiscardLog;
        EXPECT_EVENTUALLY_GT(ingressDropRaw, 0);
        if (isSupportedOnAllAsics(
                HwAsic::Feature::INGRESS_PRIORITY_GROUP_DROPPED_PACKETS)) {
          EXPECT_EVENTUALLY_GT(ingressCongestionDiscards, 0);
        }
      }
      for (auto [switchId, asic] : getAsics()) {
        if (asic->getAsicType() == cfg::AsicType::ASIC_TYPE_JERICHO3) {
          // Jericho3 has additional VSQ drops counters which accounts for
          // ingress buffer drops.
          getSw()->updateStats();
          fb303::ThreadCachedServiceData::get()->publishStats();
          auto switchIndex =
              getSw()->getSwitchInfoTable().getSwitchIndexFromSwitchId(
                  switchId);
          auto vsqResourcesExhautionDrops =
              *getSw()
                   ->getHwSwitchStatsExpensive()[switchIndex]
                   .fb303GlobalStats()
                   ->vsq_resource_exhaustion_drops();
          XLOG(DBG0)
              << " validateIngressDropCounters: vsqResourceExhaustionDrops: "
              << vsqResourcesExhautionDrops;
          EXPECT_EVENTUALLY_GT(vsqResourcesExhautionDrops, 0);
        }
      }
    });
  }

  void pumpTraffic(const int priority) {
    auto vlanId = utility::firstVlanID(getProgrammedState());
    auto intfMac = getIntfMac();
    auto srcMac = utility::MacAddressGenerator().get(intfMac.u64NBO() + 1);
    // pri = 7 => dscp 56
    int dscp = priority * 8;
    // Tomahawk4 need 5 packets per flow to trigger PFC
    int numPacketsPerFlow = getAgentEnsemble()->getMinPktsForLineRate(
        masterLogicalInterfacePortIds()[0]);
    for (int i = 0; i < numPacketsPerFlow; i++) {
      for (const auto& dstIp : kDestIps()) {
        auto txPacket = utility::makeUDPTxPacket(
            getSw(),
            vlanId,
            srcMac,
            intfMac,
            folly::IPAddressV6("2620:0:1cfe:face:b00c::3"),
            dstIp,
            8000,
            8001,
            dscp << 2, // dscp is last 6 bits in TC
            255,
            std::vector<uint8_t>(2000, 0xff));

        getAgentEnsemble()->sendPacketAsync(
            std::move(txPacket), std::nullopt, std::nullopt);
      }
    }
  }

  void stopTraffic(const std::vector<PortID>& portIds) {
    // Toggle the link to break looping traffic
    for (auto portId : portIds) {
      bringDownPort(portId);
      bringUpPort(portId);
    }
  }

  void runTestWithCfg(
      const int trafficClass,
      const int pfcPriority,
      const std::map<int, int>& tcToPgOverride = {},
      TrafficTestParams testParams = TrafficTestParams{},
      std::function<void(
          AgentEnsemble* ensemble,
          const int pri,
          const std::vector<PortID>& portIdsToValidate)> validateCounterFn =
          validatePfcCountersIncreased) {
    std::vector<PortID> portIds = portIdsForTest();

    auto setup = [&]() {
      for (auto [switchId, asic] : this->getAsics()) {
        if ((asic->getAsicType() == cfg::AsicType::ASIC_TYPE_JERICHO2) ||
            (asic->getAsicType() == cfg::AsicType::ASIC_TYPE_JERICHO3)) {
          // Keep low scaling factor so that headroom usage is attempted
          // for Jericho family of ASICs.
          testParams.buffer.scalingFactor = cfg::MMUScalingFactor::ONE_32768TH;
        }
      }

      // Setup PFC
      auto cfg = getAgentEnsemble()->getCurrentConfig();
      std::vector<PortID> portIdsToConfigure = portIds;
      if (testParams.scale) {
        // Apply PFC config to all ports
        portIdsToConfigure = masterLogicalInterfacePortIds();
      }
      setupPfcBuffers(
          cfg,
          portIdsToConfigure,
          kLosslessPgIds,
          tcToPgOverride,
          testParams.buffer);
      applyNewConfig(cfg);

      setupEcmpTraffic(portIds);

      // ensure counter is 0 before we start traffic
      validateInitPfcCounters(portIds, pfcPriority);
    };
    auto verifyCommon = [&](bool postWb) {
      pumpTraffic(trafficClass);
      // check counters are as expected
      validateCounterFn(getAgentEnsemble(), pfcPriority, portIds);
      if (testParams.expectDrop) {
        validateIngressDropCounters(portIds);
      }
      if (!FLAGS_skip_stop_pfc_test_traffic && postWb) {
        // stop traffic so that unconfiguration can happen without issues
        stopTraffic(portIds);
      }
    };
    auto verify = [&]() { verifyCommon(false /* postWb */); };
    auto verifyPostWb = [&]() { verifyCommon(true /* postWb */); };
    verifyAcrossWarmBoots(setup, verify, []() {}, verifyPostWb);
  }
};

class AgentTrafficPfcGenTest
    : public AgentTrafficPfcTest,
      public testing::WithParamInterface<TrafficTestParams> {
  void setCmdLineFlagOverrides() const override {
    AgentTrafficPfcTest::setCmdLineFlagOverrides();
    if (GetParam().buffer.pgHeadroom == 0) {
      // Force headroom 0 for lossless PG
      FLAGS_allow_zero_headroom_for_lossless_pg = true;
    }
  }
};

INSTANTIATE_TEST_SUITE_P(
    AgentTrafficPfcTest,
    AgentTrafficPfcGenTest,
    testing::Values(
        TrafficTestParams{
            .name = "WithDefaultCfg",
        },
        TrafficTestParams{
            .name = "WithScaleCfg",
            .buffer =
                PfcBufferParams{
                    .globalShared = kGlobalSharedBytes * 5,
                    .pgLimit = kPgLimitBytes / 3,
                    .resumeOffset = kPgResumeOffsetBytes / 3},
            .scale = true},
        TrafficTestParams{
            .name = "WithZeroPgHeadRoomCfg",
            .buffer =
                PfcBufferParams{.pgHeadroom = 0, .scalingFactor = std::nullopt},
            .expectDrop = true},
        TrafficTestParams{
            .name = "WithZeroGlobalHeadRoomCfg",
            .buffer =
                PfcBufferParams{
                    .globalHeadroom = 0,
                    .scalingFactor = std::nullopt},
            .expectDrop = true}),
    [](const ::testing::TestParamInfo<TrafficTestParams>& info) {
      return info.param.name;
    });

TEST_P(AgentTrafficPfcGenTest, verifyPfc) {
  const int trafficClass = kLosslessTrafficClass;
  const int pfcPriority = kLosslessPriority;
  TrafficTestParams trafficParams = GetParam();
  runTestWithCfg(trafficClass, pfcPriority, {}, trafficParams);
}

// intent of this test is to send traffic so that it maps to
// tc 2, now map tc 2 to PG 3. Mapping from PG to pfc priority
// is 1:1, which means PG 3 is mapped to pfc priority 3.
// Generate traffic to fire off PFC with smaller shared buffer
TEST_F(AgentTrafficPfcTest, verifyPfcWithMapChanges_0) {
  const int trafficClass = kLosslessTrafficClass;
  const int pfcPriority = 3;
  runTestWithCfg(trafficClass, pfcPriority, {{trafficClass, pfcPriority}});
}

// intent of this test is to send traffic so that it maps to
// tc 7. Now we map tc 7 -> PG 2. Mapping from PG to pfc
// priority is 1:1, which means PG 2 is mapped to pfc priority 2.
// Generate traffic to fire off PFC with smaller shared buffer
TEST_F(AgentTrafficPfcTest, verifyPfcWithMapChanges_1) {
  const int trafficClass = 7;
  const int pfcPriority = kLosslessPriority;
  runTestWithCfg(trafficClass, pfcPriority, {{trafficClass, pfcPriority}});
}

} // namespace facebook::fboss
