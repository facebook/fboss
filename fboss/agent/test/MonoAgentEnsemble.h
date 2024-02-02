// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/agent/single/MonolithicAgentInitializer.h"
#include "fboss/agent/test/AgentEnsemble.h"

namespace facebook::fboss {

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

  HwSwitch* getHwSwitch() const override;

 private:
  MonolithicAgentInitializer agentInitializer_{};
};
} // namespace facebook::fboss
