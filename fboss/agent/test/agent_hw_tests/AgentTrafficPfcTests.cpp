// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <folly/MapUtil.h>

#include "fboss/agent/AgentFeatures.h"
#include "fboss/agent/TxPacket.h"
#include "fboss/agent/packet/PktFactory.h"
#include "fboss/agent/test/AgentHwTest.h"
#include "fboss/agent/test/EcmpSetupHelper.h"
#include "fboss/agent/test/utils/AsicUtils.h"
#include "fboss/agent/test/utils/ConfigUtils.h"
#include "fboss/agent/test/utils/CoppTestUtils.h"
#include "fboss/agent/test/utils/PfcTestUtils.h"
#include "fboss/agent/test/utils/PortTestUtils.h"
#include "fboss/agent/test/utils/QosTestUtils.h"
#include "fboss/lib/CommonUtils.h"

DEFINE_bool(
    skip_stop_pfc_test_traffic,
    false,
    "Skip stopping traffic after traffic test!");

namespace {

using facebook::fboss::utility::PfcBufferParams;

// TODO(maxgg): Change the overall default to 20000 once CS00012382848 is fixed.
static constexpr auto kSmallGlobalSharedSize{20000};
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

    // TODO(maxgg): CS00012381334 - Rx counters not incrementing on TH5
    // However we know PFC is working as long as TX PFC is being generated, so
    // skip validating RX PFC counters on TH5 for now.
    auto asicType =
        facebook::fboss::utility::checkSameAndGetAsic(ensemble->getL3Asics())
            ->getAsicType();
    if (asicType != facebook::fboss::cfg::AsicType::ASIC_TYPE_TOMAHAWK5) {
      EXPECT_EVENTUALLY_GT(rxPfcCtr, 0);
      if (ensemble->getSw()->getHwAsicTable()->isFeatureSupportedOnAllAsic(
              facebook::fboss::HwAsic::Feature::PFC_XON_TO_XOFF_COUNTER)) {
        EXPECT_EVENTUALLY_GT(rxPfcXonCtr, 0);
      }
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

void validateBufferPoolWatermarkCounters(
    facebook::fboss::AgentEnsemble* ensemble,
    const int /* pri */,
    const std::vector<facebook::fboss::PortID>& /* portIds */) {
  uint64_t globalHeadroomWatermark{};
  uint64_t globalSharedWatermark{};
  WITH_RETRIES({
    ensemble->getSw()->updateStats();
    for (const auto& [switchIdx, stats] :
         ensemble->getSw()->getHwSwitchStatsExpensive()) {
      for (const auto& [pool, bytes] :
           *stats.switchWatermarkStats()->globalHeadroomWatermarkBytes()) {
        globalHeadroomWatermark += bytes;
      }
      for (const auto& [pool, bytes] :
           *stats.switchWatermarkStats()->globalSharedWatermarkBytes()) {
        globalSharedWatermark += bytes;
      }
    }
    XLOG(DBG0) << "Global headroom watermark: " << globalHeadroomWatermark
               << ", Global shared watermark: " << globalSharedWatermark;
    if (ensemble->getSw()->getHwAsicTable()->isFeatureSupportedOnAllAsic(
            facebook::fboss::HwAsic::Feature::
                INGRESS_PRIORITY_GROUP_HEADROOM_WATERMARK)) {
      EXPECT_EVENTUALLY_GT(globalHeadroomWatermark, 0);
    }
    EXPECT_EVENTUALLY_GT(globalSharedWatermark, 0);
  });
}

void validateIngressPriorityGroupWatermarkCounters(
    facebook::fboss::AgentEnsemble* ensemble,
    const int pri,
    const std::vector<facebook::fboss::PortID>& portIds) {
  std::string watermarkKeys = "shared";
  int numKeys = 1;
  if (ensemble->getSw()->getHwAsicTable()->isFeatureSupportedOnAllAsic(
          facebook::fboss::HwAsic::Feature::
              INGRESS_PRIORITY_GROUP_HEADROOM_WATERMARK)) {
    watermarkKeys.append("|headroom");
    numKeys++;
  }
  WITH_RETRIES({
    for (const auto& portId : portIds) {
      const auto& portName = ensemble->getProgrammedState()
                                 ->getPorts()
                                 ->getNodeIf(portId)
                                 ->getName();
      std::string pg = ensemble->isSai() ? folly::sformat(".pg{}", pri) : "";
      auto regex = folly::sformat(
          "buffer_watermark_pg_({}).{}{}.p100.60", watermarkKeys, portName, pg);
      auto counters = facebook::fb303::fbData->getRegexCounters(regex);
      CHECK_EQ(counters.size(), numKeys);
      for (const auto& ctr : counters) {
        XLOG(DBG0) << ctr.first << " : " << ctr.second;
        EXPECT_EVENTUALLY_GT(ctr.second, 0);
      }
    }
  });
}

} // namespace

namespace facebook::fboss {

class AgentTrafficPfcTest : public AgentHwTest {
 public:
  void setCmdLineFlagOverrides() const override {
    AgentHwTest::setCmdLineFlagOverrides();
    /*
     * Makes this flag available so that it can be used in early
     * stages of init to setup common buffer pool for specific
     * asics like Jericho2.
     */
    FLAGS_ingress_egress_buffer_pool_size = kGlobalIngressEgressBufferPoolSize;
    // Some platforms (TH4, TH5) requires same egress/ingress buffer pool sizes.
    FLAGS_egress_buffer_pool_size = PfcBufferParams::kGlobalSharedBytes +
        PfcBufferParams::kGlobalHeadroomBytes;
    FLAGS_skip_buffer_reservation = true;
    FLAGS_qgroup_guarantee_enable = true;
  }

  void applyPlatformConfigOverrides(
      const cfg::SwitchConfig& sw,
      cfg::PlatformConfig& config) const override {
    if (utility::checkSameAndGetAsicType(sw) ==
        cfg::AsicType::ASIC_TYPE_CHENAB) {
      return;
    }
    utility::modifyPlatformConfig(
        config,
        [](std::string& yamlCfg) {
          std::string toReplace("LOSSY");
          std::size_t pos = yamlCfg.find(toReplace);
          if (pos != std::string::npos) {
            // for TH4 we skip buffer reservation in prod
            // but it doesn't seem to work for pfc tests which
            // play around with other variables. For unblocking
            // skip it for now
            if (FLAGS_skip_buffer_reservation) {
              yamlCfg.replace(
                  pos,
                  toReplace.length(),
                  "LOSSY_AND_LOSSLESS\n      SKIP_BUFFER_RESERVATION: 1");
            } else {
              yamlCfg.replace(pos, toReplace.length(), "LOSSY_AND_LOSSLESS");
            }
          }
        },
        [](std::map<std::string, std::string>& cfg) {
          cfg["mmu_lossless"] = "0x2";
          cfg["buf.mqueue.guarantee.0"] = "0C";
          cfg["mmu_config_override"] = "0";
          cfg["buf.prigroup7.guarantee"] = "0C";
          if (FLAGS_qgroup_guarantee_enable) {
            cfg["buf.qgroup.guarantee_mc"] = "0";
            cfg["buf.qgroup.guarantee"] = "0";
          }
        });
  }

  cfg::SwitchConfig initialConfig(
      const AgentEnsemble& ensemble) const override {
    auto config = utility::onePortPerInterfaceConfig(
        ensemble.getSw(),
        ensemble.masterLogicalPortIds(),
        true /*interfaceHasSubnet*/);
    utility::setTTLZeroCpuConfig(ensemble.getL3Asics(), config);
    return config;
  }

  std::vector<production_features::ProductionFeature>
  getProductionFeaturesVerified() const override {
    return {production_features::ProductionFeature::PFC};
  }

  std::string portDesc(const PortID& portId) {
    const auto& cfg = getAgentEnsemble()->getCurrentConfig();
    for (const auto& port : *cfg.ports()) {
      if (PortID(*port.logicalID()) == portId) {
        return folly::sformat(
            "portId={} name={}", *port.logicalID(), *port.name());
      }
    }
    return "";
  }

  std::vector<PortID> portIdsForTest(bool scaleTest = false) {
    auto allPorts = masterLogicalInterfacePortIds();
    int numPorts = scaleTest ? allPorts.size() : 2;
    return std::vector<PortID>(allPorts.begin(), allPorts.begin() + numPorts);
  }

  std::vector<folly::IPAddressV6> getDestinationIps(int count = 2) const {
    std::vector<folly::IPAddressV6> ips;
    for (int i = 4; i < count + 4; i++) {
      ips.emplace_back(folly::sformat("2620:0:1cfe:face:b00c::{}", i));
    }
    return ips;
  }

  folly::MacAddress getIntfMac() const {
    return utility::getFirstInterfaceMac(getProgrammedState());
  }

 protected:
  PfcBufferParams defaultPfcBufferParams(
      PfcBufferParams buffer = PfcBufferParams{}) {
    auto asic = utility::checkSameAndGetAsic(getAgentEnsemble()->getL3Asics());
    switch (asic->getAsicType()) {
      case cfg::AsicType::ASIC_TYPE_JERICHO2:
      case cfg::AsicType::ASIC_TYPE_JERICHO3:
        buffer.globalShared = kSmallGlobalSharedSize;
        break;
      default:
        break;
    }
    if (!buffer.scalingFactor.has_value()) {
      switch (asic->getAsicType()) {
        case cfg::AsicType::ASIC_TYPE_TOMAHAWK3:
        case cfg::AsicType::ASIC_TYPE_TOMAHAWK4:
        case cfg::AsicType::ASIC_TYPE_TOMAHAWK5:
          buffer.scalingFactor = cfg::MMUScalingFactor::ONE_HALF;
          break;
        default:
          buffer.scalingFactor = cfg::MMUScalingFactor::ONE_128TH;
          break;
      }
    }
    return buffer;
  }

  void setupEcmpTraffic(const PortID& portId, const folly::IPAddressV6& ip) {
    utility::EcmpSetupTargetedPorts6 ecmpHelper{
        getProgrammedState(), getIntfMac()};

    const PortDescriptor port(portId);
    RoutePrefixV6 route{ip, 128};

    applyNewState([&](const std::shared_ptr<SwitchState>& state) {
      return ecmpHelper.resolveNextHops(state, {port});
    });

    auto routeUpdater = getSw()->getRouteUpdater();
    ecmpHelper.programRoutes(&routeUpdater, {port}, {route});

    utility::ttlDecrementHandlingForLoopbackTraffic(
        getAgentEnsemble(), ecmpHelper.getRouterId(), ecmpHelper.nhop(port));
  }

  void setupEcmpTraffic(const std::vector<PortID>& portIds) {
    auto ips = getDestinationIps(portIds.size());
    CHECK_EQ(portIds.size(), ips.size());
    for (int i = 0; i < portIds.size(); ++i) {
      setupEcmpTraffic(portIds[i], ips[i]);
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
        // In congestion discard stats is supported in native impl
        // and in sai platforms with HwAsic::Feature enabled.
        bool isIngressCongestionDiscardsSupported =
            isSupportedOnAllAsics(
                HwAsic::Feature::INGRESS_PRIORITY_GROUP_DROPPED_PACKETS) ||
            !getAgentEnsemble()->isSai();
        if (isIngressCongestionDiscardsSupported) {
          ingressCongestionDiscards = *portStats.inCongestionDiscards_();
          ingressCongestionDiscardLog = " IngressCongestionDiscards: " +
              std::to_string(ingressCongestionDiscards);
        }
        XLOG(DBG0) << " validateIngressDropCounters: Port: " << portId
                   << " IngressDropRaw: " << ingressDropRaw
                   << ingressCongestionDiscardLog;
        EXPECT_EVENTUALLY_GT(ingressDropRaw, 0);
        if (isIngressCongestionDiscardsSupported) {
          EXPECT_EVENTUALLY_GT(ingressCongestionDiscards, 0);
          // Ingress congestion discards should be less than
          // the total packets received on this port.
          uint64_t inPackets = *portStats.inUnicastPkts_() +
              *portStats.inMulticastPkts_() + *portStats.inBroadcastPkts_();
          EXPECT_EVENTUALLY_LT(ingressCongestionDiscards, inPackets);
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

  void pumpTraffic(
      const int priority,
      const std::optional<uint8_t> queue,
      const std::vector<PortID>& portIds,
      const std::vector<folly::IPAddressV6>& ips) {
    auto vlanId = utility::firstVlanID(getProgrammedState());
    auto intfMac = getIntfMac();
    auto srcMac = utility::MacAddressGenerator().get(intfMac.u64NBO() + 1);
    // pri = 7 => dscp 56
    int dscp = priority * 8;
    int numPacketsPerFlow = getAgentEnsemble()->getMinPktsForLineRate(
        masterLogicalInterfacePortIds()[0]);
    // Some asics (e.g. Yuba) won't let traffic build up evenly if all packets
    // were sent to one port before another. It's better to alternate the ports.
    for (int i = 0; i < numPacketsPerFlow; i++) {
      XLOG_EVERY_N(INFO, 100) << "Sending packet " << i;
      for (int j = 0; j < ips.size(); j++) {
        auto txPacket = utility::makeUDPTxPacket(
            getSw(),
            vlanId,
            srcMac,
            intfMac,
            folly::IPAddressV6("2620:0:1cfe:face:b00c::3"),
            ips[j],
            8000,
            8001,
            dscp << 2, // dscp is last 6 bits in TC
            255,
            std::vector<uint8_t>(2000, 0xff));

        // TODO(maxgg): Investigate TH3/4 WithScaleCfg failure when queue is
        // set to losslessPriority (2).
        getAgentEnsemble()->sendPacketAsync(
            std::move(txPacket), PortDescriptor(portIds[j]), queue);
      }
    }
  }

  void pumpTraffic(const int priority, bool scaleTest) {
    std::vector<PortID> portIds = portIdsForTest(scaleTest);
    pumpTraffic(
        priority, std::nullopt, portIds, getDestinationIps(portIds.size()));
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
      const std::map<int, int>& tcToPgOverride,
      TrafficTestParams testParams,
      std::function<void(
          AgentEnsemble* ensemble,
          const int pri,
          const std::vector<PortID>& portIdsToValidate)> validateCounterFn =
          validatePfcCountersIncreased) {
    std::vector<PortID> portIds = portIdsForTest(testParams.scale);
    for (const auto& portId : portIds) {
      XLOG(INFO) << "Testing port: " << portDesc(portId);
    }

    auto setup = [&]() {
      // Setup PFC
      auto cfg = getAgentEnsemble()->getCurrentConfig();
      // Apply PFC config to all ports of interest
      utility::setupPfcBuffers(
          getAgentEnsemble(),
          cfg,
          portIds,
          kLosslessPgIds,
          tcToPgOverride,
          testParams.buffer);
      auto asic =
          utility::checkSameAndGetAsic(getAgentEnsemble()->getL3Asics());
      if (isSupportedOnAllAsics(HwAsic::Feature::MULTIPLE_EGRESS_BUFFER_POOL)) {
        utility::setupMultipleEgressPoolAndQueueConfigs(
            cfg, kLosslessPgIds, asic->getMMUSizeBytes());
      }
      if (asic->getAsicType() == cfg::AsicType::ASIC_TYPE_YUBA) {
        // For YUBA, lossless queues needs to be configured with static
        // queue limit equal to the MMU size to ensure its lossless.
        for (auto& queueConfigs : *cfg.portQueueConfigs()) {
          for (auto& queueCfg : queueConfigs.second) {
            if (std::find(
                    kLosslessPgIds.begin(),
                    kLosslessPgIds.end(),
                    *queueCfg.id()) != kLosslessPgIds.end()) {
              // Given the 1:1 mapping for queueID to PG ID,
              // this is a lossless queue.
              queueCfg.sharedBytes() = asic->getMMUSizeBytes();
            }
          }
        }
      }
      applyNewConfig(cfg);

      setupEcmpTraffic(portIds);

      // ensure counter is 0 before we start traffic
      validateInitPfcCounters(portIds, pfcPriority);
    };
    auto verifyCommon = [&](bool postWb) {
      pumpTraffic(trafficClass, testParams.scale);
      // Sleep for a bit before validation, so that the test will fail if
      // traffic doesn't actually build up, instead of passing by luck.
      // NOLINTNEXTLINE(facebook-hte-BadCall-sleep)
      sleep(2);
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
    // Some platforms (TH4, TH5) requires same egress/ingress buffer pool sizes.
    FLAGS_egress_buffer_pool_size =
        GetParam().buffer.globalShared + GetParam().buffer.globalHeadroom;
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
            .scale = true,
        },
        TrafficTestParams{
            .name = "WithZeroPgHeadRoomCfg",
            .buffer = PfcBufferParams{.pgHeadroom = 0},
            .expectDrop = true,
        },
        TrafficTestParams{
            .name = "WithZeroGlobalHeadRoomCfg",
            .buffer = PfcBufferParams{.globalHeadroom = 0},
            .expectDrop = true,
        },
        TrafficTestParams{
            .name = "WithScaleCfgInCongestionDrops",
            .buffer = PfcBufferParams{.pgHeadroom = 0},
            .expectDrop = true,
            .scale = true,
        }),
    [](const ::testing::TestParamInfo<TrafficTestParams>& info) {
      return info.param.name;
    });

TEST_P(AgentTrafficPfcGenTest, verifyPfc) {
  const int trafficClass = kLosslessTrafficClass;
  const int pfcPriority = kLosslessPriority;
  auto param = GetParam();
  param.buffer = defaultPfcBufferParams(param.buffer);
  runTestWithCfg(trafficClass, pfcPriority, {}, param);
}

// intent of this test is to send traffic so that it maps to
// tc 2, now map tc 2 to PG 3. Mapping from PG to pfc priority
// is 1:1, which means PG 3 is mapped to pfc priority 3.
// Generate traffic to fire off PFC with smaller shared buffer
TEST_F(AgentTrafficPfcTest, verifyPfcWithMapChanges_0) {
  const int trafficClass = kLosslessTrafficClass;
  const int pfcPriority = 3;
  runTestWithCfg(
      trafficClass,
      pfcPriority,
      {{trafficClass, pfcPriority}},
      TrafficTestParams{.buffer = defaultPfcBufferParams()});
}

// intent of this test is to send traffic so that it maps to
// tc 7. Now we map tc 7 -> PG 2. Mapping from PG to pfc
// priority is 1:1, which means PG 2 is mapped to pfc priority 2.
// Generate traffic to fire off PFC with smaller shared buffer
TEST_F(AgentTrafficPfcTest, verifyPfcWithMapChanges_1) {
  const int trafficClass = 7;
  const int pfcPriority = kLosslessPriority;
  runTestWithCfg(
      trafficClass,
      pfcPriority,
      {{trafficClass, pfcPriority}},
      TrafficTestParams{.buffer = defaultPfcBufferParams()});
}

TEST_F(AgentTrafficPfcTest, verifyBufferPoolWatermarks) {
  const int trafficClass = kLosslessTrafficClass;
  const int pfcPriority = kLosslessPriority;
  runTestWithCfg(
      trafficClass,
      pfcPriority,
      {},
      TrafficTestParams{.buffer = defaultPfcBufferParams()},
      validateBufferPoolWatermarkCounters);
}

TEST_F(AgentTrafficPfcTest, verifyIngressPriorityGroupWatermarks) {
  const int trafficClass = kLosslessTrafficClass;
  const int pfcPriority = kLosslessPriority;
  runTestWithCfg(
      trafficClass,
      pfcPriority,
      {},
      TrafficTestParams{.buffer = defaultPfcBufferParams()},
      validateIngressPriorityGroupWatermarkCounters);
}

class AgentTrafficPfcWatchdogTest : public AgentTrafficPfcTest {
 protected:
  void setupConfigAndEcmpTraffic(
      const PortID& portId,
      const PortID& txOffPortId,
      const folly::IPAddressV6& ip) {
    cfg::SwitchConfig cfg = getAgentEnsemble()->getCurrentConfig();
    PfcBufferParams buffer = defaultPfcBufferParams();
    buffer.scalingFactor = cfg::MMUScalingFactor::ONE_128TH;
    utility::setupPfcBuffers(
        getAgentEnsemble(), cfg, {portId}, kLosslessPgIds, {}, buffer);
    applyNewConfig(cfg);
    setupEcmpTraffic(txOffPortId, ip);
  }

  void setupWatchdog(const std::vector<PortID>& portIds, bool enable) {
    cfg::PfcWatchdog pfcWatchdog;
    if (enable) {
      pfcWatchdog.recoveryAction() = cfg::PfcWatchdogRecoveryAction::NO_DROP;
      // Configure values that works for specific HW, include ASIC type checks
      // if needed, given some HW has limitations on the specific values that
      // can be programmed and we need to ensure that the configured value here
      // is in sync with what is in SAI/SDK to avoid a reprogramming attempt
      // during warmboot.
      auto asic =
          utility::checkSameAndGetAsic(getAgentEnsemble()->getL3Asics());
      switch (asic->getAsicType()) {
        case cfg::AsicType::ASIC_TYPE_JERICHO2:
        case cfg::AsicType::ASIC_TYPE_JERICHO3:
          pfcWatchdog.recoveryTimeMsecs() = 1000;
          pfcWatchdog.detectionTimeMsecs() = 198;
          break;
        case cfg::AsicType::ASIC_TYPE_TOMAHAWK4:
        case cfg::AsicType::ASIC_TYPE_TOMAHAWK5:
          pfcWatchdog.recoveryTimeMsecs() = 100;
          pfcWatchdog.detectionTimeMsecs() = 10;
          break;
        case cfg::AsicType::ASIC_TYPE_CHENAB:
          // The Chenab ASIC requires recovery time>=200 and detection
          // time>=200.
          pfcWatchdog.recoveryTimeMsecs() = 1000;
          pfcWatchdog.detectionTimeMsecs() = 200;
          break;
        case cfg::AsicType::ASIC_TYPE_YUBA:
          pfcWatchdog.recoveryTimeMsecs() = 100;
          pfcWatchdog.detectionTimeMsecs() = 25;
          break;
        default:
          pfcWatchdog.recoveryTimeMsecs() = 10;
          pfcWatchdog.detectionTimeMsecs() = 1;
          break;
      }
    }

    auto cfg = getAgentEnsemble()->getCurrentConfig();
    for (const auto& portID : portIds) {
      auto portCfg = utility::findCfgPort(cfg, portID);
      if (portCfg->pfc().has_value()) {
        if (enable) {
          portCfg->pfc()->watchdog() = pfcWatchdog;
        } else {
          portCfg->pfc()->watchdog().reset();
        }
      }
    }
    applyNewConfig(cfg);
  }

  void triggerPfcDeadlockDetection(
      const PortID& port,
      const PortID& txOffPortId,
      const folly::IPAddressV6& ip) {
    auto asic = utility::checkSameAndGetAsic(getAgentEnsemble()->getL3Asics());
    if ((asic->getAsicType() == cfg::AsicType::ASIC_TYPE_JERICHO2) ||
        (asic->getAsicType() == cfg::AsicType::ASIC_TYPE_JERICHO3)) {
      // As traffic cannot trigger deadlock for DNX, force back
      // to back PFC frame generation which causes a deadlock!
      XLOG(DBG0) << "Triggering PFC deadlock detection on port ID : "
                 << static_cast<int>(port);
      auto iter = kRegValToForcePfcTxForPriorityOnPortDnx.find(std::make_tuple(
          asic->getAsicType(), static_cast<int>(port), kLosslessPriority));
      EXPECT_FALSE(iter == kRegValToForcePfcTxForPriorityOnPortDnx.end());
      std::string out;
      getAgentEnsemble()->runDiagCommand(
          "modreg CFC_FRC_NIF_ETH_PFC FRC_NIF_ETH_PFC=" + iter->second +
              "\nquit\n",
          out);
    } else {
      // Disable Tx on the outbound port so that queues will build up.
      utility::setCreditWatchdogAndPortTx(
          getAgentEnsemble(), txOffPortId, false);
      pumpTraffic(kLosslessTrafficClass, kLosslessPriority, {port}, {ip});
      validatePfcCounterIncrement(port, kLosslessPriority);
    }
  }

  void validatePfcCounterIncrement(const PortID& port, const int pfcPriority) {
    // CS00012381334 - MAC loopback doesn't work on TH5 and Rx PFC counters
    // doesn't increase. Tx counters should work for all platforms.
    int txPfcCtrOld = getLatestPortStats(port).outPfc_()->at(pfcPriority);
    WITH_RETRIES_N_TIMED(3, std::chrono::milliseconds(1000), {
      int txPfcCtrNew = getLatestPortStats(port).outPfc_()->at(pfcPriority);
      EXPECT_EVENTUALLY_GT(txPfcCtrNew, txPfcCtrOld);
    });
  }

  std::tuple<int, int> getPfcDeadlockCounters(const PortID& portId) {
    const auto& portName = getAgentEnsemble()
                               ->getProgrammedState()
                               ->getPorts()
                               ->getNodeIf(portId)
                               ->getName();
    return {
        facebook::fb303::fbData
            ->getCounterIfExists(portName + ".pfc_deadlock_detection.sum")
            .value_or(0),
        facebook::fb303::fbData
            ->getCounterIfExists(portName + ".pfc_deadlock_recovery.sum")
            .value_or(0),
    };
  }

  void validatePfcWatchdogCountersIncrease(
      const PortID& portId,
      const uint64_t& deadlockCtrBefore,
      const uint64_t& recoveryCtrBefore) {
    WITH_RETRIES_N_TIMED(10, std::chrono::milliseconds(1000), {
      auto [deadlockCtr, recoveryCtr] = getPfcDeadlockCounters(portId);
      XLOG(DBG0) << "For port: " << portId << " deadlockCtr = " << deadlockCtr
                 << " recoveryCtr = " << recoveryCtr;
      EXPECT_EVENTUALLY_GT(deadlockCtr, deadlockCtrBefore);
      EXPECT_EVENTUALLY_GT(recoveryCtr, recoveryCtrBefore);
    });
  }

  void reEnablePort(const PortID& txOffPortId) {
    utility::setCreditWatchdogAndPortTx(getAgentEnsemble(), txOffPortId, true);
  }
};

// Intent of this test is to setup PFC watchdog, trigger PFC, and observe that
// watchdog counters increase as deadlock/recovery callbacks are called. To
// trigger PFC, we add a route to an IP on a port (txPortPortId), then set it to
// Tx disabled. Then we send packets to that IP on a different port (portId),
// which will eventually cause queues to build up and PFC to trigger.
TEST_F(AgentTrafficPfcWatchdogTest, PfcWatchdogDetection) {
  PortID portId = masterLogicalInterfacePortIds()[0];
  PortID txOffPortId = masterLogicalInterfacePortIds()[1];
  XLOG(DBG3) << "Injection port: " << portDesc(portId);
  XLOG(DBG3) << "Tx off port: " << portDesc(txOffPortId);
  auto ip = getDestinationIps()[0];

  auto setup = [&]() {
    setupConfigAndEcmpTraffic(portId, txOffPortId, ip);
    setupWatchdog({portId}, true /* enable watchdog */);
  };
  auto verify = [&]() {
    auto [deadlockCtrBefore, recoveryCtrBefore] =
        getPfcDeadlockCounters(portId);
    triggerPfcDeadlockDetection(portId, txOffPortId, ip);
    validatePfcWatchdogCountersIncrease(
        portId, deadlockCtrBefore, recoveryCtrBefore);
    reEnablePort(txOffPortId);
  };
  verifyAcrossWarmBoots(setup, verify);
}

// intent of this test is to setup watchdog for PFC
// and remove it. Observe that counters should stop incrementing
// Clear them and check they stay the same
// Since the watchdog counters are sw based, upon warm boot
// we don't expect these counters to be incremented either
TEST_F(AgentTrafficPfcWatchdogTest, PfcWatchdogReset) {
  PortID portId = masterLogicalInterfacePortIds()[0];
  PortID txOffPortId = masterLogicalInterfacePortIds()[1];
  XLOG(DBG3) << "Injection port: " << portDesc(portId);
  XLOG(DBG3) << "Tx off port: " << portDesc(txOffPortId);
  auto ip = getDestinationIps()[0];

  int deadlockCtrBefore = 0;
  int recoveryCtrBefore = 0;

  auto setup = [&]() {
    setupConfigAndEcmpTraffic(portId, txOffPortId, ip);
    setupWatchdog({portId}, true /* enable watchdog */);
    std::tie(deadlockCtrBefore, recoveryCtrBefore) =
        getPfcDeadlockCounters(portId);
    triggerPfcDeadlockDetection(portId, txOffPortId, ip);
    // lets wait for the watchdog counters to be populated
    validatePfcWatchdogCountersIncrease(
        portId, deadlockCtrBefore, recoveryCtrBefore);
    // reset watchdog
    setupWatchdog({portId}, false /* disable */);
    // sleep a bit to let counters stabilize
    std::this_thread::sleep_for(std::chrono::seconds(1));
    std::tie(deadlockCtrBefore, recoveryCtrBefore) =
        getPfcDeadlockCounters(portId);
  };

  auto verify = [&]() {
    // ensure that PFC counters continues to increment
    validatePfcCounterIncrement(portId, kLosslessPriority);
    // validate that pfc watchdog counters do not increment anymore
    auto [deadlockCtr, recoveryCtr] = getPfcDeadlockCounters(portId);
    XLOG(DBG0) << "For port: " << portId << " deadlockCtr = " << deadlockCtr
               << " recoveryCtr = " << recoveryCtr;
    EXPECT_EQ(deadlockCtr, deadlockCtrBefore);
    EXPECT_EQ(recoveryCtr, recoveryCtrBefore);

    // SDK will be unhappy if we don't re-enable the port before shutdown.
    if (!FLAGS_setup_for_warmboot) {
      reEnablePort(txOffPortId);
    }
  };

  auto verifyPostWb = [&]() {
    // SDK will be unhappy if we don't re-enable the port before shutdown.
    reEnablePort(txOffPortId);
  };

  verifyAcrossWarmBoots(setup, verify, []() {}, verifyPostWb);
}

} // namespace facebook::fboss
