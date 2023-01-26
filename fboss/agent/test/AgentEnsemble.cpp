// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/test/AgentEnsemble.h"

#include "fboss/agent/AgentConfig.h"
#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/lib/config/PlatformConfigUtils.h"

DECLARE_bool(tun_intf);
DEFINE_bool(
    setup_for_warmboot,
    false,
    "Set up test for SDK warmboot. Useful for testing individual "
    "tests doing a full process warmboot and verifying expectations");

namespace {
void initFlagDefaults(const std::map<std::string, std::string>& defaults) {
  for (auto item : defaults) {
    gflags::SetCommandLineOptionWithMode(
        item.first.c_str(), item.second.c_str(), gflags::SET_FLAGS_DEFAULT);
  }
}
} // namespace
namespace facebook::fboss {

void AgentEnsemble::setupEnsemble(
    int argc,
    char** argv,
    uint32_t hwFeaturesDesired,
    PlatformInitFn initPlatform,
    AgentEnsembleConfigFn initialConfig) {
  auto* initializer = agentInitializer();
  initializer->createSwitch(argc, argv, hwFeaturesDesired, initPlatform);

  utility::setPortToDefaultProfileIDMap(
      std::make_shared<PortMap>(), getPlatform());
  auto portsByControllingPort =
      utility::getSubsidiaryPortIDs(getPlatform()->getPlatformPorts());

  for (const auto& port : portsByControllingPort) {
    masterLogicalPortIds_.push_back(port.first);
  }
  initialConfig_ = initialConfig(getHw(), masterLogicalPortIds_);
  writeConfig(initialConfig_);
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
  asyncInitThread_.reset(
      new std::thread([initializer] { initializer->initAgent(); }));
  asyncInitThread_->detach();
  initializer->initializer()->waitForInitDone();
}

void AgentEnsemble::writeConfig(const cfg::SwitchConfig& config) {
  auto agentConfig = AgentConfig::fromFile(FLAGS_config)->thrift;
  agentConfig.sw() = config;
  auto newAgentConfig = AgentConfig(
      agentConfig,
      apache::thrift::SimpleJSONSerializer::serialize<std::string>(
          agentConfig));
  newAgentConfig.dumpConfig(FLAGS_config);

  initFlagDefaults(*newAgentConfig.thrift.defaultCommandLineArgs());
}

AgentEnsemble::~AgentEnsemble() {
  auto* initializer = agentInitializer();

  // if ensemble is leaked and released it would be exit for warmboot.
  // exit for cold boot.
  initializer->stopAgent(false);
}

void AgentEnsemble::applyNewConfig(cfg::SwitchConfig& config, bool activate) {
  writeConfig(config);
  if (activate) {
    getSw()->applyConfig("applying new config", config);
  }
}

const std::vector<PortID>& AgentEnsemble::masterLogicalPortIds() const {
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

} // namespace facebook::fboss
