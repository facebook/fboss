// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/test/agent_multinode_tests/AgentMultiNodeDsfUtils.h"
#include "fboss/agent/test/agent_multinode_tests/AgentMultiNodeTest.h"

namespace facebook::fboss {

class AgentMultiNodeProcessRestartTest : public AgentMultiNodeTest {};

TEST_F(AgentMultiNodeProcessRestartTest, verifyGracefulAgentRestart) {
  runTestWithVerifyCluster([](const auto& topologyInfo) {
    switch (topologyInfo->getTopologyType()) {
      case utility::TopologyInfo::TopologyType::DSF:
        utility::verifyDsfGracefulAgentRestart(topologyInfo);
        break;
    }
  });
}

} // namespace facebook::fboss
