#include "fboss/agent/Platform.h"
#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/hw/test/HwLinkStateDependentTest.h"
#include "fboss/agent/hw/test/HwTestPacketUtils.h"
#include "fboss/agent/test/EcmpSetupHelper.h"

#include "fboss/agent/hw/test/dataplane_tests/HwTestQosUtils.h"
#include "fboss/agent/state/Interface.h"
#include "fboss/agent/state/Port.h"
#include "fboss/agent/state/PortMap.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/test/ResourceLibUtil.h"

using folly::IPAddress;
using folly::IPAddressV6;
using std::string;

namespace facebook::fboss {

class HwPfcTest : public HwLinkStateDependentTest {
 private:
  void SetUp() override {
    FLAGS_mmu_lossless_mode = true;
    HwLinkStateDependentTest::SetUp();
  }

  cfg::SwitchConfig initialConfig() const override {
    std::vector<PortID> ports = {
        masterLogicalPortIds()[0],
        masterLogicalPortIds()[1],
    };
    auto cfg = utility::onePortPerVlanConfig(
        getHwSwitch(), std::move(ports), cfg::PortLoopbackMode::MAC);
    return cfg;
  }
  folly::IPAddressV6 kDstIp1() const {
    return folly::IPAddressV6{"100::1"};
  }
  folly::IPAddressV6 kDstIp2() const {
    return folly::IPAddressV6{"200::1"};
  }
  folly::CIDRNetwork kPrefix1() const {
    return std::make_pair(folly::IPAddress{"100::"}, 64);
  }
  folly::CIDRNetwork kPrefix2() const {
    return std::make_pair(folly::IPAddress{"200::"}, 64);
  }
  PortDescriptor portDesc1() const {
    return PortDescriptor(masterLogicalPortIds()[0]);
  }
  PortDescriptor portDesc2() const {
    return PortDescriptor(masterLogicalPortIds()[1]);
  }

  void setupECMPForwarding(
      const utility::EcmpSetupTargetedPorts6& ecmpHelper,
      PortDescriptor port,
      const folly::CIDRNetwork& prefix) {
    RoutePrefixV6 route{prefix.first.asV6(), prefix.second};
    applyNewState(ecmpHelper.resolveNextHops(getProgrammedState(), {port}));
    ecmpHelper.programRoutes(getRouteUpdater(), {port}, {route});
  }
  template <typename ECMP_HELPER>
  void disableTTLDecrements(const ECMP_HELPER& ecmpHelper) {
    for (const auto& nextHop :
         {ecmpHelper.nhop(portDesc1()), ecmpHelper.nhop(portDesc2())}) {
      utility::disableTTLDecrements(
          getHwSwitch(), ecmpHelper.getRouterId(), nextHop);
    }
  }

  folly::MacAddress getIntfMac() const {
    auto vlanId = utility::firstVlanID(initialConfig());
    return utility::getInterfaceMac(getProgrammedState(), vlanId);
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

    qosMap.dscpMaps_ref()->resize(8);
    for (auto i = 0; i < 8; i++) {
      qosMap.dscpMaps_ref()[i].internalTrafficClass_ref() = i;
      for (auto j = 0; j < 8; j++) {
        qosMap.dscpMaps_ref()[i].fromDscpToTrafficClass_ref()->push_back(
            8 * i + j);
      }
    }
    qosMap.trafficClassToPgId_ref() = std::move(tc2PgId);
    qosMap.pfcPriorityToPgId_ref() = std::move(pfcPri2PgId);
    qosMap.pfcPriorityToQueueId_ref() = std::move(pfcPri2QueueId);
  }

  void setupPfc(cfg::SwitchConfig& cfg, const std::vector<PortID>& ports) {
    cfg::PortPfc pfc;
    pfc.tx_ref() = true;
    pfc.rx_ref() = true;
    pfc.portPgConfigName_ref() = "foo";

    cfg::QosMap qosMap;
    // setup qos map with pfc structs
    setupQosMapForPfc(qosMap);

    // setup qosPolicy
    cfg.qosPolicies_ref()->resize(1);
    cfg.qosPolicies_ref()[0].name_ref() = "qp";
    cfg.qosPolicies_ref()[0].qosMap_ref() = qosMap;
    cfg::TrafficPolicyConfig dataPlaneTrafficPolicy;
    dataPlaneTrafficPolicy.defaultQosPolicy_ref() = "qp";
    cfg.dataPlaneTrafficPolicy_ref() = dataPlaneTrafficPolicy;

    for (const auto& portID : ports) {
      auto portCfg = std::find_if(
          cfg.ports_ref()->begin(),
          cfg.ports_ref()->end(),
          [&portID](auto& port) {
            return PortID(*port.logicalID_ref()) == portID;
          });
      portCfg->pfc_ref() = pfc;
    }
  }

  void setupHelper() {
    auto newCfg{initialConfig()};
    setupPfc(newCfg, {masterLogicalPortIds()[0], masterLogicalPortIds()[1]});

    std::vector<cfg::PortPgConfig> portPgConfigs;
    std::map<std::string, std::vector<cfg::PortPgConfig>> portPgConfigMap;

    // create 2 pgs
    for (auto pgId = 0; pgId < 2; ++pgId) {
      cfg::PortPgConfig pgConfig;
      pgConfig.id_ref() = pgId;
      pgConfig.bufferPoolName_ref() = "bufferNew";
      // provide atleast 1 cell worth of minLimit
      pgConfig.minLimitBytes_ref() = 300;
      portPgConfigs.emplace_back(pgConfig);
    }

    portPgConfigMap["foo"] = portPgConfigs;
    newCfg.portPgConfigs_ref() = portPgConfigMap;

    // create buffer pool
    std::map<std::string, cfg::BufferPoolConfig> bufferPoolCfgMap;
    cfg::BufferPoolConfig poolConfig;
    // provide small shared buffer size
    // idea is to hit the limit and trigger XOFF (PFC)
    poolConfig.sharedBytes_ref() = 10000;
    bufferPoolCfgMap.insert(std::make_pair("bufferNew", poolConfig));
    newCfg.bufferPoolConfigs_ref() = bufferPoolCfgMap;
    cfg_ = newCfg;
    applyNewConfig(newCfg);
  }

  HwSwitchEnsemble::Features featuresDesired() const override {
    return {HwSwitchEnsemble::LINKSCAN, HwSwitchEnsemble::PACKET_RX};
  }

  std::tuple<int, int> getTxRxPfcCounters(
      const PortID& portId,
      const int pfcPriority) {
    int txPfcCtr = getLatestPortStats(portId).get_outPfc_().at(pfcPriority);
    int rxPfcCtr = getLatestPortStats(portId).get_inPfc_().at(pfcPriority);
    return {txPfcCtr, rxPfcCtr};
  }

  void validateInitPfcCounters(
      const std::vector<PortID>& portIds,
      const int pfcPriority) {
    int txPfcCtr = 0, rxPfcCtr = 0;
    // no need to retry if looking for baseline counter
    for (const auto& portId : portIds) {
      std::tie(txPfcCtr, rxPfcCtr) = getTxRxPfcCounters(portId, pfcPriority);
      EXPECT_TRUE((txPfcCtr == 0) && (rxPfcCtr == 0));
    }
  }

  bool getPfcCountersRetry(const PortID& portId, const int pfcPriority) {
    int txPfcCtr = 0, rxPfcCtr = 0;
    int retries = 5;
    // retry as long as we can OR we get an expected output
    while (retries--) {
      // sleep for a bit before checking counters
      std::this_thread::sleep_for(std::chrono::seconds(1));
      std::tie(txPfcCtr, rxPfcCtr) = getTxRxPfcCounters(portId, pfcPriority);
      if (txPfcCtr > 0 && rxPfcCtr > 0) {
        // there is no undoing this state
        break;
      }
    };
    XLOG(DBG0) << " Port: " << portId << " PFC TX/RX PFC " << txPfcCtr << "/"
               << rxPfcCtr << " , priority: " << pfcPriority;
    if (txPfcCtr > 0 && rxPfcCtr > 0) {
      return true;
    }
    return false;
  }

  void validatePfcCounters(const int pri, const std::vector<PortID>& portIds) {
    // no need t retry if looking for baseline counter
    for (const auto& portId : portIds) {
      EXPECT_TRUE(getPfcCountersRetry(portId, pri));
    }
  }

 protected:
  void runTest(const int trafficClass, const int pfcPriority) {
    auto setup = [&]() {
      setupConfigAndEcmpTraffic();
      validateInitPfcCounters(
          {masterLogicalPortIds()[0], masterLogicalPortIds()[1]}, pfcPriority);
    };
    auto verify = [&]() {
      // ensure counter is 0 before we start traffic
      pumpTraffic(trafficClass);
      // ensure counter is > 0, after the traffic
      validatePfcCounters(
          pfcPriority, {masterLogicalPortIds()[0], masterLogicalPortIds()[1]});
    };
    verifyAcrossWarmBoots(setup, verify);
  }

  void setupConfigAndEcmpTraffic() {
    setupHelper();
    utility::EcmpSetupTargetedPorts6 ecmpHelper6{
        getProgrammedState(), getIntfMac()};
    setupECMPForwarding(
        ecmpHelper6, PortDescriptor(masterLogicalPortIds()[0]), kPrefix1());
    setupECMPForwarding(
        ecmpHelper6, PortDescriptor(masterLogicalPortIds()[1]), kPrefix2());
    disableTTLDecrements(ecmpHelper6);
  }

 public:
  void pumpTraffic(const int priority) {
    auto vlanId = utility::firstVlanID(initialConfig());
    auto intfMac = getIntfMac();
    auto srcMac = utility::MacAddressGenerator().get(intfMac.u64NBO() + 1);
    // pri = 7 => dscp 56
    int dscp = priority * 8;
    for (const auto& dstIp : {kDstIp1(), kDstIp2()}) {
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
          std::vector<uint8_t>(7000, 0xff));

      getHwSwitch()->sendPacketSwitchedSync(std::move(txPacket));
    }
  }

  void setupWatchdog(bool enable) {
    cfg::PfcWatchdog pfcWatchdog;
    if (enable) {
      pfcWatchdog.recoveryAction_ref() =
          cfg::PfcWatchdogRecoveryAction::NO_DROP;
      pfcWatchdog.recoveryTimeMsecs_ref() = 10;
      pfcWatchdog.detectionTimeMsecs_ref() = 1;
    }

    for (const auto& portID :
         {masterLogicalPortIds()[0], masterLogicalPortIds()[1]}) {
      auto portCfg = utility::findCfgPort(cfg_, portID);
      if (portCfg->pfc_ref().has_value()) {
        if (enable) {
          portCfg->pfc_ref()->watchdog_ref() = pfcWatchdog;
        } else {
          portCfg->pfc_ref()->watchdog_ref().reset();
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
    XLOG(INFO) << "deadlockCtr:" << deadlockCtr
               << ", recoveryCtr:" << recoveryCtr;
    EXPECT_TRUE((deadlockCtr == 0) && (recoveryCtr == 0));
  }

  void validatePfcWatchdogCounters(const PortID& port) {
    int deadlockCtr = 0, recoveryCtr = 0;
    int retries = 10;
    bool countersHit = false;
    while (retries--) {
      std::this_thread::sleep_for(std::chrono::seconds(1));
      deadlockCtr =
          getHwSwitchEnsemble()->readPfcDeadlockDetectionCounter(port);
      recoveryCtr = getHwSwitchEnsemble()->readPfcDeadlockRecoveryCounter(port);
      if (deadlockCtr > 0 && recoveryCtr > 0) {
        countersHit = true;
        break;
      }
    }
    XLOG(DBG0) << "For port: " << port << " deadlockCtr = " << deadlockCtr
               << " recoveryCtr = " << recoveryCtr;
    EXPECT_TRUE(countersHit);
  }

  void validateRxPfcCounterIncrement(const PortID& port) {
    int retries = 2;
    int rxPfcCtrOld = 0;
    std::tie(std::ignore, rxPfcCtrOld) = getTxRxPfcCounters(port, 0);
    while (retries--) {
      int rxPfcCtrNew = 0;
      std::this_thread::sleep_for(std::chrono::seconds(1));
      std::tie(std::ignore, rxPfcCtrNew) = getTxRxPfcCounters(port, 0);
      if (rxPfcCtrNew > rxPfcCtrOld) {
        return;
      }
    }
    EXPECT_TRUE(0);
  }

  std::map<int, int> tc2PgOverride = {};
  std::map<int, int> pfcPri2PgIdOverride = {};
  cfg::SwitchConfig cfg_;
};

TEST_F(HwPfcTest, verifyPfcDefault) {
  if (!HwTest::isSupported(HwAsic::Feature::PFC)) {
    return;
  }
  // default to map dscp to priority = 0
  const int trafficClass = 0;
  const int pfcPriority = 0;
  runTest(trafficClass, pfcPriority);
}

// intent of this test is to send traffic so that it maps to
// tc 0 which maps to PG0. Now map PG0 to pfc priority 7
// Generate traffic to fire off PFC with smaller shared buffer
TEST_F(HwPfcTest, verifyPfcWithMapChanges_0) {
  if (!HwTest::isSupported(HwAsic::Feature::PFC)) {
    return;
  }
  const int trafficClass = 0;
  const int pfcPriority = 7;
  pfcPri2PgIdOverride.insert(std::make_pair(7, 0));
  runTest(trafficClass, pfcPriority);
}

// intent of this test is to send traffic so that it maps to
// tc 7. Now we map tc 7 -> PG 1
// and map PG 1 -> PFC Priority 7
// Generate traffic to fire off PFC with smaller shared buffer
TEST_F(HwPfcTest, verifyPfcWithMapChanges_1) {
  if (!HwTest::isSupported(HwAsic::Feature::PFC)) {
    return;
  }
  const int trafficClass = 7;
  const int pfcPriority = 7;
  tc2PgOverride.insert(std::make_pair(7, 1));
  pfcPri2PgIdOverride.insert(std::make_pair(7, 1));
  runTest(trafficClass, pfcPriority);
}

// intent of this test is to setup watchdog for the PFC
// and observe it kicks in and update the watchdog counters
// watchdog counters are created/incremented when callback
// for deadlock/recovery kicks in
TEST_F(HwPfcTest, PfcWatchdog) {
  if (!HwTest::isSupported(HwAsic::Feature::PFC)) {
    return;
  }
  auto setup = [&]() {
    setupConfigAndEcmpTraffic();
    setupWatchdog(true /* enable watchdog */);
  };
  auto verify = [&]() {
    validatePfcWatchdogCountersReset(masterLogicalPortIds()[0]);
    pumpTraffic(0 /* traffic class */);
    validateRxPfcCounterIncrement(masterLogicalPortIds()[0]);
    validatePfcWatchdogCounters(masterLogicalPortIds()[0]);
  };
  // warmboot support to be added in next step
  setup();
  verify();
}

// intent of this test is to setup watchdog for PFC
// and remove it. Observe that counters should stop incrementing
// Clear them and check they stay the same
// Since the watchdog counters are sw based, upon warm boot
// we don't expect these counters to be incremented either
TEST_F(HwPfcTest, PfcWatchdogReset) {
  if (!HwTest::isSupported(HwAsic::Feature::PFC)) {
    return;
  }
  auto setup = [&]() {
    setupConfigAndEcmpTraffic();
    setupWatchdog(true /* enable watchdog */);
    pumpTraffic(0 /* traffic class */);
    // lets wait for the watchdog counters to be populated
    validatePfcWatchdogCounters(masterLogicalPortIds()[0]);
    // reset watchdog
    setupWatchdog(false /* disable */);
    // reset the watchdog counters
    getHwSwitchEnsemble()->clearPfcDeadlockRecoveryCounter(
        masterLogicalPortIds()[0]);
    getHwSwitchEnsemble()->clearPfcDeadlockDetectionCounter(
        masterLogicalPortIds()[0]);
  };

  auto verify = [&]() {
    // ensure that RX PFC continues to increment
    validateRxPfcCounterIncrement(masterLogicalPortIds()[0]);
    // validate that pfc watchdog counters do not increment anymore
    validatePfcWatchdogCountersReset(masterLogicalPortIds()[0]);
  };

  // warmboot support to be added in next step
  verifyAcrossWarmBoots(setup, verify);
}

} // namespace facebook::fboss
