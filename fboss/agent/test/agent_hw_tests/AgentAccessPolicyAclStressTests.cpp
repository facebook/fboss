// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <gflags/gflags.h>

#include <array>

#include "fboss/agent/test/agent_hw_tests/AgentAccessPolicyAclTestBase.h"

// Sized to keep the slower of these tests inside a 10 minute budget.
DEFINE_int32(
    access_policy_stress_iterations,
    300,
    "config applies per access policy stress test");

namespace facebook::fboss {

namespace {

// The two ports the functional tests verify plus four more, so the churn moves
// more ports than any of them does. Index 2 is the egress port.
constexpr std::array<int, 6> kChurnPortIdxs{0, 1, 3, 4, 5, 6};
constexpr std::array<cfg::AclLookupClassPort, 2> kChurnClasses{
    kRestricted,
    kUnconstrained};
constexpr size_t kChurnRequiredInterfacePorts = 7;

} // namespace

template <typename BaseT>
class AgentAccessPolicyStressTest : public BaseT {
 protected:
  std::optional<size_t> maxRequiredInterfacePorts() const override {
    return kChurnRequiredInterfacePorts;
  }

  static cfg::AclLookupClassPort churnClass(size_t portSlot, int iteration) {
    return kChurnClasses[(portSlot + iteration) % kChurnClasses.size()];
  }

  void applyClassAssignment(int iteration) {
    auto config = this->getAgentEnsemble()->getCurrentConfig();
    auto portIds = this->masterLogicalInterfacePortIds();
    ASSERT_GE(portIds.size(), kChurnRequiredInterfacePorts);
    for (size_t slot = 0; slot < kChurnPortIdxs.size(); ++slot) {
      utility::bindAccessPolicyPort(
          config,
          this->shape(),
          portIds[kChurnPortIdxs[slot]],
          churnClass(slot, iteration));
    }
    this->applyNewConfig(config);
  }

  void runPortClassChurnTest() {
    auto setup = [this]() { this->programRouteToEgressPort(); };
    auto verify = [this]() {
      this->applyClassAssignment(0);
      this->verifyAccessPolicy(true /*accessPolicyProgrammed*/, {});
      for (int i = 1; i <= FLAGS_access_policy_stress_iterations; ++i) {
        this->applyClassAssignment(i);
      }
      this->applyClassAssignment(0);
      this->verifyAccessPolicy(true /*accessPolicyProgrammed*/, {});
    };
    this->verifyAcrossWarmBoots(setup, verify);
  }
};

using AgentAccessPolicyClassIdStressTest =
    AgentAccessPolicyStressTest<AgentAccessPolicyClassIdAclTest>;
using AgentAccessPolicyPortBoundStressTest =
    AgentAccessPolicyStressTest<AgentAccessPolicyPortBoundAclTest>;

TEST_F(AgentAccessPolicyClassIdStressTest, PortClassChurn) {
  runPortClassChurnTest();
}

TEST_F(AgentAccessPolicyPortBoundStressTest, PortClassChurn) {
  runPortClassChurnTest();
}

} // namespace facebook::fboss
