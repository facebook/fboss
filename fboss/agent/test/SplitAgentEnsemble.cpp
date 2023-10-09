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
  // For SplitAgentEnsemble, initialize splitSwAgentInitializer
  agentInitializer_ = std::make_unique<SplitSwAgentInitializer>();
  return;
}

void SplitAgentEnsemble::reloadPlatformConfig() {
  // No-op for Split Agent Ensemble.
  return;
}

std::unique_ptr<AgentEnsemble> createAgentEnsemble(
    AgentEnsembleSwitchConfigFn initialConfigFn,
    AgentEnsemblePlatformConfigFn platformConfigFn,
    uint32_t featuresDesired) {
  std::unique_ptr<AgentEnsemble> ensemble =
      std::make_unique<SplitAgentEnsemble>();
  ensemble->setupEnsemble(featuresDesired, initialConfigFn, platformConfigFn);
  return ensemble;
}

} // namespace facebook::fboss
