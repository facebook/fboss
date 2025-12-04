// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/agent/mnpu/SplitSwAgentInitializer.h"
#include "fboss/agent/test/AgentEnsemble.h"

DECLARE_bool(multi_switch);

namespace facebook::fboss {

class MultiSwitchAgentEnsemble : public AgentEnsemble {
 public:
  ~MultiSwitchAgentEnsemble() override;

  const SwAgentInitializer* agentInitializer() const override;
  SwAgentInitializer* agentInitializer() override;
  void createSwitch(
      std::unique_ptr<AgentConfig> config,
      uint32_t hwFeaturesDesired,
      PlatformInitFn initPlatform) override;
  void reloadPlatformConfig() override;
  bool isSai() const override;
  void ensureHwSwitchConnected(SwitchID switchId) override;
  cfg::SwitchingMode getFwdSwitchingMode(
      const RouteNextHopEntry& entry) override;

 private:
  std::unique_ptr<SplitSwAgentInitializer> agentInitializer_;
};
} // namespace facebook::fboss
