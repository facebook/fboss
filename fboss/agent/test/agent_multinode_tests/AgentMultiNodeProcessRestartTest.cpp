// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/test/agent_multinode_tests/AgentMultiNodeDsfUtils.h"
#include "fboss/agent/test/agent_multinode_tests/AgentMultiNodeTest.h"

namespace facebook::fboss {

class AgentMultiNodeProcessRestartTest : public AgentMultiNodeTest {};

TEST_F(AgentMultiNodeProcessRestartTest, verifyAgentDownUp) {
  switch (topologyInfo_->getTopologyType()) {
    case utility::TopologyInfo::TopologyType::DSF:
      utility::verifyDsfAgentDownUp();
      break;
  }
}

} // namespace facebook::fboss
