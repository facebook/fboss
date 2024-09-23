// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include <fboss/agent/gen-cpp2/agent_config_types.h>
#include <fboss/agent/gen-cpp2/platform_config_types.h>
#include <fboss/agent/gen-cpp2/switch_config_types.h>
#include <fboss/agent/hw/bcm/gen-cpp2/bcm_config_types.h>
#include <unistd.h>
#include <functional>
#include "fboss/agent/L2Entry.h"
#include "fboss/agent/Main.h"

#include "fboss/agent/FbossInit.h"
#include "fboss/agent/HwSwitchThriftClientTable.h"
#include "fboss/agent/SwAgentInitializer.h"
#include "fboss/agent/SwSwitch.h"
#include "fboss/agent/SwSwitchWarmBootHelper.h"
#include "fboss/agent/test/LinkStateToggler.h"
#include "fboss/agent/test/RouteDistributionGenerator.h"
#include "fboss/agent/test/TestEnsembleIf.h"

#include "fboss/agent/if/gen-cpp2/AgentHwTestCtrl.h"

DECLARE_string(config);
DECLARE_bool(setup_for_warmboot);

namespace facebook::fboss {
class StateObserver;
class AgentEnsemble;
class HwSwitch;

using AgentEnsembleSwitchConfigFn =
    std::function<cfg::SwitchConfig(const AgentEnsemble&)>;
using AgentEnsemblePlatformConfigFn = std::function<void(cfg::PlatformConfig&)>;

class AgentEnsemble : public TestEnsembleIf {
 public:
  AgentEnsemble() : AgentEnsemble("agent.conf") {}
  explicit AgentEnsemble(const std::string& configFileName);
  virtual ~AgentEnsemble() override;
  using TestEnsembleIf::masterLogicalPortIds;
  using StateUpdateFn = SwSwitch::StateUpdateFn;
  using TestEnsembleIf::getLatestPortStats;
  using TestEnsembleIf::getLatestSysPortStats;

  void setupEnsemble(
      AgentEnsembleSwitchConfigFn initConfig,
      bool disableLinkStateToggler,
      AgentEnsemblePlatformConfigFn platformConfig =
          AgentEnsemblePlatformConfigFn(),
      uint32_t featuresDesired =
          (HwSwitch::FeaturesDesired::PACKET_RX_DESIRED |
           HwSwitch::FeaturesDesired::LINKSCAN_DESIRED |
           HwSwitch::FeaturesDesired::TAM_EVENT_NOTIFY_DESIRED),
      bool failHwCallsOnWarmboot = false);

  void startAgent(bool failHwCallsOnWarmboot = false);

  void applyNewConfig(const cfg::SwitchConfig& config, bool activate);

  void setupLinkStateToggler();

  SwSwitch* getSw() const {
    return agentInitializer()->sw();
  }

  std::shared_ptr<SwitchState> getProgrammedState() const override {
    return getSw()->getState();
  }

  const std::map<int32_t, cfg::PlatformPortEntry>& getPlatformPorts()
      const override {
    return getSw()->getPlatformMapping()->getPlatformPorts();
  }

  void applyInitialConfig(const cfg::SwitchConfig& config) override {
    // agent will start after writing an initial config.
    applyNewConfig(config, false);
  }

  std::shared_ptr<SwitchState> applyNewConfig(
      const cfg::SwitchConfig& config) override {
    applyNewConfig(config, true);
    return getProgrammedState();
  }

  std::unique_ptr<RouteUpdateWrapper> getRouteUpdaterWrapper() override {
    return std::make_unique<SwSwitchRouteUpdateWrapper>(
        getSw()->getRouteUpdater());
  }

  virtual const SwAgentInitializer* agentInitializer() const = 0;
  virtual SwAgentInitializer* agentInitializer() = 0;
  virtual void createSwitch(
      std::unique_ptr<AgentConfig> config,
      uint32_t hwFeaturesDesired,
      PlatformInitFn initPlatform) = 0;
  virtual void reloadPlatformConfig() = 0;

  void applyNewState(
      StateUpdateFn fn,
      const std::string& name = "test-update",
      bool transaction = false) override;

  std::vector<PortID> masterLogicalPortIds() const override;

  std::vector<PortID> masterLogicalPortIds(SwitchID switchID) const;

  void switchRunStateChanged(SwitchRunState runState) override;

  void programRoutes(
      const RouterID& rid,
      const ClientID& client,
      const utility::RouteDistributionGenerator::ThriftRouteChunks&
          routeChunks);

  void unprogramRoutes(
      const RouterID& rid,
      const ClientID& client,
      const utility::RouteDistributionGenerator::ThriftRouteChunks&
          routeChunks);

  void gracefulExit();

  static void enableExactMatch(bcm::BcmConfig& config);

  std::string getInputConfigFile();

  void packetReceived(std::unique_ptr<RxPacket> pkt) noexcept override {
    getSw()->packetReceived(std::move(pkt));
  }

  void linkStateChanged(
      PortID port,
      bool up,
      std::optional<phy::LinkFaultStatus> iPhyFaultStatus =
          std::nullopt) override {
    if (getSw()->getSwitchRunState() >= SwitchRunState::CONFIGURED) {
      if (linkToggler_) {
        linkToggler_->linkStateChanged(port, up);
      }
      getSw()->linkStateChanged(port, up);
    }
  }

  void linkActiveStateChanged(
      const std::map<PortID, bool>& port2IsActive) override {
    getSw()->linkActiveStateChanged(port2IsActive);
  }
  void linkConnectivityChanged(
      const std::map<PortID, multiswitch::FabricConnectivityDelta>&
          port2OldAndNewConnectivity) override {
    getSw()->linkConnectivityChanged(port2OldAndNewConnectivity);
  }

  void switchReachabilityChanged(
      const SwitchID switchId,
      const std::map<SwitchID, std::set<PortID>>& switchReachabilityInfo)
      override {
    getSw()->switchReachabilityChanged(switchId, switchReachabilityInfo);
  }

  void l2LearningUpdateReceived(
      L2Entry l2Entry,
      L2EntryUpdateType l2EntryUpdateType) override {
    getSw()->l2LearningUpdateReceived(l2Entry, l2EntryUpdateType);
  }

  void exitFatal() const noexcept override {
    getSw()->exitFatal();
  }

  void pfcWatchdogStateChanged(const PortID& port, const bool deadlock)
      override {
    getSw()->pfcWatchdogStateChanged(port, deadlock);
  }

  std::map<PortID, HwPortStats> getLatestPortStats(
      const std::vector<PortID>& ports) override;

  std::map<SystemPortID, HwSysPortStats> getLatestSysPortStats(
      const std::vector<SystemPortID>& ports) override;

  void setLoopbackMode(cfg::PortLoopbackMode mode) {
    mode_ = mode;
  }

  const SwitchIdScopeResolver& scopeResolver() const override {
    CHECK(getSw()->getScopeResolver());
    return *(getSw()->getScopeResolver());
  }

  HwAsicTable* getHwAsicTable() override {
    return getSw()->getHwAsicTable();
  }

  const HwAsicTable* getHwAsicTable() const override {
    return getSw()->getHwAsicTable();
  }

  std::map<PortID, FabricEndpoint> getFabricConnectivity(
      SwitchID switchId) const override;

  FabricReachabilityStats getFabricReachabilityStats() const override {
    return getSw()->getFabricReachabilityStats();
  }

  void updateStats() override {
    getSw()->updateStats();
  }

  void registerStateObserver(StateObserver* observer, const std::string& name)
      override;
  void unregisterStateObserver(StateObserver* observer) override;

  virtual HwSwitch* getHwSwitch() = 0;
  virtual const HwSwitch* getHwSwitch() const = 0;
  void runDiagCommand(
      const std::string& input,
      std::string& output,
      std::optional<SwitchID> switchId = std::nullopt) override;

  void clearPortStats(
      const std::unique_ptr<std::vector<int32_t>>& ports) override;

  void clearPortStats();

  LinkStateToggler* getLinkToggler() override;

  folly::MacAddress getLocalMac(SwitchID id) const override {
    return getSw()->getLocalMac(id);
  }
  void waitForLineRateOnPort(PortID port);
  void waitForSpecificRateOnPort(
      PortID port,
      const uint64_t desiredBps,
      int secondsToWaitPerIteration = 1);

  void sendPacketAsync(
      std::unique_ptr<TxPacket> pkt,
      std::optional<PortDescriptor> portDescriptor,
      std::optional<uint8_t> queueId) override;

  std::unique_ptr<TxPacket> allocatePacket(uint32_t size) override;

  bool supportsAddRemovePort() const override {
    return getSw()->getPlatformSupportsAddRemovePort();
  }

  const PlatformMapping* getPlatformMapping() const override {
    return getSw()->getPlatformMapping();
  }

  void bringUpPort(PortID port) {
    bringUpPorts({port});
  }
  void bringDownPort(PortID port) {
    bringDownPorts({port});
  }
  void bringUpPorts(const std::vector<PortID>& ports);
  void bringDownPorts(const std::vector<PortID>& ports);

  cfg::SwitchConfig getCurrentConfig() const override {
    return getSw()->getConfig();
  }
  bool ensureSendPacketSwitched(std::unique_ptr<TxPacket> pkt);
  bool ensureSendPacketOutOfPort(
      std::unique_ptr<TxPacket> pkt,
      PortID portID,
      std::optional<uint8_t> queue = std::nullopt);

  virtual void ensureHwSwitchConnected(SwitchID switchId) = 0;
  uint64_t getTrafficRate(
      const HwPortStats& prevPortStats,
      const HwPortStats& curPortStats,
      const int secondsBetweenStatsCollection);

  std::unique_ptr<apache::thrift::Client<utility::AgentHwTestCtrl>>
  getHwAgentTestClient(SwitchID switchId);
  BootType getBootType() const;

 protected:
  void joinAsyncInitThread() {
    if (asyncInitThread_) {
      asyncInitThread_->join();
      asyncInitThread_.reset();
    }
  }

 private:
  void setConfigFiles(const std::string& fileName);
  void setBootType();
  void overrideConfigFlag(const std::string& fileName);

  void writeConfig(const cfg::SwitchConfig& config);
  void writeConfig(const cfg::AgentConfig& config);
  void writeConfig(const cfg::AgentConfig& config, const std::string& file);
  bool waitForRateOnPort(
      PortID port,
      uint64_t desiredBps,
      int secondsToWaitPerIteration = 2);

  cfg::SwitchConfig initialConfig_;
  std::unique_ptr<std::thread> asyncInitThread_{nullptr};
  std::vector<PortID> masterLogicalPortIds_; /* all ports */
  std::map<SwitchID, std::vector<PortID>>
      switchId2PortIds_; /* per switch ports */
  std::string configFile_{};
  std::unique_ptr<LinkStateToggler> linkToggler_;
  cfg::PortLoopbackMode mode_{cfg::PortLoopbackMode::MAC};
  BootType bootType_{BootType::COLD_BOOT};
};

void initEnsemble(
    PlatformInitFn initPlatform,
    std::optional<cfg::StreamType> streamType = std::nullopt);

std::unique_ptr<AgentEnsemble> createAgentEnsemble(
    AgentEnsembleSwitchConfigFn initialConfigFn,
    bool disableLinkStateToggler,
    AgentEnsemblePlatformConfigFn platformConfigFn =
        AgentEnsemblePlatformConfigFn(),
    uint32_t featuresDesired =
        (HwSwitch::FeaturesDesired::PACKET_RX_DESIRED |
         HwSwitch::FeaturesDesired::LINKSCAN_DESIRED |
         HwSwitch::FeaturesDesired::TAM_EVENT_NOTIFY_DESIRED),
    bool failHwCallsOnWarmboot = false);

} // namespace facebook::fboss
