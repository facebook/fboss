// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include "fboss/agent/test/AgentHwTest.h"
#include "fboss/agent/HwAsicTable.h"
#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/hw/test/HwTestCoppUtils.h"
#include "fboss/lib/CommonUtils.h"

DEFINE_bool(run_forever, false, "run the test forever");
DEFINE_bool(run_forever_on_failure, false, "run the test forever on failure");
DEFINE_bool(
    list_production_feature,
    false,
    "list production feature needed for every single test");
DECLARE_bool(disable_neighbor_updates);
DECLARE_bool(disable_icmp_error_response);

namespace {
int kArgc;
char** kArgv;
} // namespace

namespace facebook::fboss {
void AgentHwTest::SetUp() {
  gflags::ParseCommandLineFlags(&kArgc, &kArgv, false);
  if (FLAGS_list_production_feature) {
    printProductionFeatures();
    return;
  }
  fbossCommonInit(kArgc, kArgv);
  FLAGS_verify_apply_oper_delta = true;
  FLAGS_hide_fabric_ports = hideFabricPorts();
  // Reset any global state being tracked in singletons
  // Each test then sets up its own state as needed.
  folly::SingletonVault::singleton()->destroyInstances();
  folly::SingletonVault::singleton()->reenableInstances();
  // Set watermark stats update interval to 0 so we always refresh BST stats
  // in each updateStats call
  FLAGS_update_watermark_stats_interval_s = 0;
  // Don't send/receive periodic lldp packets. They will
  // interfere with tests.
  FLAGS_enable_lldp = false;
  // Disable tun intf, else pkts from host will interfere
  // with tests
  FLAGS_tun_intf = false;
  // disable neighbor updates
  FLAGS_disable_neighbor_updates = true;
  // disable icmp error response
  FLAGS_disable_icmp_error_response = true;
  // Disable FSDB publishing on single-box test
  FLAGS_publish_stats_to_fsdb = false;
  FLAGS_publish_state_to_fsdb = false;

  AgentEnsembleSwitchConfigFn initialConfigFn =
      [this](const AgentEnsemble& ensemble) { return initialConfig(ensemble); };
  agentEnsemble_ = createAgentEnsemble(initialConfigFn);
}

void AgentHwTest::TearDown() {
  if (FLAGS_run_forever ||
      (::testing::Test::HasFailure() && FLAGS_run_forever_on_failure)) {
    runForever();
  }
  tearDownAgentEnsemble();
}

void AgentHwTest::tearDownAgentEnsemble(bool doWarmboot) {
  if (!agentEnsemble_) {
    return;
  }

  if (::testing::Test::HasFailure()) {
    // TODO: Collect Info and dump counters
  }
  if (doWarmboot) {
    agentEnsemble_->gracefulExit();
  }
  agentEnsemble_.reset();
}

void AgentHwTest::runForever() const {
  XLOG(DBG2) << "AgentHwTest run forever...";
  while (true) {
    sleep(1);
    XLOG_EVERY_MS(DBG2, 5000) << "AgentHwTest running forever";
  }
}

std::shared_ptr<SwitchState> AgentHwTest::applyNewConfig(
    const cfg::SwitchConfig& config) {
  return agentEnsemble_->applyNewConfig(config);
}

SwSwitch* AgentHwTest::getSw() const {
  return agentEnsemble_->getSw();
}

const std::map<SwitchID, const HwAsic*> AgentHwTest::getAsics() const {
  return agentEnsemble_->getSw()->getHwAsicTable()->getHwAsics();
}

bool AgentHwTest::isSupportedOnAllAsics(HwAsic::Feature feature) const {
  // All Asics supporting the feature
  return agentEnsemble_->getSw()->getHwAsicTable()->isFeatureSupportedOnAllAsic(
      feature);
}

AgentEnsemble* AgentHwTest::getAgentEnsemble() const {
  return agentEnsemble_.get();
}

const std::shared_ptr<SwitchState> AgentHwTest::getProgrammedState() const {
  return getAgentEnsemble()->getProgrammedState();
}

std::vector<PortID> AgentHwTest::masterLogicalPortIds() const {
  return getAgentEnsemble()->masterLogicalPortIds();
}

std::vector<PortID> AgentHwTest::masterLogicalPortIds(
    const std::set<cfg::PortType>& portTypes) const {
  return getAgentEnsemble()->masterLogicalPortIds(portTypes);
}

void AgentHwTest::setSwitchDrainState(
    const cfg::SwitchConfig& curConfig,
    cfg::SwitchDrainState drainState) {
  auto newCfg = curConfig;
  *newCfg.switchSettings()->switchDrainState() = drainState;
  applyNewConfig(newCfg);
}

bool AgentHwTest::hideFabricPorts() const {
  // Due to the speedup in test run time (6m->21s on meru400biu)
  // we want to skip over fabric ports in a overwhelming
  // majority of test cases. Make this the default HwTest mode
  return true;
}

cfg::SwitchConfig AgentHwTest::initialConfig(
    const AgentEnsemble& ensemble) const {
  return utility::onePortPerInterfaceConfig(
      ensemble.getSw(),
      ensemble.masterLogicalPortIds(),
      true /*interfaceHasSubnet*/);
}

void AgentHwTest::printProductionFeatures() const {
  std::vector<std::string> asicFeatures;
  for (const auto& feature : getProductionFeaturesVerified()) {
    asicFeatures.push_back(apache::thrift::util::enumNameSafe(feature));
  }
  std::cout << "Feature List: " << folly::join(",", asicFeatures) << "\n";
  GTEST_SKIP();
}

std::map<PortID, HwPortStats> AgentHwTest::getLatestPortStats(
    const std::vector<PortID>& ports) {
  // Stats collection from SwSwitch is async, wait for stats
  // being available before returning here.
  std::map<PortID, HwPortStats> portStats;
  checkWithRetry(
      [&portStats, &ports, this]() {
        portStats = getSw()->getHwPortStats(ports);
        return !portStats.empty();
      },
      120,
      std::chrono::milliseconds(1000),
      " fetch port stats");
  return portStats;
}

HwPortStats AgentHwTest::getLatestPortStats(const PortID& port) {
  return getLatestPortStats(std::vector<PortID>({port})).begin()->second;
}

std::map<SystemPortID, HwSysPortStats> AgentHwTest::getLatestSysPortStats(
    const std::vector<SystemPortID>& ports) {
  std::map<std::string, HwSysPortStats> systemPortStats;
  std::map<SystemPortID, HwSysPortStats> portIdStatsMap;
  checkWithRetry(
      [&systemPortStats, &portIdStatsMap, &ports, this]() {
        portIdStatsMap.clear();
        getSw()->getAllHwSysPortStats(systemPortStats);
        for (auto [portStatName, stats] : systemPortStats) {
          SystemPortID portId;
          // Sysport stats names are suffixed with _switchIndex. Remove that
          // to get at sys port name
          auto portName =
              portStatName.substr(0, portStatName.find_last_of("_"));
          try {
            portId = getProgrammedState()
                         ->getSystemPorts()
                         ->getSystemPort(portName)
                         ->getID();
          } catch (const FbossError&) {
            // Look in remote sys ports if we couldn't find in local sys ports
            portId = getProgrammedState()
                         ->getRemoteSystemPorts()
                         ->getSystemPort(portName)
                         ->getID();
          }
          if (std::find(ports.begin(), ports.end(), portId) != ports.end()) {
            portIdStatsMap.emplace(portId, stats);
          }
        }
        return ports.size() == portIdStatsMap.size();
      },
      120,
      std::chrono::milliseconds(1000),
      " fetch system port stats");

  return portIdStatsMap;
}

HwSysPortStats AgentHwTest::getLatestSysPortStats(const SystemPortID& port) {
  return getLatestSysPortStats(std::vector<SystemPortID>({port}))
      .begin()
      ->second;
}

HwSwitchDropStats AgentHwTest::getAggregatedSwitchDropStats() {
  HwSwitchDropStats hwSwitchDropStats;
  checkWithRetry([&hwSwitchDropStats, this]() {
    HwSwitchDropStats aggHwSwitchDropStats;

    auto switchStats = getSw()->getHwSwitchStatsExpensive();
    for (const auto& switchId : getSw()->getHwAsicTable()->getSwitchIDs()) {
      if (switchStats.find(switchId) == switchStats.end()) {
        return false;
      }
      const auto& dropStats = *switchStats.at(switchId).switchDropStats();

#define FILL_DROP_COUNTERS(stat)                       \
  aggHwSwitchDropStats.stat##Drops() =                 \
      aggHwSwitchDropStats.stat##Drops().value_or(0) + \
      dropStats.stat##Drops().value_or(0);

      FILL_DROP_COUNTERS(global);
      FILL_DROP_COUNTERS(globalReachability);
      FILL_DROP_COUNTERS(packetIntegrity);
      FILL_DROP_COUNTERS(fdrCell);
      FILL_DROP_COUNTERS(voqResourceExhaustion);
      FILL_DROP_COUNTERS(globalResourceExhaustion);
      FILL_DROP_COUNTERS(sramResourceExhaustion);
      FILL_DROP_COUNTERS(vsqResourceExhaustion);
      FILL_DROP_COUNTERS(dropPrecedence);
      FILL_DROP_COUNTERS(queueResolution);
      FILL_DROP_COUNTERS(ingressPacketPipelineReject);
      FILL_DROP_COUNTERS(corruptedCellPacketIntegrity);
    }
    hwSwitchDropStats = aggHwSwitchDropStats;
    return true;
  });
  return hwSwitchDropStats;
}

std::map<uint16_t, HwSwitchWatermarkStats>
AgentHwTest::getAllSwitchWatermarkStats() {
  std::map<uint16_t, HwSwitchWatermarkStats> watermarkStats;
  const auto& hwSwitchStatsMap = getSw()->getHwSwitchStatsExpensive();
  for (const auto& [switchIdx, hwSwitchStats] : hwSwitchStatsMap) {
    watermarkStats.emplace(switchIdx, *hwSwitchStats.switchWatermarkStats());
  }
  return watermarkStats;
}

void AgentHwTest::applyNewStateImpl(
    StateUpdateFn fn,
    const std::string& name,
    bool transaction) {
  agentEnsemble_->applyNewState(fn, name, transaction);
}

cfg::SwitchConfig AgentHwTest::addCoppConfig(
    const AgentEnsemble& ensemble,
    const cfg::SwitchConfig& in) const {
  auto config = in;
  // Before m-mpu agent test, use first Asic for initialization.
  auto switchIds = ensemble.getSw()->getHwAsicTable()->getSwitchIDs();
  CHECK_GE(switchIds.size(), 1);
  auto asic =
      ensemble.getSw()->getHwAsicTable()->getHwAsic(*switchIds.cbegin());
  const auto& cpuStreamTypes =
      asic->getQueueStreamTypes(cfg::PortType::CPU_PORT);
  utility::setDefaultCpuTrafficPolicyConfig(config, asic, ensemble.isSai());
  utility::addCpuQueueConfig(config, asic, ensemble.isSai());
  return config;
}

void initAgentHwTest(
    int argc,
    char* argv[],
    PlatformInitFn initPlatform,
    std::optional<cfg::StreamType> streamType) {
  initEnsemble(initPlatform, streamType);
  kArgc = argc;
  kArgv = argv;
}

} // namespace facebook::fboss
