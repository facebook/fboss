// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/test/agent_multinode_tests/AgentMultiNodeDsfUtils.h"
#include "fboss/agent/test/agent_multinode_tests/AgentMultiNodeTest.h"

namespace facebook::fboss {

class AgentMultiNodeVoqSwitchNeighborTest : public AgentMultiNodeTest {
 private:
  struct NeighborInfo {
    int32_t portID = 0;
    int32_t intfID = 0;
    folly::IPAddress ip = folly::IPAddress("::");
    folly::MacAddress mac = folly::MacAddress("00:00:00:00:00:00");
  };
};

TEST_F(AgentMultiNodeVoqSwitchNeighborTest, verifyNeighborAddRemove) {}

} // namespace facebook::fboss
