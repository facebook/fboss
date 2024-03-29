// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/test/AgentEnsemble.h"

#include "fboss/agent/AgentConfig.h"
#include "fboss/agent/SwitchStats.h"
#include "fboss/agent/Utils.h"

#include <folly/io/async/ScopedEventBaseThread.h>
#include "fboss/agent/CommonInit.h"
#include "fboss/agent/EncapIndexAllocator.h"
#include "fboss/agent/TxPacket.h"
#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/lib/CommonUtils.h"
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
      initializer->sw()->getDirUtil()->agentEnsembleConfigDir();
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
  bool gracefulExit = !::testing::Test::HasFailure();
  initializer->stopAgent(
      true /* setupWarmboot */, gracefulExit /* gracefulExit */);
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
  return getSw()->getHwPortStats(ports);
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

std::map<PortID, FabricEndpoint> AgentEnsemble::getFabricConnectivity(
    SwitchID switchId) const {
  if (FLAGS_multi_switch) {
    std::map<PortID, FabricEndpoint> connectivity;
    auto gotConnectivity =
        getSw()->getHwSwitchThriftClientTable()->getFabricConnectivity(
            switchId);
    CHECK(gotConnectivity.has_value());
    for (auto [portId, fabricEndpoint] : gotConnectivity.value()) {
      connectivity.insert({PortID(portId), fabricEndpoint});
    }
    return connectivity;
  } else {
    return getSw()->getHwSwitchHandler()->getFabricConnectivity();
  }
}

void AgentEnsemble::runDiagCommand(
    const std::string& input,
    std::string& output,
    std::optional<SwitchID> switchId) {
  ClientInformation clientInfo;
  clientInfo.username() = "agent_ensemble";
  clientInfo.hostname() = "agent_ensemble";
  if (FLAGS_multi_switch) {
    CHECK(switchId.has_value());
    output = getSw()->getHwSwitchThriftClientTable()->diagCmd(
        switchId.value(), input, clientInfo);
  } else {
    auto client = createFbossHwClient(
        5909, std::make_shared<folly::ScopedEventBaseThread>());
    fbstring out;
    client->sync_diagCmd(
        out,
        input,
        clientInfo,
        0 /* serverTimeoutMsecs */,
        false /* bypassFilter */);
    output = out;
  }
}

LinkStateToggler* AgentEnsemble::getLinkToggler() {
  return linkToggler_.get();
}

/*
 * Wait for traffic on port to reach specified rate. If the
 * specified rate is reached, return true, else false.
 */
bool AgentEnsemble::waitForRateOnPort(
    PortID port,
    const uint64_t desiredBps,
    int secondsToWaitPerIteration) {
  // Need to wait for atleast one second
  if (secondsToWaitPerIteration < 1) {
    secondsToWaitPerIteration = 1;
    XLOG(WARNING) << "Setting wait time to 1 second for tests!";
  }

  const auto portSpeedBps =
      static_cast<uint64_t>(
          getProgrammedState()->getPorts()->getNodeIf(port)->getSpeed()) *
      1000 * 1000;
  if (desiredBps > portSpeedBps) {
    // Cannot achieve higher than line rate
    XLOG(ERR) << "Desired rate " << desiredBps << " bps is > port rate "
              << portSpeedBps << " bps!!";
    return false;
  }

  WITH_RETRIES_N_TIMED(
      10, std::chrono::milliseconds(1000 * secondsToWaitPerIteration), {
        const auto prevPortStats = getLatestPortStats(port);
        auto prevPortBytes = *prevPortStats.outBytes_();
        auto prevPortPackets =
            (*prevPortStats.outUnicastPkts_() +
             *prevPortStats.outMulticastPkts_() +
             *prevPortStats.outBroadcastPkts_());

        const auto curPortStats = getLatestPortStats(port);
        auto curPortPackets =
            (*curPortStats.outUnicastPkts_() +
             *curPortStats.outMulticastPkts_() +
             *curPortStats.outBroadcastPkts_());

        // 20 bytes are consumed by ethernet preamble, start of frame and
        // interpacket gap. Account for that in linerate.
        auto packetPaddingBytes = (curPortPackets - prevPortPackets) * 20;
        auto curPortBytes = *curPortStats.outBytes_() + packetPaddingBytes;
        auto rate = static_cast<uint64_t>((curPortBytes - prevPortBytes) * 8) /
            secondsToWaitPerIteration;
        XLOG(DBG2) << ": Current rate " << rate << " bps < expected rate "
                   << desiredBps << " bps. curPortBytes " << curPortBytes
                   << " prevPortBytes " << prevPortBytes << " curPortPackets "
                   << curPortPackets << " prevPortPackets " << prevPortPackets;
        EXPECT_EVENTUALLY_TRUE(rate >= desiredBps);
        return true;
      });
  return false;
}

void AgentEnsemble::waitForLineRateOnPort(PortID port) {
  const auto portSpeedBps =
      static_cast<uint64_t>(
          getProgrammedState()->getPorts()->getNodeIf(port)->getSpeed()) *
      1000 * 1000;
  if (waitForRateOnPort(port, portSpeedBps)) {
    // Traffic on port reached line rate!
    return;
  }
  throw FbossError("Line rate was never reached");
}

void AgentEnsemble::waitForSpecificRateOnPort(
    PortID port,
    const uint64_t desiredBps,
    int secondsToWaitPerIteration) {
  if (waitForRateOnPort(port, desiredBps, secondsToWaitPerIteration)) {
    // Traffic on port reached desired rate!
    return;
  }

  throw FbossError("Desired rate ", desiredBps, " bps was never reached");
}

void AgentEnsemble::sendPacketAsync(
    std::unique_ptr<TxPacket> pkt,
    std::optional<PortDescriptor> portDescriptor,
    std::optional<uint8_t> queueId) {
  if (!portDescriptor.has_value()) {
    getSw()->sendPacketSwitchedAsync(std::move(pkt));
    return;
  }
  getSw()->sendPacketOutOfPortAsync(
      std::move(pkt), portDescriptor->phyPortID(), queueId);
}

std::unique_ptr<TxPacket> AgentEnsemble::allocatePacket(uint32_t size) {
  return getSw()->allocatePacket(size);
}
} // namespace facebook::fboss
