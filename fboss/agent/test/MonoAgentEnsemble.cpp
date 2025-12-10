// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/test/MonoAgentEnsemble.h"
#include <gtest/gtest.h>
#include "fboss/agent/Main.h"

#include "fboss/agent/hw/test/HwTestThriftHandler.h"

namespace facebook::fboss {

MonoAgentEnsemble::~MonoAgentEnsemble() {
  bool gracefulExit = !::testing::Test::HasFailure();
  agentInitializer_.stopAgent(
      false /* setupWarmboot */, gracefulExit /* gracefulExit */);
  // wait for async thread to finish to prevent race between
  // - stopping of thrift server
  // - destruction of agentInitializer_ triggering destruction of thrift server
  // this race can cause accessing data which has already been destroyed
  joinAsyncInitThread();
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

std::vector<std::shared_ptr<apache::thrift::AsyncProcessorFactory>>
MonolithicTestAgentInitializer::getThrifthandlers() {
  auto handlers = MonolithicAgentInitializer::getThrifthandlers();

  auto testUtilsHandler =
      utility::createHwTestThriftHandler(platform()->getHwSwitch());
  CHECK(testUtilsHandler);
  handlers.push_back(testUtilsHandler);
  return handlers;
}

cfg::SwitchingMode MonoAgentEnsemble::getFwdSwitchingMode(
    const RouteNextHopEntry& entry) {
  return agentInitializer_.platform()->getHwSwitch()->getFwdSwitchingMode(
      entry);
}

std::unique_ptr<AgentEnsemble> createAgentEnsemble(
    AgentEnsembleSwitchConfigFn initialConfigFn,
    bool disableLinkStateToggler,
    AgentEnsemblePlatformConfigFn platformConfigFn,
    uint32_t featuresDesired,
    bool failHwCallsOnWarmboot) {
  std::unique_ptr<AgentEnsemble> ensemble =
      std::make_unique<MonoAgentEnsemble>();
  ensemble->setupEnsemble(
      initialConfigFn,
      disableLinkStateToggler,
      platformConfigFn,
      featuresDesired,
      failHwCallsOnWarmboot);
  return ensemble;
}

} // namespace facebook::fboss
