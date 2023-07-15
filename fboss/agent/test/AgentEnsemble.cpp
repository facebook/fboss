// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/test/AgentEnsemble.h"

#include "fboss/agent/AgentConfig.h"
#include "fboss/agent/SwitchStats.h"
#include "fboss/agent/Utils.h"

#include "fboss/agent/CommonInit.h"
#include "fboss/agent/EncapIndexAllocator.h"
#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/lib/config/PlatformConfigUtils.h"

DECLARE_bool(tun_intf);
DEFINE_bool(
    setup_for_warmboot,
    false,
    "Set up test for SDK warmboot. Useful for testing individual "
    "tests doing a full process warmboot and verifying expectations");

namespace {
int kArgc;
char** kArgv;
facebook::fboss::PlatformInitFn kPlatformInitFn;
static std::string kInputConfigFile;

} // namespace

namespace facebook::fboss {
AgentEnsemble::AgentEnsemble(const std::string& configFileName) {
  configFile_ = configFileName;
}

void AgentEnsemble::setupEnsemble(
    int argc,
    char** argv,
    uint32_t hwFeaturesDesired,
    PlatformInitFn initPlatform,
    AgentEnsembleSwitchConfigFn initialConfigFn,
    AgentEnsemblePlatformConfigFn platformConfigFn) {
  // to ensure FLAGS_config is set, as this is used in case platform config is
  // overriden by the application.
  gflags::ParseCommandLineFlags(&argc, &argv, false);

  FLAGS_verify_apply_oper_delta = true;

  if (platformConfigFn) {
    auto agentConf =
        AgentConfig::fromFile(AgentEnsemble::getInputConfigFile())->thrift;
    platformConfigFn(*(agentConf.platform()));
    // some platform config may need cold boots. so overwrite the config before
    // creating a switch
    writeConfig(agentConf, FLAGS_config);
  }
  auto config = fbossCommonInit(argc, argv);
  auto* initializer = agentInitializer();
  initializer->createSwitch(std::move(config), hwFeaturesDesired, initPlatform);

  utility::setPortToDefaultProfileIDMap(
      std::make_shared<MultiSwitchPortMap>(), getPlatform());
  auto portsByControllingPort =
      utility::getSubsidiaryPortIDs(getPlatform()->getPlatformPorts());

  for (const auto& port : portsByControllingPort) {
    masterLogicalPortIds_.push_back(port.first);
  }
  initialConfig_ = initialConfigFn(getHw(), masterLogicalPortIds_);
  applyInitialConfig(initialConfig_);
  // reload the new config
  getPlatform()->reloadConfig();
}

void AgentEnsemble::startAgent() {
  // TODO: provide a way to enable tun intf, for now disable it expressedly,
  // this can also be done with CLI option, however netcastle runners can not
  // use that option, because hw tests and hw benchmarks using hwswitch ensemble
  // doesn't have CLI option to disable tun intf. Also get rid of explicit
  // setting this flag and emply CLI option to disable tun manager.
  FLAGS_tun_intf = false;
  auto* initializer = agentInitializer();
  asyncInitThread_.reset(new std::thread([this, initializer] {
    // hardware switch events will be dispatched to agent ensemble
    // agent ensemble is responsible to dispatch them to SwSwitch
    initializer->initAgent(this);
  }));
  initializer->initializer()->waitForInitDone();
  // if cold booting, invoke link toggler
  if (getHw()->getBootType() != BootType::WARM_BOOT &&
      linkToggler_ != nullptr) {
    linkToggler_->applyInitialConfig(initialConfig_);
  }
}

void AgentEnsemble::writeConfig(const cfg::SwitchConfig& config) {
  auto* initializer = agentInitializer();
  auto agentConfig = initializer->platform()->config()->thrift;
  agentConfig.sw() = config;
  writeConfig(agentConfig);
}

void AgentEnsemble::writeConfig(const cfg::AgentConfig& agentConfig) {
  auto* initializer = agentInitializer();
  auto testConfigDir =
      initializer->platform()->getPersistentStateDir() + "/agent_ensemble/";
  utilCreateDir(testConfigDir);
  auto fileName = testConfigDir + configFile_;
  writeConfig(agentConfig, fileName);
}

void AgentEnsemble::writeConfig(
    const cfg::AgentConfig& agentConfig,
    const std::string& fileName) {
  auto newAgentConfig = AgentConfig(
      agentConfig,
      apache::thrift::SimpleJSONSerializer::serialize<std::string>(
          agentConfig));
  newAgentConfig.dumpConfig(fileName);
  if (kInputConfigFile.empty()) {
    // saving the original config file.
    kInputConfigFile = FLAGS_config;
  }
  FLAGS_config = fileName;
  initFlagDefaults(*newAgentConfig.thrift.defaultCommandLineArgs());
}

AgentEnsemble::~AgentEnsemble() {
  auto* initializer = agentInitializer();
  initializer->stopAgent(false);
  asyncInitThread_->join();
  asyncInitThread_.reset();
}

void AgentEnsemble::applyNewConfig(
    const cfg::SwitchConfig& config,
    bool activate) {
  writeConfig(config);
  if (activate) {
    getSw()->applyConfig("applying new config", config);
  }
}

std::vector<PortID> AgentEnsemble::masterLogicalPortIds() const {
  return masterLogicalPortIds_;
}

void AgentEnsemble::programRoutes(
    const RouterID& rid,
    const ClientID& client,
    const utility::RouteDistributionGenerator::ThriftRouteChunks& routeChunks) {
  auto updater = getSw()->getRouteUpdater();
  for (const auto& routeChunk : routeChunks) {
    std::for_each(
        routeChunk.begin(),
        routeChunk.end(),
        [&updater, client, rid](const auto& route) {
          updater.addRoute(rid, client, route);
        });
    updater.program();
  }
}

void AgentEnsemble::unprogramRoutes(
    const RouterID& rid,
    const ClientID& client,
    const utility::RouteDistributionGenerator::ThriftRouteChunks& routeChunks) {
  auto updater = getSw()->getRouteUpdater();
  for (const auto& routeChunk : routeChunks) {
    std::for_each(
        routeChunk.begin(),
        routeChunk.end(),
        [&updater, client, rid](const auto& route) {
          updater.delRoute(rid, *route.dest(), client);
        });
    updater.program();
  }
}

void AgentEnsemble::gracefulExit() {
  auto* initializer = agentInitializer();
  // exit for warm boot
  initializer->stopAgent(true);
}

std::shared_ptr<SwitchState> AgentEnsemble::applyNewState(
    std::shared_ptr<SwitchState> state,
    bool transaction) {
  if (!state) {
    return getSw()->getState();
  }
  state = EncapIndexAllocator::updateEncapIndices(
      StateDelta(getProgrammedState(), state), *getPlatform()->getAsic());
  transaction
      ? getSw()->updateStateWithHwFailureProtection(
            "apply new state with failure protection",
            [state](const std::shared_ptr<SwitchState>&) { return state; })
      : getSw()->updateStateBlocking(
            "apply new state",
            [state](const std::shared_ptr<SwitchState>&) { return state; });
  return getSw()->getState();
}

void AgentEnsemble::enableExactMatch(bcm::BcmConfig& config) {
  if (auto yamlCfg = config.yamlConfig()) {
    // use common func
    facebook::fboss::enableExactMatch(*yamlCfg);
  } else {
    auto& cfg = *(config.config());
    cfg["fpem_mem_entries"] = "0x10000";
  }
}

void AgentEnsemble::setupLinkStateToggler() {
  if (linkToggler_) {
    return;
  }
  const std::map<cfg::PortType, cfg::PortLoopbackMode> kLoopbackMode = {
      {cfg::PortType::INTERFACE_PORT, mode_}};
  linkToggler_ = createHwLinkStateToggler(this, kLoopbackMode);
}

std::string AgentEnsemble::getInputConfigFile() {
  if (kInputConfigFile.empty()) {
    return FLAGS_config;
  }
  return kInputConfigFile;
}

void ensembleMain(int argc, char* argv[], PlatformInitFn initPlatform) {
  kArgc = argc;
  kArgv = argv;
  kPlatformInitFn = std::move(initPlatform);
}

std::unique_ptr<AgentEnsemble> createAgentEnsemble(
    AgentEnsembleSwitchConfigFn initialConfigFn,
    AgentEnsemblePlatformConfigFn platformConfigFn,
    uint32_t featuresDesired,
    bool startAgent) {
  auto ensemble = std::make_unique<AgentEnsemble>();
  ensemble->setupEnsemble(
      kArgc,
      kArgv,
      featuresDesired,
      kPlatformInitFn,
      initialConfigFn,
      platformConfigFn);
  if (featuresDesired & HwSwitch::FeaturesDesired::LINKSCAN_DESIRED) {
    ensemble->setupLinkStateToggler();
  }
  if (startAgent) {
    ensemble->startAgent();
  }
  return ensemble;
}

std::map<PortID, HwPortStats> AgentEnsemble::getLatestPortStats(
    const std::vector<PortID>& ports) {
  std::map<PortID, HwPortStats> portIdStatsMap;
  SwitchStats dummy{};
  getHw()->updateStats(&dummy);

  auto swState = getSw()->getState();
  auto stats = getHw()->getPortStats();
  for (auto [portName, stats] : stats) {
    auto portId = swState->getPorts()->getPort(portName)->getID();
    if (std::find(ports.begin(), ports.end(), (PortID)portId) == ports.end()) {
      continue;
    }
    portIdStatsMap.emplace((PortID)portId, stats);
  }
  return portIdStatsMap;
}

HwPortStats AgentEnsemble::getLatestPortStats(const PortID& port) {
  return getLatestPortStats(std::vector<PortID>{port})[port];
}

void AgentEnsemble::registerStateObserver(
    StateObserver* observer,
    const std::string& name) {
  getSw()->registerStateObserver(observer, name);
}

void AgentEnsemble::unregisterStateObserver(StateObserver* observer) {
  getSw()->unregisterStateObserver(observer);
}
} // namespace facebook::fboss
