// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/test/MonoAgentEnsemble.h"

namespace facebook::fboss {

MonoAgentEnsemble::~MonoAgentEnsemble() {
  agentInitializer_.stopAgent(false);
}

const SwAgentInitializer* MonoAgentEnsemble::agentInitializer() const {
  return &agentInitializer_;
}

SwAgentInitializer* MonoAgentEnsemble::agentInitializer() {
  return &agentInitializer_;
}

void MonoAgentEnsemble::createSwitch(
    std::unique_ptr<AgentConfig> config,
    uint32_t hwFeaturesDesired,
    PlatformInitFn initPlatform) {
  agentInitializer_.createSwitch(
      std::move(config), hwFeaturesDesired, initPlatform);
}

void MonoAgentEnsemble::reloadPlatformConfig() {
  agentInitializer_.platform()->reloadConfig();
}

std::unique_ptr<AgentEnsemble> createAgentEnsemble(
    AgentEnsembleSwitchConfigFn initialConfigFn,
    AgentEnsemblePlatformConfigFn platformConfigFn,
    uint32_t featuresDesired,
    bool startAgent) {
  auto ensemble = std::make_unique<MonoAgentEnsemble>();
  ensemble->setupEnsemble(featuresDesired, initialConfigFn, platformConfigFn);
  if (featuresDesired & HwSwitch::FeaturesDesired::LINKSCAN_DESIRED) {
    ensemble->setupLinkStateToggler();
  }
  if (startAgent) {
    ensemble->startAgent();
  }
  return ensemble;
}
} // namespace facebook::fboss
