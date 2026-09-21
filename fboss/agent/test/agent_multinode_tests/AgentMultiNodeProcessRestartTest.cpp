// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/test/agent_multinode_tests/AgentMultiNodeDsfUtils.h"
#include "fboss/agent/test/agent_multinode_tests/AgentMultiNodeTest.h"

namespace facebook::fboss {

class AgentMultiNodeProcessRestartTest : public AgentMultiNodeTest {};

TEST_F(AgentMultiNodeProcessRestartTest, verifyGracefulAgentRestart) {
  runTestWithVerifyCluster([](const auto& topologyInfo) {
    switch (topologyInfo->getTopologyType()) {
      case utility::TopologyInfo::TopologyType::DSF:
        return utility::verifyDsfGracefulAgentRestart(topologyInfo);
    }
  });
}
TEST_F(AgentMultiNodeProcessRestartTest, verifyUngracefulAgentRestart) {
  runTestWithVerifyCluster([](const auto& topologyInfo) {
    switch (topologyInfo->getTopologyType()) {
      case utility::TopologyInfo::TopologyType::DSF:
        return utility::verifyDsfUngracefulAgentRestart(topologyInfo);
    }
  });
}

TEST_F(AgentMultiNodeProcessRestartTest, verifyGracefulQsfpRestart) {
  runTestWithVerifyCluster([](const auto& topologyInfo) {
    switch (topologyInfo->getTopologyType()) {
      case utility::TopologyInfo::TopologyType::DSF:
        return utility::verifyDsfGracefulQsfpRestart(topologyInfo);
    }
  });
}

TEST_F(AgentMultiNodeProcessRestartTest, verifyUngracefulQsfpRestart) {
  runTestWithVerifyCluster([](const auto& topologyInfo) {
    switch (topologyInfo->getTopologyType()) {
      case utility::TopologyInfo::TopologyType::DSF:
        return utility::verifyDsfUngracefulQsfpRestart(topologyInfo);
    }
  });
}

TEST_F(AgentMultiNodeProcessRestartTest, verifyGracefulFSDBRestart) {
  runTestWithVerifyCluster([](const auto& topologyInfo) {
    switch (topologyInfo->getTopologyType()) {
      case utility::TopologyInfo::TopologyType::DSF:
        return utility::verifyDsfGracefulFSDBRestart(topologyInfo);
    }
  });
}

TEST_F(AgentMultiNodeProcessRestartTest, verifyUngracefulFSDBRestart) {
  runTestWithVerifyCluster([](const auto& topologyInfo) {
    switch (topologyInfo->getTopologyType()) {
      case utility::TopologyInfo::TopologyType::DSF:
        return utility::verifyDsfUngracefulFSDBRestart(topologyInfo);
    }
  });
}

} // namespace facebook::fboss
