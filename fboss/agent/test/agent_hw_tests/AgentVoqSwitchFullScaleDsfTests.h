// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/agent/test/agent_hw_tests/AgentVoqSwitchTests.h"

namespace facebook::fboss {

class AgentVoqSwitchFullScaleDsfNodesTest : public AgentVoqSwitchTest {
 public:
  cfg::SwitchConfig initialConfig(const AgentEnsemble& ensemble) const override;

  std::optional<std::map<int64_t, cfg::DsfNode>> overrideDsfNodes(
      const std::map<int64_t, cfg::DsfNode>& curDsfNodes) const;

 protected:
  int getMaxEcmpWidth() const {
    // J3 starting to support ECMP width of 2K in 12.x.
    // Since the test is also running on 11.x, use 512 that's supported on all
    // SDK versions.
    return 512;
  }

  int getMaxEcmpGroup() const {
    return 64;
  }

  flat_set<PortDescriptor> getRemoteSysPortDesc();

 protected:
  void setCmdLineFlagOverrides() const override;
};
} // namespace facebook::fboss
