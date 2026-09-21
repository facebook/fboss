// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/agent/single/MonolithicAgentInitializer.h"
#include "fboss/agent/test/AgentEnsemble.h"

namespace facebook::fboss {

class MonolithicTestAgentInitializer : public MonolithicAgentInitializer {
 public:
  std::vector<std::shared_ptr<apache::thrift::AsyncProcessorFactory>>
  getThrifthandlers() override;
};

class MonoAgentEnsemble : public AgentEnsemble {
 public:
  ~MonoAgentEnsemble() override;

  const SwAgentInitializer* agentInitializer() const override;
  SwAgentInitializer* agentInitializer() override;
  void createSwitch(
      std::unique_ptr<AgentConfig> config,
      uint32_t hwFeaturesDesired,
      PlatformInitFn initPlatform) override;
  void reloadPlatformConfig() override;
  bool isSai() const override;
  void ensureHwSwitchConnected(SwitchID /*switchId*/) override {
    // nothing to do for monolithic agent
  }

  cfg::SwitchingMode getFwdSwitchingMode(
      const RouteNextHopEntry& entry) override;

 private:
  MonolithicTestAgentInitializer agentInitializer_{};
};
} // namespace facebook::fboss
