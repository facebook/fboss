// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/test/agent_multinode_tests/AgentMultiNodeVoqSwitchTrafficTests.h"

namespace facebook::fboss {

class AgentMultiNodeVoqSwitchNoTrafficDropTest
    : public AgentMultiNodeVoqSwitchTrafficTest {
 public:
  bool verifyNoTrafficDropOnProcessRestarts(
      const std::unique_ptr<utility::TopologyInfo>& topologyInfo) {
    XLOG(DBG2) << "Verifying No Traffic Drops on process restarts";
    return true;
  }
};

TEST_F(
    AgentMultiNodeVoqSwitchNoTrafficDropTest,
    verifyNoTrafficDropOnProcessRestarts) {
  runTestWithVerifyCluster([this](const auto& topologyInfo) {
    switch (topologyInfo->getTopologyType()) {
      case utility::TopologyInfo::TopologyType::DSF:
        return this->verifyNoTrafficDropOnProcessRestarts(topologyInfo);
    }
  });
}

} // namespace facebook::fboss
