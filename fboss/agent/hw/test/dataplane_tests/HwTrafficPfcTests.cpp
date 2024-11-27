#include <fb303/ServiceData.h>

#include "fboss/agent/Platform.h"
#include "fboss/agent/SwitchStats.h"
#include "fboss/agent/Utils.h"
#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/hw/test/HwLinkStateDependentTest.h"
#include "fboss/agent/hw/test/HwTestCoppUtils.h"
#include "fboss/agent/hw/test/HwTestPacketUtils.h"
#include "fboss/agent/test/EcmpSetupHelper.h"

#include "fboss/agent/hw/test/dataplane_tests/HwTestQosUtils.h"
#include "fboss/agent/state/Interface.h"
#include "fboss/agent/state/Port.h"
#include "fboss/agent/state/PortMap.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/test/ResourceLibUtil.h"

#include "fboss/lib/CommonUtils.h"

using folly::IPAddress;
using folly::IPAddressV6;
using std::string;

DEFINE_bool(
    skip_stop_pfc_test_traffic,
    false,
    "Skip stopping traffic after traffic test!");
namespace {
static constexpr auto kGlobalSharedBytes{20000};
static constexpr auto kGlobalHeadroomBytes{
    5000}; // keep this lower than globalSharedBytes
static constexpr auto kPgLimitBytes{2200};
static constexpr auto kPgResumeOffsetBytes{1800};
static constexpr auto kPgHeadroomBytes{
    2200}; // keep this lower than kGlobalSharedBytes
static constexpr auto kGlobalIngressEgressBufferPoolSize{
    5064760}; // Keep a high pool size for DNX
static constexpr auto kLosslessTrafficClass{2};
static constexpr auto kLosslessPriority{2};
static constexpr auto kNumberOfPortsToEnablePfcOn{2};
static const std::vector<int> kLosslessPgIds{2, 3};
static const std::vector<int> kLossyPgIds{0};
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

struct PfcBufferParams {
  int globalShared = kGlobalSharedBytes;
  int globalHeadroom = kGlobalHeadroomBytes;
  int pgLimit = kPgLimitBytes;
  int pgHeadroom = kPgHeadroomBytes;
  std::optional<facebook::fboss::cfg::MMUScalingFactor> scalingFactor =
      facebook::fboss::cfg::MMUScalingFactor::ONE_128TH;
  int resumeOffset = kPgResumeOffsetBytes;
};

struct TrafficTestParams {
  PfcBufferParams buffer = PfcBufferParams{};
  bool expectDrop = false;
  bool scale = false;
};

std::tuple<int, int, int> getPfcTxRxXonHwPortStats(
    facebook::fboss::HwSwitchEnsemble* ensemble,
    const facebook::fboss::HwPortStats& portStats,
    const int pfcPriority) {
  return {
      portStats.get_outPfc_().at(pfcPriority),
      portStats.get_inPfc_().at(pfcPriority),
      ensemble->getAsic()->isSupported(
          facebook::fboss::HwAsic::Feature::PFC_XON_TO_XOFF_COUNTER)
          ? portStats.get_inPfcXon_().at(pfcPriority)
          : 0};
}

bool getPfcCountersRetry(
    facebook::fboss::HwSwitchEnsemble* ensemble,
    const facebook::fboss::PortID& portId,
    const int pfcPriority) {
  int txPfcCtr = 0, rxPfcCtr = 0, rxPfcXonCtr = 0;

  auto pfcCountersIncrementing = [&](const auto& newStats) {
    auto portStatsIter = newStats.find(portId);
    std::tie(txPfcCtr, rxPfcCtr, rxPfcXonCtr) =
        getPfcTxRxXonHwPortStats(ensemble, portStatsIter->second, pfcPriority);
    XLOG(DBG0) << " Port: " << portId << " PFC TX/RX PFC/RX_PFC_XON "
               << txPfcCtr << "/" << rxPfcCtr << "/" << rxPfcXonCtr
               << ", priority: " << pfcPriority;

    if (ensemble->getAsic()->isSupported(
            facebook::fboss::HwAsic::Feature::PFC_XON_TO_XOFF_COUNTER)) {
      return (txPfcCtr > 0 && rxPfcCtr > 0 && rxPfcXonCtr > 0);
    }
    return (txPfcCtr > 0 && rxPfcCtr > 0);
  };

  return ensemble->waitPortStatsCondition(
      pfcCountersIncrementing, 10, std::chrono::milliseconds(500));
}

void validatePfcCounters(
    facebook::fboss::HwSwitchEnsemble* ensemble,
    const int pri,
    const std::vector<facebook::fboss::PortID>& portIds) {
  // no need t retry if looking for baseline counter
  for (const auto& portId : portIds) {
    EXPECT_TRUE(getPfcCountersRetry(ensemble, portId, pri));
  }
}

void validateBufferPoolWatermarkCounters(
    facebook::fboss::HwSwitchEnsemble* ensemble,
    const int /* pri */,
    const std::vector<facebook::fboss::PortID>& /* portIds */) {
  uint64_t globalHeadroomWatermark{};
  uint64_t globalSharedWatermark{};
  WITH_RETRIES({
    ensemble->getHwSwitch()->updateStats();
    auto switchWatermarkStats =
        ensemble->getHwSwitch()->getSwitchWatermarkStats();
    for (auto& headroomStats :
         *switchWatermarkStats.globalHeadroomWatermarkBytes()) {
      globalHeadroomWatermark += headroomStats.second;
    }
    for (auto& sharedStats :
         *switchWatermarkStats.globalSharedWatermarkBytes()) {
      globalSharedWatermark += sharedStats.second;
    }
    XLOG(DBG0) << "Global headroom watermark: " << globalHeadroomWatermark
               << ", Global shared watermark: " << globalSharedWatermark;
    if (ensemble->getAsic()->isSupported(
            facebook::fboss::HwAsic::Feature::
                INGRESS_PRIORITY_GROUP_HEADROOM_WATERMARK)) {
      EXPECT_EVENTUALLY_GT(globalHeadroomWatermark, 0);
    }
    EXPECT_EVENTUALLY_GT(globalSharedWatermark, 0);
  });
}

void validateIngressPriorityGroupWatermarkCounters(
    facebook::fboss::HwSwitchEnsemble* ensemble,
    const int pri,
    const std::vector<facebook::fboss::PortID>& portIds) {
  std::string watermarkKeys = "shared";
  int numKeys = 1;
  if (ensemble->getAsic()->isSupported(
          facebook::fboss::HwAsic::Feature::
              INGRESS_PRIORITY_GROUP_HEADROOM_WATERMARK)) {
    watermarkKeys.append("|headroom");
    numKeys++;
  }
  auto ingressPriorityGroupWatermarksIncrementing =
      [&](const auto& /*newStats*/) {
        for (const auto& portId : portIds) {
          const auto& portName = ensemble->getProgrammedState()
                                     ->getPorts()
                                     ->getNodeIf(portId)
                                     ->getName();
          std::string pg =
              ensemble->isSai() ? folly::sformat(".pg{}", pri) : "";
          auto regex = folly::sformat(
              "buffer_watermark_pg_({}).{}{}.p100.60",
              watermarkKeys,
              portName,
              pg);
          auto counters = facebook::fb303::fbData->getRegexCounters(regex);
          CHECK_EQ(counters.size(), numKeys);
          for (const auto& ctr : counters) {
            XLOG(DBG0) << ctr.first << " : " << ctr.second;
            if (!ctr.second) {
              return false;
            }
          }
        }
        return true;
      };
  EXPECT_TRUE(ensemble->waitPortStatsCondition(
      ingressPriorityGroupWatermarksIncrementing,
      5,
      std::chrono::milliseconds(1000)));
}

} // namespace

namespace facebook::fboss {

class HwTrafficPfcTest : public HwLinkStateDependentTest {
 private:
  void SetUp() override {
    FLAGS_mmu_lossless_mode = true;
    /*
     * Makes this flag available so that it can be used in early
     * stages of init to setup common buffer pool for specific
     * asics like Jericho2.
     */
    FLAGS_ingress_egress_buffer_pool_size = kGlobalIngressEgressBufferPoolSize;
    HwLinkStateDependentTest::SetUp();
  }

  cfg::SwitchConfig initialConfig() const override {
    auto cfg = utility::onePortPerInterfaceConfig(
        getHwSwitch(),
        masterLogicalPortIds(),
        getAsic()->desiredLoopbackModes());
    utility::setTTLZeroCpuConfig(getHwSwitchEnsemble()->getL3Asics(), cfg);
    return cfg;
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

  void setupQosMapForPfc(cfg::QosMap& qosMap) {
    // update pfc maps
    std::map<int16_t, int16_t> tc2PgId;
    std::map<int16_t, int16_t> pfcPri2PgId;
    std::map<int16_t, int16_t> pfcPri2QueueId;
    // program defaults
    for (auto i = 0; i < 8; i++) {
      tc2PgId.emplace(i, i);
      pfcPri2PgId.emplace(i, i);
      pfcPri2QueueId.emplace(i, i);
    }

    for (auto& tc2Pg : tc2PgOverride) {
      tc2PgId[tc2Pg.first] = tc2Pg.second;
    }
    for (auto& tmp : pfcPri2PgIdOverride) {
      pfcPri2PgId[tmp.first] = tmp.second;
    }

    qosMap.dscpMaps()->resize(8);
    for (auto i = 0; i < 8; i++) {
      qosMap.dscpMaps()[i].internalTrafficClass() = i;
      for (auto j = 0; j < 8; j++) {
        qosMap.dscpMaps()[i].fromDscpToTrafficClass()->push_back(8 * i + j);
      }
    }
    qosMap.trafficClassToPgId() = std::move(tc2PgId);
    qosMap.pfcPriorityToPgId() = std::move(pfcPri2PgId);
    qosMap.pfcPriorityToQueueId() = std::move(pfcPri2QueueId);
  }

  void setupPfc(cfg::SwitchConfig& cfg, const std::vector<PortID>& ports) {
    cfg::PortPfc pfc;
    pfc.tx() = true;
    pfc.rx() = true;
    pfc.portPgConfigName() = "foo";

    cfg::QosMap qosMap;
    // setup qos map with pfc structs
    setupQosMapForPfc(qosMap);

    // setup qosPolicy
    cfg.qosPolicies()->resize(1);
    cfg.qosPolicies()[0].name() = "qp";
    cfg.qosPolicies()[0].qosMap() = qosMap;
    cfg::TrafficPolicyConfig dataPlaneTrafficPolicy;
    dataPlaneTrafficPolicy.defaultQosPolicy() = "qp";
    cfg.dataPlaneTrafficPolicy() = dataPlaneTrafficPolicy;

    for (const auto& portID : ports) {
      auto portCfg = std::find_if(
          cfg.ports()->begin(), cfg.ports()->end(), [&portID](auto& port) {
            return PortID(*port.logicalID()) == portID;
          });
      portCfg->pfc() = pfc;
    }
  }

  void setupBufferPoolConfig(
      std::map<std::string, cfg::BufferPoolConfig>& bufferPoolCfgMap,
      int globalSharedBytes,
      int globalHeadroomBytes) {
    cfg::BufferPoolConfig poolConfig;
    // provide small shared buffer size
    // idea is to hit the limit and trigger XOFF (PFC)
    poolConfig.sharedBytes() = globalSharedBytes;
    poolConfig.headroomBytes() = globalHeadroomBytes;
    bufferPoolCfgMap.insert(std::make_pair("bufferNew", poolConfig));
  }

  void setupPortPgConfig(
      std::map<std::string, std::vector<cfg::PortPgConfig>>& portPgConfigMap,
      int pgLimit,
      int pgHeadroom,
      std::optional<cfg::MMUScalingFactor> scalingFactor,
      int resumeOffset) {
    std::vector<cfg::PortPgConfig> portPgConfigs;
    // create 2 pgs
    for (auto pgId : kLosslessPgIds) {
      cfg::PortPgConfig pgConfig;
      pgConfig.id() = pgId;
      pgConfig.bufferPoolName() = "bufferNew";
      // provide atleast 1 cell worth of minLimit
      pgConfig.minLimitBytes() = pgLimit;
      // set large enough headroom to avoid drop
      pgConfig.headroomLimitBytes() = pgHeadroom;
      // resume offset
      pgConfig.resumeOffsetBytes() = resumeOffset;
      // set scaling factor
      if (scalingFactor) {
        pgConfig.scalingFactor() = *scalingFactor;
      }
      portPgConfigs.emplace_back(pgConfig);
    }

    // create lossy pgs
    if (!FLAGS_allow_zero_headroom_for_lossless_pg) {
      // If the flag is set, we already have lossless PGs being created
      // with headroom as 0 and there is no way to differentiate lossy
      // and lossless PGs now that headroom is set to zero for lossless.
      // So, avoid creating lossy PGs as this will result in PFC being
      // enabled for 3 priorities, which is not supported for TAJO.
      for (auto pgId : kLossyPgIds) {
        cfg::PortPgConfig pgConfig;
        pgConfig.id() = pgId;
        pgConfig.bufferPoolName() = "bufferNew";
        // provide atleast 1 cell worth of minLimit
        pgConfig.minLimitBytes() = pgLimit;
        // headroom set 0 identifies lossy pgs
        pgConfig.headroomLimitBytes() = 0;
        // resume offset
        pgConfig.resumeOffsetBytes() = resumeOffset;
        // set scaling factor
        if (scalingFactor) {
          pgConfig.scalingFactor() = *scalingFactor;
        }
        portPgConfigs.emplace_back(pgConfig);
      }
    }

    portPgConfigMap["foo"] = portPgConfigs;
  }

  void setupBuffers(
      PfcBufferParams buffer = PfcBufferParams{},
      bool scaleConfig = false) {
    auto newCfg{initialConfig()};
    int numberOfPorts = scaleConfig ? masterLogicalInterfacePortIds().size()
                                    : kNumberOfPortsToEnablePfcOn;
    auto allPorts = masterLogicalInterfacePortIds();
    std::vector<PortID> ports(
        allPorts.begin(), allPorts.begin() + numberOfPorts);
    setupPfc(newCfg, ports);

    std::map<std::string, std::vector<cfg::PortPgConfig>> portPgConfigMap;
    setupPortPgConfig(
        portPgConfigMap,
        buffer.pgLimit,
        buffer.pgHeadroom,
        buffer.scalingFactor,
        buffer.resumeOffset);
    newCfg.portPgConfigs() = portPgConfigMap;

    // create buffer pool
    std::map<std::string, cfg::BufferPoolConfig> bufferPoolCfgMap;
    setupBufferPoolConfig(
        bufferPoolCfgMap, buffer.globalShared, buffer.globalHeadroom);
    newCfg.bufferPoolConfigs() = bufferPoolCfgMap;
    cfg_ = newCfg;
    applyNewConfig(newCfg);
  }

  HwSwitchEnsemble::Features featuresDesired() const override {
    return {HwSwitchEnsemble::LINKSCAN, HwSwitchEnsemble::PACKET_RX};
  }

  std::tuple<int, int, int> getTxRxXonPfcCounters(
      const facebook::fboss::PortID& portId,
      const int pfcPriority) {
    auto portStats = getLatestPortStats(portId);
    return getPfcTxRxXonHwPortStats(
        getHwSwitchEnsemble(), portStats, pfcPriority);
  }

  void validateInitPfcCounters(
      const std::vector<PortID>& portIds,
      const int pfcPriority) {
    int txPfcCtr = 0, rxPfcCtr = 0, rxPfcXonCtr = 0;
    // no need to retry if looking for baseline counter
    for (const auto& portId : portIds) {
      auto portStats = getHwSwitchEnsemble()->getLatestPortStats(portId);
      auto ingressDropRaw = *portStats.inDiscardsRaw_();
      XLOG(DBG0) << " validateInitPfcCounters: Port: " << portId
                 << " IngressDropRaw: " << ingressDropRaw;
      EXPECT_TRUE(ingressDropRaw == 0);
      std::tie(txPfcCtr, rxPfcCtr, rxPfcXonCtr) =
          getTxRxXonPfcCounters(portId, pfcPriority);
      EXPECT_TRUE((txPfcCtr == 0) && (rxPfcCtr == 0) && (rxPfcXonCtr == 0));
    }
  }

  void validateIngressDropCounters(const std::vector<PortID>& portIds) {
    WITH_RETRIES({
      for (const auto& portId : portIds) {
        auto portStats = getHwSwitchEnsemble()->getLatestPortStats(portId);
        auto ingressDropRaw = *portStats.inDiscardsRaw_();
        uint64_t ingressCongestionDiscards = 0;
        std::string ingressCongestionDiscardLog{};
        if (getHwSwitchEnsemble()->getAsic()->isSupported(
                HwAsic::Feature::INGRESS_PRIORITY_GROUP_DROPPED_PACKETS)) {
          ingressCongestionDiscards = *portStats.inCongestionDiscards_();
          ingressCongestionDiscardLog = " IngressCongestionDiscards: " +
              std::to_string(ingressCongestionDiscards);
        }
        XLOG(DBG0) << " validateIngressDropCounters: Port: " << portId
                   << " IngressDropRaw: " << ingressDropRaw
                   << ingressCongestionDiscardLog;
        EXPECT_EVENTUALLY_GT(ingressDropRaw, 0);
        if (getHwSwitchEnsemble()->getAsic()->isSupported(
                HwAsic::Feature::INGRESS_PRIORITY_GROUP_DROPPED_PACKETS)) {
          EXPECT_EVENTUALLY_GT(ingressCongestionDiscards, 0);
        }
      }
      if (getAsic()->getAsicType() == cfg::AsicType::ASIC_TYPE_JERICHO3) {
        // Jericho3 has additional VSQ drops counters which accounts for
        // ingress buffer drops.
        getHwSwitchEnsemble()->getHwSwitch()->updateStats();
        fb303::ThreadCachedServiceData::get()->publishStats();
        auto vsqResourcesExhautionDrops = getHwSwitchEnsemble()
                                              ->getHwSwitch()
                                              ->getSwitchStats()
                                              ->getVsqResourcesExhautionDrops();
        XLOG(DBG0)
            << " validateIngressDropCounters: vsqResourceExhaustionDrops: "
            << vsqResourcesExhautionDrops;
        EXPECT_EVENTUALLY_GT(vsqResourcesExhautionDrops, 0);
      }
    });
  }

 protected:
  void runTestWithCfg(
      const int trafficClass,
      const int pfcPriority,
      TrafficTestParams testParams = TrafficTestParams{},
      std::function<void(
          HwSwitchEnsemble* ensemble,
          const int pri,
          const std::vector<PortID>& portIdsToValidate)> validateCounterFn =
          validatePfcCounters) {
    std::vector<PortID> portIds = portIdsForTest();
    auto setup = [&]() {
      if ((getAsic()->getAsicType() == cfg::AsicType::ASIC_TYPE_JERICHO2) ||
          (getAsic()->getAsicType() == cfg::AsicType::ASIC_TYPE_JERICHO3)) {
        // Keep low scaling factor so that headroom usage is attempted
        // for Jericho family of ASICs.
        testParams.buffer.scalingFactor = cfg::MMUScalingFactor::ONE_32768TH;
      }
      setupBuffers(testParams.buffer, testParams.scale);
      setupEcmpTraffic(portIds);
      // ensure counter is 0 before we start traffic
      validateInitPfcCounters(portIds, pfcPriority);
    };
    auto verifyCommon = [&](bool postWb) {
      pumpTraffic(trafficClass);
      // check counters are as expected
      validateCounterFn(getHwSwitchEnsemble(), pfcPriority, portIds);
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

  void setupEcmpTraffic(const std::vector<PortID>& portIds) {
    utility::EcmpSetupTargetedPorts6 ecmpHelper{
        getProgrammedState(), getIntfMac()};

    CHECK_EQ(portIds.size(), kDestIps().size());
    for (int i = 0; i < portIds.size(); ++i) {
      const PortDescriptor port(portIds[i]);
      RoutePrefixV6 route{kDestIps()[i], 128};
      applyNewState(ecmpHelper.resolveNextHops(getProgrammedState(), {port}));
      ecmpHelper.programRoutes(getRouteUpdater(), {port}, {route});
      utility::ttlDecrementHandlingForLoopbackTraffic(
          getHwSwitchEnsemble(),
          ecmpHelper.getRouterId(),
          ecmpHelper.nhop(port));
    }
  }

  void setupConfigAndEcmpTraffic() {
    setupBuffers();
    setupEcmpTraffic(portIdsForTest());
  }

 public:
  void pumpTraffic(const int priority) {
    auto vlanId = utility::firstVlanID(initialConfig());
    auto intfMac = getIntfMac();
    auto srcMac = utility::MacAddressGenerator().get(intfMac.u64NBO() + 1);
    // pri = 7 => dscp 56
    int dscp = priority * 8;
    // Tomahawk4 need 5 packets per flow to trigger PFC
    int numPacketsPerFlow = getHwSwitchEnsemble()->getMinPktsForLineRate(
        masterLogicalInterfacePortIds()[0]);
    for (int i = 0; i < numPacketsPerFlow; i++) {
      for (const auto& dstIp : kDestIps()) {
        auto txPacket = utility::makeUDPTxPacket(
            getHwSwitch(),
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

        getHwSwitch()->sendPacketSwitchedSync(std::move(txPacket));
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

  bool canTriggerPfcDeadlockDetectionWithTraffic() {
    // Return false for ASICs that cannot trigger PFC detection with
    // loop traffic!
    if ((getAsic()->getAsicType() == cfg::AsicType::ASIC_TYPE_JERICHO2) ||
        (getAsic()->getAsicType() == cfg::AsicType::ASIC_TYPE_JERICHO3)) {
      return false;
    } else {
      return true;
    }
  }

  void setupWatchdog(bool enable) {
    cfg::PfcWatchdog pfcWatchdog;
    if (enable) {
      pfcWatchdog.recoveryAction() = cfg::PfcWatchdogRecoveryAction::NO_DROP;
      if (canTriggerPfcDeadlockDetectionWithTraffic()) {
        pfcWatchdog.recoveryTimeMsecs() = 10;
        pfcWatchdog.detectionTimeMsecs() = 1;
      } else {
        pfcWatchdog.recoveryTimeMsecs() = 1000;
        pfcWatchdog.detectionTimeMsecs() = 200;
      }
    }

    for (const auto& portID : portIdsForTest()) {
      auto portCfg = utility::findCfgPort(cfg_, portID);
      if (portCfg->pfc().has_value()) {
        if (enable) {
          portCfg->pfc()->watchdog() = pfcWatchdog;
        } else {
          portCfg->pfc()->watchdog().reset();
        }
      }
    }
    applyNewConfig(cfg_);
  }

  void validatePfcWatchdogCountersReset(const PortID& port) {
    auto deadlockCtr =
        getHwSwitchEnsemble()->readPfcDeadlockDetectionCounter(port);
    auto recoveryCtr =
        getHwSwitchEnsemble()->readPfcDeadlockRecoveryCounter(port);
    XLOG(DBG2) << "deadlockCtr:" << deadlockCtr
               << ", recoveryCtr:" << recoveryCtr;
    EXPECT_TRUE((deadlockCtr == 0) && (recoveryCtr == 0));
  }

  void validatePfcWatchdogCounters(const PortID& port) {
    int deadlockCtrBefore =
        getHwSwitchEnsemble()->readPfcDeadlockDetectionCounter(port);
    int recoveryCtrBefore =
        getHwSwitchEnsemble()->readPfcDeadlockRecoveryCounter(port);
    WITH_RETRIES_N_TIMED(10, std::chrono::milliseconds(1000), {
      int deadlockCtr =
          getHwSwitchEnsemble()->readPfcDeadlockDetectionCounter(port);
      int recoveryCtr =
          getHwSwitchEnsemble()->readPfcDeadlockRecoveryCounter(port);
      XLOG(DBG0) << "For port: " << port << " deadlockCtr = " << deadlockCtr
                 << " recoveryCtr = " << recoveryCtr;
      EXPECT_EVENTUALLY_TRUE(
          (deadlockCtr > deadlockCtrBefore) &&
          (recoveryCtr > recoveryCtrBefore));
    });
  }

  void validateRxPfcCounterIncrement(
      const PortID& port,
      const int pfcPriority) {
    int retries = 2;
    int rxPfcCtrOld = 0;
    std::tie(std::ignore, rxPfcCtrOld, std::ignore) =
        getTxRxXonPfcCounters(port, pfcPriority);
    while (retries--) {
      int rxPfcCtrNew = 0;
      std::this_thread::sleep_for(std::chrono::seconds(1));
      std::tie(std::ignore, rxPfcCtrNew, std::ignore) =
          getTxRxXonPfcCounters(port, pfcPriority);
      if (rxPfcCtrNew > rxPfcCtrOld) {
        return;
      }
    }
    EXPECT_TRUE(0);
  }

  void triggerPfcDeadlockDetection(const PortID& port) {
    if ((getAsic()->getAsicType() == cfg::AsicType::ASIC_TYPE_JERICHO2) ||
        (getAsic()->getAsicType() == cfg::AsicType::ASIC_TYPE_JERICHO3)) {
      // As traffic cannot trigger deadlock for DNX, force back
      // to back PFC frame generation which causes a deadlock!
      XLOG(DBG0) << "Triggering PFC deadlock detection on port ID : "
                 << static_cast<int>(port);
      auto iter = kRegValToForcePfcTxForPriorityOnPortDnx.find(std::make_tuple(
          getAsic()->getAsicType(), static_cast<int>(port), kLosslessPriority));
      EXPECT_FALSE(iter == kRegValToForcePfcTxForPriorityOnPortDnx.end());
      std::string out;
      getHwSwitchEnsemble()->runDiagCommand(
          "modreg CFC_FRC_NIF_ETH_PFC FRC_NIF_ETH_PFC=" + iter->second +
              "\nquit\n",
          out);
    } else {
      // Send traffic to trigger deadlock
      pumpTraffic(kLosslessTrafficClass);
      validateRxPfcCounterIncrement(port, kLosslessPriority);
    }
  }

  std::map<int, int> tc2PgOverride = {};
  std::map<int, int> pfcPri2PgIdOverride = {};
  cfg::SwitchConfig cfg_;
};

class HwTrafficPfcGenTest
    : public HwTrafficPfcTest,
      public testing::WithParamInterface<TrafficTestParams> {
  void SetUp() override {
    auto testParams = GetParam();
    FLAGS_mmu_lossless_mode = true;
    /*
     * Makes this flag available so that it can be used in early
     * stages of init to setup common buffer pool for specific
     * asics like Jericho2.
     */
    FLAGS_ingress_egress_buffer_pool_size = kGlobalIngressEgressBufferPoolSize;
    if (testParams.buffer.pgHeadroom == 0) {
      // Force headroom 0 for lossless PG
      FLAGS_allow_zero_headroom_for_lossless_pg = true;
    }
    HwLinkStateDependentTest::SetUp();
  }
};

INSTANTIATE_TEST_SUITE_P(
    HwTrafficPfcTest,
    HwTrafficPfcGenTest,
    testing::Values(
        TrafficTestParams{},
        TrafficTestParams{
            .buffer =
                PfcBufferParams{
                    .globalShared = kGlobalSharedBytes * 5,
                    .pgLimit = kPgLimitBytes / 3,
                    .resumeOffset = kPgResumeOffsetBytes / 3},
            .scale = true},
        TrafficTestParams{
            .buffer =
                PfcBufferParams{.pgHeadroom = 0, .scalingFactor = std::nullopt},
            .expectDrop = true},
        TrafficTestParams{
            .buffer =
                PfcBufferParams{
                    .globalHeadroom = 0,
                    .scalingFactor = std::nullopt},
            .expectDrop = true}),
    [](const ::testing::TestParamInfo<TrafficTestParams>& info) {
      auto testParams = info.param;
      if (testParams.buffer.pgHeadroom == 0) {
        return "WithZeroPgHeadRoomCfg";
      } else if (testParams.buffer.globalHeadroom == 0) {
        return "WithZeroGlobalHeadRoomCfg";
      } else if (testParams.scale) {
        return "WithScaleCfg";
      } else {
        return "WithDefaultCfg";
      }
    });

TEST_P(HwTrafficPfcGenTest, verifyPfc) {
  const int trafficClass = kLosslessTrafficClass;
  const int pfcPriority = kLosslessPriority;
  TrafficTestParams trafficParams = GetParam();
  runTestWithCfg(trafficClass, pfcPriority, trafficParams);
}

TEST_F(HwTrafficPfcTest, verifyBufferPoolWatermarks) {
  const int trafficClass = kLosslessTrafficClass;
  const int pfcPriority = kLosslessPriority;
  cfg::MMUScalingFactor scalingFactor =
      getAsic()->getAsicType() == cfg::AsicType::ASIC_TYPE_JERICHO2
      ? cfg::MMUScalingFactor::ONE_32768TH
      : cfg::MMUScalingFactor::ONE_64TH;
  runTestWithCfg(
      trafficClass,
      pfcPriority,
      TrafficTestParams{
          .buffer = PfcBufferParams{.scalingFactor = scalingFactor}},
      validateBufferPoolWatermarkCounters);
}

TEST_F(HwTrafficPfcTest, verifyIngressPriorityGroupWatermarks) {
  const int trafficClass = kLosslessTrafficClass;
  const int pfcPriority = kLosslessPriority;
  runTestWithCfg(
      trafficClass,
      pfcPriority,
      TrafficTestParams{
          .buffer = PfcBufferParams{.scalingFactor = std::nullopt}},
      validateIngressPriorityGroupWatermarkCounters);
}

// intent of this test is to send traffic so that it maps to
// tc 2, now map tc 2 to PG 3. Mapping from PG to pfc priority
// is 1:1, which means PG 3 is mapped to pfc priority 3.
// Generate traffic to fire off PFC with smaller shared buffer
TEST_F(HwTrafficPfcTest, verifyPfcWithMapChanges_0) {
  const int trafficClass = kLosslessTrafficClass;
  const int pfcPriority = 3;
  tc2PgOverride.insert(std::make_pair(trafficClass, pfcPriority));
  runTestWithCfg(trafficClass, pfcPriority);
}

// intent of this test is to send traffic so that it maps to
// tc 7. Now we map tc 7 -> PG 2. Mapping from PG to pfc
// priority is 1:1, which means PG 2 is mapped to pfc priority 2.
// Generate traffic to fire off PFC with smaller shared buffer
TEST_F(HwTrafficPfcTest, verifyPfcWithMapChanges_1) {
  const int trafficClass = 7;
  const int pfcPriority = kLosslessPriority;
  tc2PgOverride.insert(std::make_pair(trafficClass, pfcPriority));
  runTestWithCfg(trafficClass, pfcPriority);
}

// intent of this test is to setup watchdog for the PFC
// and observe it kicks in and update the watchdog counters
// watchdog counters are created/incremented when callback
// for deadlock/recovery kicks in
TEST_F(HwTrafficPfcTest, PfcWatchdog) {
  auto setup = [&]() {
    setupConfigAndEcmpTraffic();
    setupWatchdog(true /* enable watchdog */);
  };
  auto verify = [&]() {
    triggerPfcDeadlockDetection(masterLogicalInterfacePortIds()[0]);
    validatePfcWatchdogCounters(masterLogicalInterfacePortIds()[0]);
  };
  verifyAcrossWarmBoots(setup, verify);
}

// intent of this test is to setup watchdog for PFC
// and remove it. Observe that counters should stop incrementing
// Clear them and check they stay the same
// Since the watchdog counters are sw based, upon warm boot
// we don't expect these counters to be incremented either
TEST_F(HwTrafficPfcTest, PfcWatchdogReset) {
  auto setup = [&]() {
    setupConfigAndEcmpTraffic();
    setupWatchdog(true /* enable watchdog */);
    triggerPfcDeadlockDetection(masterLogicalInterfacePortIds()[0]);
    // lets wait for the watchdog counters to be populated
    validatePfcWatchdogCounters(masterLogicalInterfacePortIds()[0]);
    // reset watchdog
    setupWatchdog(false /* disable */);
    // reset the watchdog counters
    getHwSwitchEnsemble()->clearPfcDeadlockRecoveryCounter(
        masterLogicalInterfacePortIds()[0]);
    getHwSwitchEnsemble()->clearPfcDeadlockDetectionCounter(
        masterLogicalInterfacePortIds()[0]);
  };

  auto verify = [&]() {
    // ensure that RX PFC continues to increment
    validateRxPfcCounterIncrement(
        masterLogicalInterfacePortIds()[0], kLosslessPriority);
    // validate that pfc watchdog counters do not increment anymore
    validatePfcWatchdogCountersReset(masterLogicalInterfacePortIds()[0]);
  };

  // warmboot support to be added in next step
  verifyAcrossWarmBoots(setup, verify);
}

} // namespace facebook::fboss
