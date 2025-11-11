// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/test/agent_multinode_tests/AgentMultiNodeVoqSwitchTrafficTests.h"

#include "fboss/agent/test/agent_multinode_tests/AgentMultiNodeDsfUtils.h"

namespace facebook::fboss {

class AgentMultiNodeVoqSwitchNoTrafficDropTest
    : public AgentMultiNodeVoqSwitchTrafficTest {
 protected:
  struct Scenario {
    std::string name;
    std::function<bool()> setup;
  };

  // Return true only if all scenarios are successful
  bool runScenariosAndVerifyNoDrops(
      const std::unique_ptr<utility::TopologyInfo>& topologyInfo,
      const std::vector<Scenario>& scenarios) const {
    for (const auto& scenario : scenarios) {
      XLOG(DBG2) << "Running scenario: " << scenario.name;
      if (!scenario.setup()) {
        XLOG(DBG2) << "Scenario: " << scenario.name << " failed";
        return false;
      }

      if (!utility::verifyNoReassemblyErrorsForAllSwitches(topologyInfo)) {
        // TODO query drop counters to root cause reason for reassembly errors
        XLOG(DBG2) << "Scenario: " << scenario.name
                   << " unexpected reassembly errors";
        return false;
      }
    }

    return true;
  }

 public:
  bool verifyNoTrafficDropOnProcessRestarts(
      const std::unique_ptr<utility::TopologyInfo>& topologyInfo) {
    XLOG(DBG2) << "Verifying No Traffic Drops on process restarts";

    if (!setupTrafficLoop(topologyInfo)) {
      XLOG(DBG2) << "Traffic loop setup failed";
      return false;
    }

    if (!utility::verifyNoReassemblyErrorsForAllSwitches(topologyInfo)) {
      XLOG(DBG2) << "Unexpected reassembly errors";
      // TODO query drop counters to root cause reason for reassembly errors
      return false;
    }

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
