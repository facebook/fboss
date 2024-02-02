// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/test/MultiSwitchAgentEnsemble.h"

namespace facebook::fboss {
MultiSwitchAgentEnsemble::~MultiSwitchAgentEnsemble() {
  agentInitializer_->stopAgent(false);
  // wait for async thread to finish to prevent race between
  // - stopping of thrift server
  // - destruction of agentInitializer_ triggering destruction of thrift server
  // this race can cause accessing data which has already been destroyed
  joinAsyncInitThread();
}

const SwAgentInitializer* MultiSwitchAgentEnsemble::agentInitializer() const {
  CHECK(agentInitializer_);
  return agentInitializer_.get();
}

SwAgentInitializer* MultiSwitchAgentEnsemble::agentInitializer() {
  CHECK(agentInitializer_);
  return agentInitializer_.get();
}

void MultiSwitchAgentEnsemble::createSwitch(
    std::unique_ptr<AgentConfig> /* config */,
    uint32_t /* hwFeaturesDesired */,
    PlatformInitFn /* initPlatform */) {
  // For MultiSwitchAgentEnsemble, initialize splitSwAgentInitializer
  agentInitializer_ = std::make_unique<SplitSwAgentInitializer>();
  return;
}

void MultiSwitchAgentEnsemble::reloadPlatformConfig() {
  // No-op for Split Agent Ensemble.
  return;
}

bool MultiSwitchAgentEnsemble::isSai() const {
  // MultiSwitch Agent ensemble requires config to provide SDK version.
  auto sdkVersion = getSw()->getSdkVersion();
  CHECK(sdkVersion.has_value() && sdkVersion.value().asicSdk().has_value());
  return sdkVersion.value().saiSdk().has_value();
}

HwSwitch* MultiSwitchAgentEnsemble::getHwSwitch() const {
  throw FbossError("getHwSwitch is unsupported for MultiSwitchAgentEnsemble");
  return nullptr;
}

std::unique_ptr<AgentEnsemble> createAgentEnsemble(
    AgentEnsembleSwitchConfigFn initialConfigFn,
    AgentEnsemblePlatformConfigFn platformConfigFn,
    uint32_t featuresDesired) {
  // Set multi switch flag to true for MultiSwitchAgentEnsemble
  FLAGS_multi_switch = true;
  std::unique_ptr<AgentEnsemble> ensemble =
      std::make_unique<MultiSwitchAgentEnsemble>();
  ensemble->setupEnsemble(featuresDesired, initialConfigFn, platformConfigFn);
  return ensemble;
}

} // namespace facebook::fboss
