// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/test/AgentEnsemble.h"

#include <chrono>
#include <map>
#include <vector>

#include "fboss/agent/AgentConfig.h"
#include "fboss/agent/AgentFeatures.h"
#include "fboss/agent/CommonInit.h"
#include "fboss/agent/EncapIndexAllocator.h"
#include "fboss/agent/SwSwitch.h"
#include "fboss/agent/ThriftHandler.h"
#include "fboss/agent/TxPacket.h"
#include "fboss/agent/Utils.h"
#include "fboss/agent/hw/gen-cpp2/hardware_stats_constants.h"
#include "fboss/agent/hw/gen-cpp2/hardware_stats_types.h"
#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/test/utils/PacketSendUtils.h"
#include "fboss/agent/types.h"
#include "fboss/lib/CommonFileUtils.h"
#include "fboss/lib/CommonUtils.h"
#include "fboss/lib/config/PlatformConfigUtils.h"

#include <folly/io/async/EventBase.h>
#include <folly/io/async/ScopedEventBaseThread.h>
#include <folly/testing/TestUtil.h>
#include <thrift/lib/cpp2/async/PooledRequestChannel.h>
#include <thrift/lib/cpp2/async/ReconnectingRequestChannel.h>
#include <thrift/lib/cpp2/async/RetryingRequestChannel.h>
#include <thrift/lib/cpp2/async/RocketClientChannel.h>
#ifndef IS_OSS
#include "common/thrift/thrift/gen-cpp2/MonitorAsyncClient.h"
#endif
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
constexpr auto kConfig = "config";
constexpr auto kMultiSwitch = "multi_switch";
constexpr auto kOverriddenAgentConfigFile = "overridden_agent.conf";
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

  if (bootType_ == BootType::COLD_BOOT || FLAGS_prod_invariant_config_test) {
    auto inputAgentConfig =
        AgentConfig::fromFile(AgentEnsemble::getInputConfigFile())->thrift;
    if (platformConfigFn) {
      platformConfigFn(
          *(inputAgentConfig.sw()), *(inputAgentConfig.platform()));
    }
    // some platform config may need cold boots. so overwrite the config before
    // creating a switch
    writeConfig(inputAgentConfig, configFile_);
  }

  auto agentConf = AgentConfig::fromFile(configFile_);

  overrideConfigFlag(configFile_);
  createSwitch(std::move(agentConf), hwFeaturesDesired, kPlatformInitFn);

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
    if (*platformPorts.find(static_cast<int32_t>(port.first))
                ->second.mapping()
                ->portType() == cfg::PortType::FABRIC_PORT &&
        FLAGS_hide_fabric_ports) {
      continue;
    }
    if (*platformPorts.find(static_cast<int32_t>(port.first))
                ->second.mapping()
                ->portType() == cfg::PortType::MANAGEMENT_PORT &&
        FLAGS_hide_management_ports) {
      continue;
    }
    masterLogicalPortIds_.push_back(port.first);
    auto switchId = getSw()->getScopeResolver()->scope(port.first).switchId();
    switchId2PortIds_[switchId].push_back(port.first);
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

  // during config invariant test, we want to use the input config file during
  // warm boot
  if (bootType_ == BootType::COLD_BOOT || FLAGS_prod_invariant_config_test) {
    initialConfig_ = initialConfigFn(*this);
    applyInitialConfig(initialConfig_);
    // reload the new config
    reloadPlatformConfig();
  } else {
    initialConfig_ = *(AgentConfig::fromFile(configFile_)->thrift.sw());
  }

  createAndDumpOverriddenAgentConfig();

  // Setup LinkStateToggler and start agent
  if (hwFeaturesDesired & HwSwitch::FeaturesDesired::LINKSCAN_DESIRED &&
      disableLinkStateToggler == false) {
    setupLinkStateToggler();
  }
  startAgent(failHwCallsOnWarmboot);

  for (const auto& switchId : getSw()->getSwitchInfoTable().getL3SwitchIDs()) {
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
  } else {
    if (FLAGS_prod_invariant_config_test) {
      // During warmboot, the ports are already up.
      applyNewConfig(initialConfig_);
    }
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

void AgentEnsemble::stopStatsThread() {
  agentInitializer()->stopStatsThread();
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
  // Stats collection from SwSwitch is async, wait for stats
  // being available before returning here.
  std::map<PortID, HwPortStats> portIdStatsMap;
  checkWithRetry(
      [&portIdStatsMap, &ports, this]() {
        portIdStatsMap = getSw()->getHwPortStats(ports);
        // Check collect timestamp is valid
        for (const auto& [_, portStats] : portIdStatsMap) {
          if (*portStats.timestamp_() ==
              hardware_stats_constants::STAT_UNINITIALIZED()) {
            return false;
          }
        }
        return !portIdStatsMap.empty();
      },
      120,
      std::chrono::milliseconds(1000),
      " fetch port stats");
  return portIdStatsMap;
}

std::map<InterfaceID, HwRouterInterfaceStats>
AgentEnsemble::getLatestInterfaceStats(
    const std::vector<InterfaceID>& interfaces) {
  std::map<InterfaceID, HwRouterInterfaceStats> intfIdStatsMap;
  checkWithRetry(
      [&intfIdStatsMap, &interfaces, this]() {
        intfIdStatsMap = getSw()->getHwRouterInterfaceStats(interfaces);
        return !intfIdStatsMap.empty();
      },
      120,
      std::chrono::milliseconds(1000),
      " fetch interface stats");
  return intfIdStatsMap;
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
  std::map<PortID, FabricEndpoint> connectivity;
  auto gotConnectivity =
      getSw()->getHwSwitchThriftClientTable()->getFabricConnectivity(switchId);
  CHECK(gotConnectivity.has_value());

  for (const auto& [portName, fabricEndpoint] : gotConnectivity.value()) {
    auto portID = getSw()->getPlatformMapping()->getPortID(portName);

    connectivity.insert({portID, fabricEndpoint});
  }
  return connectivity;
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

void AgentEnsemble::runCint(
    const std::string& cintData,
    std::string& output,
    const SwitchID& switchId) {
  folly::test::TemporaryFile file;
  folly::writeFull(file.fd(), cintData.c_str(), cintData.size());
  auto cmd = folly::sformat("cint {}\n", file.path().c_str());
  runDiagCommand(cmd, output, switchId);
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

/**
 * Creates an overridden AgentConfig object by incorporating the overridden
 * initial configuration  and command line args, with the platform config from
 * the test configuration in configerator. This config is dumped for hw-agents
 * and for some warmboot tests.
 */
void AgentEnsemble::createAndDumpOverriddenAgentConfig() {
  CHECK(initialConfig_ != cfg::SwitchConfig());
  auto testConfig = AgentConfig::fromFile(configFile_);

  // Create base agent config with command line args
  std::map<std::string, std::string> defaultCommandLineArgs;
  std::vector<gflags::CommandLineFlagInfo> flags;
  gflags::GetAllFlags(&flags);
  for (const auto& flag : flags) {
    // Skip writing flags if 1) default value, and 2) config itself.
    if (!flag.is_default && flag.name != kConfig) {
      defaultCommandLineArgs.emplace(flag.name, flag.current_value);
    }
  }

  // Build the new agent config
  cfg::AgentConfig newAgentConf;
  newAgentConf.defaultCommandLineArgs() = defaultCommandLineArgs;
  newAgentConf.sw() = initialConfig_;
  newAgentConf.platform() = *testConfig->thrift.platform();

  auto agentConfig = AgentConfig(newAgentConf);

  // Create directory and dump ensemble config
  utilCreateDir(AgentDirectoryUtil().agentEnsembleConfigDir());
  agentConfig.dumpConfig(
      AgentDirectoryUtil().agentEnsembleConfigDir() +
      kOverriddenAgentConfigFile);

  // Handle hardware agent config for multi-switch setups
  if (FLAGS_multi_switch ||
      folly::get_default(defaultCommandLineArgs, kMultiSwitch, "") == "true") {
    for (const auto& [_, switchInfo] :
         *newAgentConf.sw()->switchSettings()->switchIdToSwitchInfo()) {
      agentConfig.dumpConfig(
          AgentDirectoryUtil().getTestHwAgentConfigFile(
              *switchInfo.switchIndex()));
    }
  }
}

std::optional<VlanID> AgentEnsemble::getVlanIDForTx() const {
  auto intf = utility::firstInterfaceWithPorts(getProgrammedState());
  return getSw()->getVlanIDForTx(intf);
}

std::vector<FirmwareInfo> AgentEnsemble::getAllFirmwareInfo(
    SwitchID switchId) const {
  return getSw()->getHwSwitchThriftClientTable()->getAllFirmwareInfo(switchId);
}

/**
 * Retrieves monitoring counters that match a given regex pattern for a specific
 * port.
 *
 * @details
 * Works in both mono-switch and multi-switch environments.
 *
 * @param portId The ID of the port for which to retrieve counters.
 * @param regex The regex pattern to match against the counter names.
 *
 * @return A map of counter names to their respective values that match the
 * regex pattern.
 */
std::map<std::string, int64_t> AgentEnsemble::getFb303CountersByRegex(
    const PortID& portId,
    const std::string& regex) {
  std::map<std::string, int64_t> counters;
#ifndef IS_OSS
  auto switchID = scopeResolver().scope(portId).switchId();
  auto client = getSw()->getHwSwitchThriftClientTable()->getClient(switchID);
  apache::thrift::Client<facebook::thrift::Monitor> monitoringClient{
      client->getChannelShared()};
  monitoringClient.sync_getRegexCounters(counters, regex);
#else
  // TODO: This needs to be updated to support multi-switch.
  counters = facebook::fb303::fbData->getRegexCounters(regex);
#endif
  return counters;
}

/**
 * Retrieves the value of a specific fb303 counter for a given switch.
 *
 * @details
 * Works in both mono-switch and multi-switch environments.
 *
 * @param key The name of the counter to retrieve.
 * @param switchID The ID of the switch for which to retrieve the counter.
 *
 * @return The value of the specified counter.
 */
int64_t AgentEnsemble::getFb303Counter(
    const std::string& key,
    const SwitchID& switchID) {
  int64_t counter{0};
#ifndef IS_OSS
  auto client = getSw()->getHwSwitchThriftClientTable()->getClient(switchID);
  apache::thrift::Client<facebook::thrift::Monitor> monitoringClient{
      client->getChannelShared()};
  counter = monitoringClient.sync_getCounter(key);
#else
  // TODO: This needs to be updated to support multi-switch.
  counter = facebook::fb303::fbData->getCounter(key);
#endif
  return counter;
}

/**
 * Retrieves the value of the first counter that matches a given regex pattern
 * for a specific port.
 *
 * @details
 * Works in both mono-switch and multi-switch environments.
 *
 * @param portId The ID of the port for which to retrieve the counter.
 * @param regex The regex pattern to match against counter names.
 *
 * @return The value of the first matching counter if one exists, otherwise
 * nullopt.
 */
std::optional<int64_t> AgentEnsemble::getFb303CounterIfExists(
    const PortID& portId,
    const std::string& regex) {
  auto counters = getFb303CountersByRegex(portId, regex);
  if (!counters.empty()) {
    return counters.begin()->second;
  }
  return std::nullopt;
}

/**
 * Retrieves monitoring counters that match a given regex pattern for a specific
 * switch.
 *
 * @details
 * Works in both mono-switch and multi-switch environments.
 * @param regex The regex pattern to match against the counter names.
 * @param switchID The ID of the switch for which to retrieve counters.
 *
 * @return A map of counter names to their respective values that match the
 * regex pattern.
 */
std::map<std::string, int64_t> AgentEnsemble::getFb303RegexCounters(
    const std::string& regex,
    const SwitchID& switchID) {
  std::map<std::string, int64_t> counters;
#ifndef IS_OSS
  auto client = getSw()->getHwSwitchThriftClientTable()->getClient(switchID);
  apache::thrift::Client<facebook::thrift::Monitor> monitoringClient{
      client->getChannelShared()};
  monitoringClient.sync_getRegexCounters(counters, regex);
#else
  // TODO: This needs to be updated to support multi-switch.
  counters = facebook::fb303::fbData->getRegexCounters(regex);
#endif
  return counters;
}

std::string AgentEnsemble::getHwDebugDump() {
  std::string out{};
  ThriftHandler(getSw()).getHwDebugDump(out);
  return out;
}

cfg::SwitchingMode AgentEnsemble::getFwdSwitchingMode(
    const RoutePrefixV6& prefix) {
  auto resolvedRoute = findRoute<folly::IPAddressV6>(
      RouterID(0), {prefix.network(), prefix.mask()}, getProgrammedState());
  return getFwdSwitchingMode(resolvedRoute->getForwardInfo());
}

} // namespace facebook::fboss
