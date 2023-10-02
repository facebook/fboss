// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/agent/mnpu/SplitSwAgentInitializer.h"
#include "fboss/agent/test/AgentEnsemble.h"

namespace facebook::fboss {

class SplitAgentEnsemble : public AgentEnsemble {
 public:
  ~SplitAgentEnsemble() override;

  const SwAgentInitializer* agentInitializer() const override;
  SwAgentInitializer* agentInitializer() override;
  void createSwitch(
      std::unique_ptr<AgentConfig> config,
      uint32_t hwFeaturesDesired,
      PlatformInitFn initPlatform) override;
  void reloadPlatformConfig() override;

 private:
  std::unique_ptr<SplitSwAgentInitializer> agentInitializer_;
};
} // namespace facebook::fboss
