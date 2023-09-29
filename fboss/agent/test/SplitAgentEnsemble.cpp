// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/test/SplitAgentEnsemble.h"

namespace facebook::fboss {

SplitAgentEnsemble::~SplitAgentEnsemble() {
  agentInitializer_->stopAgent(false);
}

const SwAgentInitializer* SplitAgentEnsemble::agentInitializer() const {
  CHECK(agentInitializer_);
  return agentInitializer_.get();
}

SwAgentInitializer* SplitAgentEnsemble::agentInitializer() {
  CHECK(agentInitializer_);
  return agentInitializer_.get();
}

void SplitAgentEnsemble::createSwitch(
    std::unique_ptr<AgentConfig> /* config */,
    uint32_t /* hwFeaturesDesired */,
    PlatformInitFn /* initPlatform */) {
  // TODO: Initialize sw in createSwitch.
  return;
}

void SplitAgentEnsemble::reloadPlatformConfig() {
  // No-op for Split Agent Ensemble.
  return;
}

std::unique_ptr<AgentEnsemble> createAgentEnsemble(
    AgentEnsembleSwitchConfigFn initialConfigFn,
    AgentEnsemblePlatformConfigFn platformConfigFn,
    uint32_t featuresDesired,
    bool startAgent) {
  auto ensemble = std::make_unique<SplitAgentEnsemble>();
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
