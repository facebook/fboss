// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/test/agent_multinode_tests/AgentMultiNodeVoqSwitchTrafficTests.h"

#include "fboss/agent/test/agent_multinode_tests/AgentMultiNodeDsfUtils.h"
#include "fboss/agent/test/agent_multinode_tests/AgentMultiNodeUtils.h"
#include "fboss/agent/test/thrift_client_utils/ThriftClientUtils.h"

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

    const auto& switches = topologyInfo->getAllSwitches();

    auto allQsfpRestartHelper = [switches](bool gracefulRestart) {
      {
        // Restart QSFP on all switches
        utility::forEach(
            switches,
            gracefulRestart ? utility::triggerGracefulQsfpRestart
                            : utility::triggerUngracefulQsfpRestart);

        // Wait for QSFP to come up on all switches
        auto restart = utility::checkForEachExcluding(
            switches,
            {}, // exclude none
            [](const std::string& switchName,
               const QsfpServiceRunState& state) {
              return utility::verifyQsfpServiceRunState(switchName, state);
            },
            QsfpServiceRunState::ACTIVE);

        return restart;
      }
    };

    auto allFsdbRestartHelper = [switches](bool gracefulRestart) {
      {
        // Restart FSDB on all switches
        utility::forEach(
            switches,
            gracefulRestart ? utility::triggerGracefulFsdbRestart
                            : utility::triggerUngracefulFsdbRestart);

        // Wait for FSDB to come up on all switches
        auto restart = utility::checkForEachExcluding(
            switches,
            {}, // exclude none
            [](const std::string& switchName,
               const QsfpServiceRunState& state) {
              return utility::verifyFsdbIsUp(switchName);
            },
            QsfpServiceRunState::ACTIVE);

        return restart;
      }
    };

    // With traffic loop running, execute a variety of scenarios.
    // For each scenario, expect no drops on Fabric ports.
    Scenario gracefullyRestartQsfpAllSwitches = {
        "gracefullyRestartQsfpAllSwitches",
        [&] { return allQsfpRestartHelper(true /* gracefulRestart */); }};

    Scenario unGracefullyRestartQsfpAllSwitches = {
        "unGracefullyRestartQsfpAllSwitches",
        [&] { return allQsfpRestartHelper(false /* ungracefulRestart */); }};

    Scenario gracefullyRestartFsdbAllSwitches = {
        "gracefullyRestartFsdbAllSwitches",
        [&] { return allFsdbRestartHelper(true /* gracefulRestart */); }};

    Scenario unGracefullyRestartFsdbAllSwitches = {
        "unGracefullyRestartFsdbAllSwitches",
        [&] { return allFsdbRestartHelper(false /* ungracefulRestart */); }};

    std::vector<Scenario> scenarios = {
        std::move(gracefullyRestartQsfpAllSwitches),
        std::move(unGracefullyRestartQsfpAllSwitches),
        std::move(gracefullyRestartFsdbAllSwitches),
        std::move(unGracefullyRestartFsdbAllSwitches),
    };

    return runScenariosAndVerifyNoDrops(topologyInfo, scenarios);
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
