// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/test/MultiSwitchAgentEnsemble.h"
#include <gtest/gtest.h>

#include "fboss/agent/SwitchStats.h"

namespace facebook::fboss {
MultiSwitchAgentEnsemble::~MultiSwitchAgentEnsemble() {
  bool gracefulExit = !::testing::Test::HasFailure();
  agentInitializer_->stopAgent(
      false /* setupWarmboot */, gracefulExit /* gracefulExit */);
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

std::unique_ptr<AgentEnsemble> createAgentEnsemble(
    AgentEnsembleSwitchConfigFn initialConfigFn,
    bool disableLinkStateToggler,
    AgentEnsemblePlatformConfigFn platformConfigFn,
    uint32_t featuresDesired,
    bool failHwCallsOnWarmboot) {
  // Set multi switch flag to true for MultiSwitchAgentEnsemble
  FLAGS_multi_switch = true;
  std::unique_ptr<AgentEnsemble> ensemble =
      std::make_unique<MultiSwitchAgentEnsemble>();
  ensemble->setupEnsemble(
      initialConfigFn,
      disableLinkStateToggler,
      platformConfigFn,
      featuresDesired,
      failHwCallsOnWarmboot);
  return ensemble;
}

void MultiSwitchAgentEnsemble::ensureHwSwitchConnected(SwitchID switchId) {
  auto switchIndex =
      getSw()->getSwitchInfoTable().getSwitchIndexFromSwitchId(switchId);
  WITH_RETRIES({
    std::map<int16_t, HwAgentEventSyncStatus> statusMap;
    getSw()->stats()->getHwAgentStatus(statusMap);
    EXPECT_EVENTUALLY_EQ(*(statusMap[switchIndex].fdbEventSyncActive()), 1);
    EXPECT_EVENTUALLY_EQ(*(statusMap[switchIndex].txPktEventSyncActive()), 1);
    EXPECT_EVENTUALLY_EQ(*(statusMap[switchIndex].rxPktEventSyncActive()), 1);
    EXPECT_EVENTUALLY_EQ(*(statusMap[switchIndex].linkEventSyncActive()), 1);
    EXPECT_EVENTUALLY_EQ(*(statusMap[switchIndex].statsEventSyncActive()), 1);
  });
}

cfg::SwitchingMode MultiSwitchAgentEnsemble::getFwdSwitchingMode(
    const RouteNextHopEntry& entry) {
  auto switchId =
      getSw()->getScopeResolver()->scope(masterLogicalPortIds()[0]).switchId();
  auto client = getHwAgentTestClient(switchId);
  return client->sync_getFwdSwitchingMode(entry.toThrift());
}

} // namespace facebook::fboss
