// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/test/agent_multinode_tests/AgentMultiNodeDsfUtils.h"
#include "fboss/agent/test/agent_multinode_tests/AgentMultiNodeTest.h"

namespace facebook::fboss {

class AgentMultiNodeVoqSwitchTest : public AgentMultiNodeTest {};

TEST_F(AgentMultiNodeVoqSwitchTest, verifyGracefulFabricLinkDownUp) {
  runTestWithVerifyCluster([](const auto& topologyInfo) {
    switch (topologyInfo->getTopologyType()) {
      case utility::TopologyInfo::TopologyType::DSF:
        return utility::verifyDsfGracefulFabricLinkDownUp(topologyInfo);
    }
  });
}

TEST_F(AgentMultiNodeVoqSwitchTest, verifyFabricLinkDrainUndrain) {
  runTestWithVerifyCluster([](const auto& topologyInfo) {
    switch (topologyInfo->getTopologyType()) {
      case utility::TopologyInfo::TopologyType::DSF:
        return utility::verifyDsfFabricLinkDrainUndrain(topologyInfo);
    }
  });
}

TEST_F(AgentMultiNodeVoqSwitchTest, verifyGracefulAgentRestartTimeoutRecovery) {
  runTestWithVerifyCluster([](const auto& topologyInfo) {
    switch (topologyInfo->getTopologyType()) {
      case utility::TopologyInfo::TopologyType::DSF:
        return utility::verifyDsfGracefulAgentRestartTimeoutRecovery(
            topologyInfo);
    }
  });
}

} // namespace facebook::fboss
