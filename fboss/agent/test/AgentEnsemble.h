// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include <fboss/agent/gen-cpp2/agent_config_types.h>
#include <fboss/agent/gen-cpp2/platform_config_types.h>
#include <fboss/agent/gen-cpp2/switch_config_types.h>
#include <fboss/agent/hw/bcm/gen-cpp2/bcm_config_types.h>
#include <functional>
#include "fboss/agent/L2Entry.h"
#include "fboss/agent/Main.h"

#include "fboss/agent/test/RouteDistributionGenerator.h"
#include "fboss/agent/test/TestEnsembleIf.h"

DECLARE_string(config);
DECLARE_bool(setup_for_warmboot);

namespace facebook::fboss {

using AgentEnsembleSwitchConfigFn = std::function<
    cfg::SwitchConfig(HwSwitch* hwSwitch, const std::vector<PortID>&)>;
using AgentEnsemblePlatformConfigFn = std::function<void(cfg::PlatformConfig&)>;

class AgentEnsemble : public TestEnsembleIf {
 public:
  AgentEnsemble() {}
  explicit AgentEnsemble(const std::string& configFileName);

  ~AgentEnsemble();
  void setupEnsemble(
      int argc,
      char** argv,
      uint32_t hwFeaturesDesired,
      PlatformInitFn initPlatform,
      AgentEnsembleSwitchConfigFn initConfig,
      AgentEnsemblePlatformConfigFn platformConfig =
          AgentEnsemblePlatformConfigFn());

  void startAgent();

  void applyNewConfig(const cfg::SwitchConfig& config, bool activate);

  const AgentInitializer* agentInitializer() const {
    return &agentInitializer_;
  }

  AgentInitializer* agentInitializer() {
    return &agentInitializer_;
  }

  SwSwitch* getSw() const {
    return agentInitializer_.sw();
  }

  std::shared_ptr<SwitchState> getProgrammedState() const override {
    return getSw()->getState();
  }

  void applyInitialConfig(const cfg::SwitchConfig& /* config */) override {
    throw FbossError("Not Implement");
  }

  std::shared_ptr<SwitchState> applyNewConfig(
      const cfg::SwitchConfig& config) override {
    applyNewConfig(config, true);
    return getProgrammedState();
  }

  Platform* getPlatform() const {
    return agentInitializer_.platform();
  }

  HwSwitch* getHw() const {
    return getSw()->getHw();
  }

  std::shared_ptr<SwitchState> applyNewState(
      std::shared_ptr<SwitchState> state,
      bool transaction = false) override;

  const std::vector<PortID>& masterLogicalPortIds() const;

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

  static std::string getInputConfigFile();

  void packetReceived(std::unique_ptr<RxPacket> /*pkt*/) noexcept override {}

  void linkStateChanged(
      PortID /*port*/,
      bool /*up*/,
      std::optional<phy::LinkFaultStatus> /*iPhyFaultStatus*/ =
          std::nullopt) override {}

  void l2LearningUpdateReceived(
      L2Entry /* l2Entry */,
      L2EntryUpdateType /*l2EntryUpdateType*/) override {}

  void exitFatal() const noexcept override {}

  void pfcWatchdogStateChanged(const PortID& /*port*/, const bool /*deadlock*/)
      override {}

  HwSwitch* getHwSwitch() override {
    return getHw();
  }

  const HwSwitch* getHwSwitch() const override {
    return getHw();
  }

 private:
  void writeConfig(const cfg::SwitchConfig& config);
  void writeConfig(const cfg::AgentConfig& config);
  void writeConfig(const cfg::AgentConfig& config, const std::string& file);

  AgentInitializer agentInitializer_{};
  cfg::SwitchConfig initialConfig_;
  std::unique_ptr<std::thread> asyncInitThread_{nullptr};
  std::vector<PortID> masterLogicalPortIds_;
  std::string configFile_{"agent.conf"};
};

void ensembleMain(int argc, char* argv[], PlatformInitFn initPlatform);

std::unique_ptr<AgentEnsemble> createAgentEnsemble(
    AgentEnsembleSwitchConfigFn initialConfigFn,
    AgentEnsemblePlatformConfigFn platformConfigFn =
        AgentEnsemblePlatformConfigFn(),
    uint32_t featuresDesired =
        (HwSwitch::FeaturesDesired::PACKET_RX_DESIRED |
         HwSwitch::FeaturesDesired::LINKSCAN_DESIRED));

} // namespace facebook::fboss
