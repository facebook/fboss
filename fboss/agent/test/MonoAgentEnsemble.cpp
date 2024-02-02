// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/test/MonoAgentEnsemble.h"

#include "fboss/agent/Main.h"

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
  setVersionInfo();
  agentInitializer_.createSwitch(
      std::move(config), hwFeaturesDesired, initPlatform);
}

void MonoAgentEnsemble::reloadPlatformConfig() {
  agentInitializer_.platform()->reloadConfig();
}

bool MonoAgentEnsemble::isSai() const {
  return agentInitializer_.platform()->isSai();
}

HwSwitch* MonoAgentEnsemble::getHwSwitch() const {
  return agentInitializer_.platform()->getHwSwitch();
}

std::unique_ptr<AgentEnsemble> createAgentEnsemble(
    AgentEnsembleSwitchConfigFn initialConfigFn,
    AgentEnsemblePlatformConfigFn platformConfigFn,
    uint32_t featuresDesired) {
  std::unique_ptr<AgentEnsemble> ensemble =
      std::make_unique<MonoAgentEnsemble>();
  ensemble->setupEnsemble(featuresDesired, initialConfigFn, platformConfigFn);
  return ensemble;
}

} // namespace facebook::fboss
