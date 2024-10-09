// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/test/AgentEnsemble.h"

#include "fboss/agent/AgentConfig.h"
#include "fboss/agent/SwitchStats.h"
#include "fboss/agent/Utils.h"

#include "fboss/agent/CommonInit.h"
#include "fboss/agent/EncapIndexAllocator.h"
#include "fboss/agent/ThriftHandler.h"
#include "fboss/agent/TxPacket.h"
#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/test/utils/PacketSendUtils.h"
#include "fboss/lib/CommonFileUtils.h"
#include "fboss/lib/CommonUtils.h"
#include "fboss/lib/config/PlatformConfigUtils.h"

#include <folly/io/async/EventBase.h>
#include <folly/io/async/ScopedEventBaseThread.h>
#include <thrift/lib/cpp2/async/PooledRequestChannel.h>
#include <thrift/lib/cpp2/async/ReconnectingRequestChannel.h>
#include <thrift/lib/cpp2/async/RetryingRequestChannel.h>
#include <thrift/lib/cpp2/async/RocketClientChannel.h>

#include <gtest/gtest.h>

DEFINE_bool(
    setup_for_warmboot,
    false,
    "Set up test for SDK warmboot. Useful for testing individual "
    "tests doing a full process warmboot and verifying expectations");

namespace {
facebook::fboss::PlatformInitFn kPlatformInitFn;
static std::string kInputConfigFile;
std::optional<facebook::fboss::cfg::StreamType> kStreamTypeOpt{std::nullopt};
static const int kMsWaitForStatsRetry = 2000;
} // namespace

namespace facebook::fboss {
AgentEnsemble::AgentEnsemble(const std::string& configFileName) {
  setConfigFiles(configFileName);
  setBootType();
}

void AgentEnsemble::setupEnsemble(
    AgentEnsembleSwitchConfigFn initialConfigFn,
    bool disableLinkStateToggler,
    AgentEnsemblePlatformConfigFn platformConfigFn,
    uint32_t hwFeaturesDesired,
    bool failHwCallsOnWarmboot) {
  FLAGS_verify_apply_oper_delta = true;

  if (bootType_ == BootType::COLD_BOOT) {
    auto agentConf =
        AgentConfig::fromFile(AgentEnsemble::getInputConfigFile())->thrift;
    if (platformConfigFn) {
      platformConfigFn(*(agentConf.platform()));
    }
    // some platform config may need cold boots. so overwrite the config before
    // creating a switch
    writeConfig(agentConf, configFile_);
  }
  overrideConfigFlag(configFile_);
  createSwitch(
      AgentConfig::fromFile(configFile_), hwFeaturesDesired, kPlatformInitFn);

  for (auto switchId : getSw()->getHwAsicTable()->getSwitchIDs()) {
    HwAsic* asic = getHwAsicTable()->getHwAsicIf(switchId);
    if (kStreamTypeOpt.has_value()) {
      asic->setDefaultStreamType(kStreamTypeOpt.value());
    }
    switchId2PortIds_[switchId] = std::vector<PortID>();
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
      auto switchId = getSw()->getScopeResolver()->scope(port.first).switchId();
      switchId2PortIds_[switchId].push_back(port.first);
    }
  }

  for (auto switchId : getSw()->getHwAsicTable()->getSwitchIDs()) {
    HwAsic* asic = getHwAsicTable()->getHwAsicIf(switchId);
    utility::setPortToDefaultProfileIDMap(
        std::make_shared<MultiSwitchPortMap>(), /* unused */
        getSw()->getPlatformMapping(),
        asic,
        getSw()->getPlatformSupportsAddRemovePort(),
        switchId2PortIds_[switchId]);
  }

  if (bootType_ == BootType::COLD_BOOT) {
    initialConfig_ = initialConfigFn(*this);
    applyInitialConfig(initialConfig_);
    // reload the new config
    reloadPlatformConfig();
  } else {
    initialConfig_ = *(AgentConfig::fromFile(configFile_)->thrift.sw());
  }

  // Setup LinkStateToggler and start agent
  if (hwFeaturesDesired & HwSwitch::FeaturesDesired::LINKSCAN_DESIRED &&
      disableLinkStateToggler == false) {
    setupLinkStateToggler();
  }
  startAgent(failHwCallsOnWarmboot);

  for (const auto& switchId : getSw()->getSwitchInfoTable().getSwitchIDs()) {
    ensureHwSwitchConnected(switchId);
  }
}

void AgentEnsemble::startAgent(bool failHwCallsOnWarmboot) {
  auto* initializer = agentInitializer();
  auto hwWriteBehavior = HwWriteBehavior::WRITE;
  if (getSw()->getWarmBootHelper()->canWarmBoot()) {
    hwWriteBehavior = HwWriteBehavior::LOG_FAIL;
    if (getSw()->getHwAsicTable()->isFeatureSupportedOnAllAsic(
            HwAsic::Feature::ZERO_SDK_WRITE_WARMBOOT)) {
      if (failHwCallsOnWarmboot) {
        hwWriteBehavior = HwWriteBehavior::FAIL;
      } else {
        // If failHwCallsOnWarmboot = false, skip logging as the write is
        // expected
        hwWriteBehavior = HwWriteBehavior::WRITE;
      }
    }
  }
  asyncInitThread_.reset(new std::thread([this, initializer, hwWriteBehavior] {
    // hardware switch events will be dispatched to agent ensemble
    // agent ensemble is responsible to dispatch them to SwSwitch
    initializer->initAgent(this, hwWriteBehavior);
  }));
  initializer->initializer()->waitForInitDone();

  if (getSw()->getBootType() == BootType::COLD_BOOT) {
    if (linkToggler_ != nullptr) {
      linkToggler_->applyInitialConfig(initialConfig_);
    }
    // With link state toggler, initial config is applied with ports down to
    // later bring up the ports. This causes the config to be written as
    // loopback mode = None. Write the init config again to have the proper
    // config.
    applyNewConfig(initialConfig_);
  }
}

void AgentEnsemble::writeConfig(const cfg::SwitchConfig& config) {
  auto* initializer = agentInitializer();
  auto isSwConfigured =
      initializer->sw() && initializer->sw()->isFullyConfigured();
  auto agentConfig = isSwConfigured
      ? initializer->sw()->getAgentConfig()
      : AgentConfig::fromFile(configFile_)->thrift;

  agentConfig.sw() = config;
  // Inherit SDK version from previous config
  auto inputConfig = AgentConfig::fromFile(FLAGS_config)->thrift;
  if (inputConfig.sw()->sdkVersion().has_value()) {
    agentConfig.sw()->sdkVersion() = inputConfig.sw()->sdkVersion().value();
  }
  writeConfig(agentConfig);
}

void AgentEnsemble::writeConfig(const cfg::AgentConfig& agentConfig) {
  writeConfig(agentConfig, configFile_);
}

void AgentEnsemble::writeConfig(
    const cfg::AgentConfig& agentConfig,
    const std::string& fileName) {
  auto newAgentConfig = AgentConfig(
      agentConfig,
      apache::thrift::SimpleJSONSerializer::serialize<std::string>(
          agentConfig));
  newAgentConfig.dumpConfig(fileName);
}

void AgentEnsemble::overrideConfigFlag(const std::string& fileName) {
  FLAGS_config = fileName;
  initFlagDefaults(
      *(AgentConfig::fromFile(fileName)->thrift.defaultCommandLineArgs()));
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
    kInputConfigFile = FLAGS_config;
  }
  return kInputConfigFile;
}

void AgentEnsemble::setConfigFiles(const std::string& fileName) {
  if (kInputConfigFile.empty()) {
    kInputConfigFile = FLAGS_config;
  }
  utilCreateDir(AgentDirectoryUtil().agentEnsembleConfigDir());
  configFile_ = AgentDirectoryUtil().agentEnsembleConfigDir() + fileName;
}

void AgentEnsemble::setBootType() {
  auto dirUtil = AgentDirectoryUtil();
  bootType_ = (checkFileExists(dirUtil.getSwSwitchCanWarmBootFile()))
      ? BootType::WARM_BOOT
      : BootType::COLD_BOOT;
}

BootType AgentEnsemble::getBootType() const {
  return bootType_;
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

std::map<SystemPortID, HwSysPortStats> AgentEnsemble::getLatestSysPortStats(
    const std::vector<SystemPortID>& ports) {
  return getSw()->getHwSysPortStats(ports);
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

uint64_t AgentEnsemble::getTrafficRate(
    const HwPortStats& prevPortStats,
    const HwPortStats& curPortStats,
    const int secondsBetweenStatsCollection) {
  auto prevPortBytes = *prevPortStats.outBytes_();
  auto prevPortPackets =
      (*prevPortStats.outUnicastPkts_() + *prevPortStats.outMulticastPkts_() +
       *prevPortStats.outBroadcastPkts_());

  auto curPortPackets =
      (*curPortStats.outUnicastPkts_() + *curPortStats.outMulticastPkts_() +
       *curPortStats.outBroadcastPkts_());

  // 20 bytes are consumed by ethernet preamble, start of frame and
  // interpacket gap. Account for that in linerate.
  auto packetPaddingBytes = (curPortPackets - prevPortPackets) * 20;
  auto curPortBytes = *curPortStats.outBytes_() + packetPaddingBytes;
  auto rate = static_cast<uint64_t>((curPortBytes - prevPortBytes) * 8) /
      secondsBetweenStatsCollection;
  XLOG(DBG2) << "Current rate " << rate << " bps" << ", curPortBytes "
             << curPortBytes << " prevPortBytes " << prevPortBytes
             << " curPortPackets " << curPortPackets << " prevPortPackets "
             << prevPortPackets;
  return rate;
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

  // The first iteration in the below loop will not be successful
  // given the prev/curr stats collections are back to back!
  auto prevPortStats = getLatestPortStats(port);
  XLOG(DBG0) << "PortID: " << port << ", Desired rate " << desiredBps;
  bool metDesiredRate = false;
  WITH_RETRIES_N_TIMED(
      10, std::chrono::milliseconds(1000 * secondsToWaitPerIteration), {
        auto curPortStats = getLatestPortStats(port);
        auto rate = getTrafficRate(
            prevPortStats, curPortStats, secondsToWaitPerIteration);
        // Update prev stats for the next iteration if needed!
        prevPortStats = curPortStats;
        if (desiredBps == 0) {
          metDesiredRate = rate == desiredBps;
        } else {
          metDesiredRate = rate >= desiredBps;
        }
        EXPECT_EVENTUALLY_TRUE(metDesiredRate);
      });
  return metDesiredRate;
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

void AgentEnsemble::bringUpPorts(const std::vector<PortID>& ports) {
  CHECK(linkToggler_);
  linkToggler_->bringUpPorts(ports);
}

void AgentEnsemble::bringDownPorts(const std::vector<PortID>& ports) {
  CHECK(linkToggler_);
  linkToggler_->bringDownPorts(ports);
}

std::vector<PortID> AgentEnsemble::masterLogicalPortIds(
    SwitchID switchId) const {
  return switchId2PortIds_.at(switchId);
}

void AgentEnsemble::clearPortStats() {
  auto portsVec = std::make_unique<std::vector<int32_t>>();
  for (const auto& [key, ports] :
       std::as_const(*getProgrammedState()->getPorts())) {
    for (const auto& [id, port] : std::as_const(*ports)) {
      portsVec->push_back(id);
    }
  }
  clearPortStats(std::move(portsVec));
}

void AgentEnsemble::clearPortStats(
    const std::unique_ptr<std::vector<int32_t>>& ports) {
  ThriftHandler(getSw()).clearPortStats(
      std::make_unique<std::vector<int32_t>>(std::move(*ports)));
}

bool AgentEnsemble::ensureSendPacketSwitched(std::unique_ptr<TxPacket> pkt) {
  // lambda that returns HwPortStats for the given port(s)
  auto getPortStats =
      [&](const std::vector<PortID>& portIds) -> std::map<PortID, HwPortStats> {
    std::map<PortID, HwPortStats> portStats;
    WITH_RETRIES({
      portStats = getLatestPortStats(portIds);
      EXPECT_EVENTUALLY_TRUE(portStats.size());
    });
    return portStats;
  };
  auto getSysPortStats = [&](const std::vector<SystemPortID>& portIds)
      -> std::map<SystemPortID, HwSysPortStats> {
    return getLatestSysPortStats(portIds);
  };

  return utility::ensureSendPacketSwitched(
      this,
      std::move(pkt),
      masterLogicalPortIds({cfg::PortType::INTERFACE_PORT}),
      getPortStats,
      masterLogicalSysPortIds(),
      getSysPortStats,
      kMsWaitForStatsRetry);
}

bool AgentEnsemble::ensureSendPacketOutOfPort(
    std::unique_ptr<TxPacket> pkt,
    PortID portID,
    std::optional<uint8_t> queue) {
  // lambda that returns HwPortStats for the given port(s)
  auto getPortStats =
      [&](const std::vector<PortID>& portIds) -> std::map<PortID, HwPortStats> {
    return getLatestPortStats(portIds);
  };
  return utility::ensureSendPacketOutOfPort(
      this,
      std::move(pkt),
      portID,
      masterLogicalPortIds({cfg::PortType::INTERFACE_PORT}),
      getPortStats,
      queue,
      kMsWaitForStatsRetry);
}

std::unique_ptr<apache::thrift::Client<utility::AgentHwTestCtrl>>
AgentEnsemble::getHwAgentTestClient(SwitchID switchId) {
  auto switchIndex =
      getSw()->getSwitchInfoTable().getSwitchIndexFromSwitchId(switchId);
  uint16_t port = FLAGS_hwagent_port_base + switchIndex;
  auto evbThread = std::make_shared<folly::ScopedEventBaseThread>();

  auto reconnectingChannel =
      apache::thrift::PooledRequestChannel::newSyncChannel(
          evbThread, [port, evbThread](folly::EventBase& evb) {
            return apache::thrift::RetryingRequestChannel::newChannel(
                evb,
                2, /*retries before error*/
                apache::thrift::ReconnectingRequestChannel::newChannel(
                    *evbThread->getEventBase(), [port](folly::EventBase& evb) {
                      auto socket = folly::AsyncSocket::UniquePtr(
                          new folly::AsyncSocket(&evb));
                      socket->connect(
                          nullptr, folly::SocketAddress("::1", port));
                      auto channel =
                          apache::thrift::RocketClientChannel::newChannel(
                              std::move(socket));
                      channel->setTimeout(FLAGS_hwswitch_query_timeout * 1000);
                      return channel;
                    }));
          });
  return std::make_unique<apache::thrift::Client<utility::AgentHwTestCtrl>>(
      std::move(reconnectingChannel));
}

} // namespace facebook::fboss
