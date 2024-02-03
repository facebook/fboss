// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/test/AgentEnsemble.h"

#include "fboss/agent/AgentConfig.h"
#include "fboss/agent/SwitchStats.h"
#include "fboss/agent/Utils.h"

#include "fboss/agent/CommonInit.h"
#include "fboss/agent/EncapIndexAllocator.h"
#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/lib/config/PlatformConfigUtils.h"

#include <gtest/gtest.h>

DECLARE_bool(tun_intf);
DEFINE_bool(
    setup_for_warmboot,
    false,
    "Set up test for SDK warmboot. Useful for testing individual "
    "tests doing a full process warmboot and verifying expectations");

namespace {
facebook::fboss::PlatformInitFn kPlatformInitFn;
static std::string kInputConfigFile;
std::optional<facebook::fboss::cfg::StreamType> kStreamTypeOpt{std::nullopt};
} // namespace

namespace facebook::fboss {
AgentEnsemble::AgentEnsemble(const std::string& configFileName) {
  configFile_ = configFileName;
}

void AgentEnsemble::setupEnsemble(
    uint32_t hwFeaturesDesired,
    AgentEnsembleSwitchConfigFn initialConfigFn,
    AgentEnsemblePlatformConfigFn platformConfigFn) {
  FLAGS_verify_apply_oper_delta = true;

  if (platformConfigFn) {
    auto agentConf =
        AgentConfig::fromFile(AgentEnsemble::getInputConfigFile())->thrift;
    platformConfigFn(*(agentConf.platform()));
    // some platform config may need cold boots. so overwrite the config before
    // creating a switch
    writeConfig(agentConf, FLAGS_config);
  }
  createSwitch(
      AgentConfig::fromDefaultFile(), hwFeaturesDesired, kPlatformInitFn);

  // TODO: Handle multiple Asics
  HwAsic* asic = getHwAsicTable()->getHwAsicIf(SwitchID(0));
  if (kStreamTypeOpt.has_value()) {
    asic->setDefaultStreamType(kStreamTypeOpt.value());
  }

  auto portsByControllingPort = utility::getSubsidiaryPortIDs(
      getSw()->getPlatformMapping()->getPlatformPorts());
  const auto& platformPorts = getSw()->getPlatformMapping()->getPlatformPorts();
  for (const auto& port : portsByControllingPort) {
    if (!FLAGS_hide_fabric_ports ||
        *platformPorts.find(static_cast<int32_t>(port.first))
                ->second.mapping()
                ->portType() != cfg::PortType::FABRIC_PORT) {
      masterLogicalPortIds_.push_back(port.first);
    }
  }
  utility::setPortToDefaultProfileIDMap(
      std::make_shared<MultiSwitchPortMap>(),
      getSw()->getPlatformMapping(),
      asic,
      getSw()->getPlatformSupportsAddRemovePort(),
      masterLogicalPortIds_);

  initialConfig_ = initialConfigFn(*this);
  applyInitialConfig(initialConfig_);
  // reload the new config
  reloadPlatformConfig();

  // Setup LinkStateToggler and start agent
  if (hwFeaturesDesired & HwSwitch::FeaturesDesired::LINKSCAN_DESIRED) {
    setupLinkStateToggler();
  }
  startAgent();
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
  if (getSw()->getBootType() != BootType::WARM_BOOT &&
      linkToggler_ != nullptr) {
    linkToggler_->applyInitialConfig(initialConfig_);
  }
}

void AgentEnsemble::writeConfig(const cfg::SwitchConfig& config) {
  auto* initializer = agentInitializer();
  auto agentConfig = initializer->sw()->getAgentConfig();
  agentConfig.sw() = config;
  writeConfig(agentConfig);
}

void AgentEnsemble::writeConfig(const cfg::AgentConfig& agentConfig) {
  auto* initializer = agentInitializer();
  auto testConfigDir =
      initializer->sw()->getDirUtil()->getPersistentStateDir() +
      "/agent_ensemble/";
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
  joinAsyncInitThread();
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

void AgentEnsemble::switchRunStateChanged(SwitchRunState runState) {}

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

void AgentEnsemble::applyNewState(
    StateUpdateFn fn,
    const std::string& name,
    bool transaction) {
  // TODO: Handle multiple Asics
  auto asic = getSw()->getHwAsicTable()->getHwAsics().cbegin()->second;
  CHECK(asic);
  auto applyUpdate = [&](const std::shared_ptr<SwitchState>& in) {
    auto newState = fn(in);
    if (!newState) {
      return newState;
    }
    return EncapIndexAllocator::updateEncapIndices(
        StateDelta(in, newState), *asic);
  };
  transaction ? getSw()->updateStateWithHwFailureProtection(name, applyUpdate)
              : getSw()->updateStateBlocking(name, applyUpdate);
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
  linkToggler_ = std::make_unique<LinkStateToggler>(this);
}

std::string AgentEnsemble::getInputConfigFile() {
  if (kInputConfigFile.empty()) {
    return FLAGS_config;
  }
  return kInputConfigFile;
}

void initEnsemble(
    PlatformInitFn initPlatform,
    std::optional<cfg::StreamType> streamType) {
  kPlatformInitFn = std::move(initPlatform);
  kStreamTypeOpt = streamType;
}

std::map<PortID, HwPortStats> AgentEnsemble::getLatestPortStats(
    const std::vector<PortID>& ports) {
  std::map<PortID, HwPortStats> portIdStatsMap;
  getSw()->updateStats();

  auto swState = getSw()->getState();
  auto stats = getSw()->getHwSwitchHandler()->getPortStats();
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
