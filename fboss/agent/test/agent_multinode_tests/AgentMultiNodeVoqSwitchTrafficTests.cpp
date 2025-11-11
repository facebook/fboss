// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/test/agent_multinode_tests/AgentMultiNodeVoqSwitchNeighborTests.h"

namespace facebook::fboss {

class AgentMultiNodeVoqSwitchTrafficTest
    : public AgentMultiNodeVoqSwitchNeighborTest {
 public:
  bool verifyTrafficSpray(
      const std::unique_ptr<utility::TopologyInfo>& topologyInfo) const {
    return true;
  }
};

TEST_F(AgentMultiNodeVoqSwitchTrafficTest, verifyTrafficSpray) {
  runTestWithVerifyCluster([this](const auto& topologyInfo) {
    switch (topologyInfo->getTopologyType()) {
      case utility::TopologyInfo::TopologyType::DSF:
        return this->verifyTrafficSpray(topologyInfo);
    }
  });
}

} // namespace facebook::fboss
