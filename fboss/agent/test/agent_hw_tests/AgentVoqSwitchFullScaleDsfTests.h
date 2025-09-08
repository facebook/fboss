// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/agent/test/agent_hw_tests/AgentVoqSwitchTests.h"

#include "fboss/agent/AsicUtils.h"
#include "fboss/agent/test/utils/DsfConfigUtils.h"
#include "fboss/agent/test/utils/LoadBalancerTestUtils.h"
#include "fboss/agent/test/utils/ScaleTestUtils.h"

namespace facebook::fboss {

class AgentVoqSwitchFullScaleDsfNodesTest : public AgentVoqSwitchTest {
 public:
  cfg::SwitchConfig initialConfig(
      const AgentEnsemble& ensemble) const override {
    auto cfg = AgentVoqSwitchTest::initialConfig(ensemble);
    cfg.dsfNodes() = *overrideDsfNodes(*cfg.dsfNodes());
    cfg.loadBalancers()->push_back(
        utility::getEcmpFullHashConfig(ensemble.getL3Asics()));
    return cfg;
  }

  std::optional<std::map<int64_t, cfg::DsfNode>> overrideDsfNodes(
      const std::map<int64_t, cfg::DsfNode>& curDsfNodes) const {
    return utility::addRemoteIntfNodeCfg(curDsfNodes);
  }

 protected:
  int getMaxEcmpWidth() const {
    return isDualStage3Q2QMode() ? 2048 : 512;
  }

  int getMaxEcmpGroup() const {
    auto groups = checkSameAndGetAsic(getL3Asics())->getMaxEcmpGroups();
    CHECK(groups.has_value());
    return *groups;
  }

  flat_set<PortDescriptor> getRemoteSysPortDesc() {
    auto remoteSysPorts =
        getProgrammedState()->getRemoteSystemPorts()->getAllNodes();
    flat_set<PortDescriptor> sysPortDescs;
    std::for_each(
        remoteSysPorts->begin(),
        remoteSysPorts->end(),
        [&sysPortDescs](const auto& idAndPort) {
          if (!idAndPort.second->isStatic()) {
            sysPortDescs.insert(
                PortDescriptor(static_cast<SystemPortID>(idAndPort.first)));
          }
        });
    return sysPortDescs;
  }

 protected:
  void setCmdLineFlagOverrides() const override {
    AgentVoqSwitchTest::setCmdLineFlagOverrides();
    // Collect sats less frequently.
    FLAGS_update_stats_interval_s = 120;
    // Allow 100% ECMP resource usage
    FLAGS_ecmp_resource_percentage = 100;
    FLAGS_ecmp_width = getMaxEcmpWidth();
  }
};
} // namespace facebook::fboss
