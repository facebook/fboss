// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <gflags/gflags.h>

#include <array>

#include "fboss/agent/hw/gen-cpp2/hardware_stats_constants.h"
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
// Coprime with the ACL churn stride so the spot checks sweep every omit
// window rather than revisiting half of them.
constexpr int kVerifyEveryIterations = 11;

} // namespace

template <typename BaseT>
class AgentAccessPolicyStressTest : public BaseT {
 protected:
  std::optional<size_t> maxRequiredInterfacePorts() const override {
    return kChurnRequiredInterfacePorts;
  }

  struct AclResourceFree {
    int32_t entries{0};
    int32_t counters{0};
    bool valid{false};
  };

  AclResourceFree aclResourceFree() {
    AclResourceFree free{.valid = true};
    auto allStats = this->getAllHwSwitchStats();
    if (allStats.empty()) {
      return {};
    }
    for (const auto& [_, stats] : allStats) {
      const auto& resources = *stats.hwResourceStats();
      // STAT_UNINITIALIZED is what a platform that does not report these
      // reads back, and comparing it against itself would pass vacuously.
      if (*resources.hw_table_stats_stale() ||
          *resources.acl_entries_free() ==
              hardware_stats_constants::STAT_UNINITIALIZED() ||
          *resources.acl_counters_free() ==
              hardware_stats_constants::STAT_UNINITIALIZED()) {
        return {};
      }
      free.entries += *resources.acl_entries_free();
      free.counters += *resources.acl_counters_free();
    }
    return free;
  }

  void expectNoAclResourceLeak(const AclResourceFree& before) {
    ASSERT_TRUE(before.valid) << "platform did not report acl resource stats";
    AclResourceFree after;
    WITH_RETRIES({
      after = aclResourceFree();
      EXPECT_EVENTUALLY_TRUE(after.valid);
      EXPECT_EVENTUALLY_EQ(after.entries, before.entries);
      EXPECT_EVENTUALLY_EQ(after.counters, before.counters);
    });
    XLOG(INFO) << "acl free entries " << before.entries << " -> "
               << after.entries << ", counters " << before.counters << " -> "
               << after.counters;
  }

  void spotCheck(
      int ingressPortIdx,
      cfg::AclLookupClassPort lookupClass,
      const std::set<std::string>& omitRules) {
    auto ingressPort = this->masterLogicalInterfacePortIds()[ingressPortIdx];
    SCOPED_TRACE(
        fmt::format(
            "spot check port index {} class {}",
            ingressPortIdx,
            apache::thrift::util::enumNameSafe(lookupClass)));
    const utility::AccessPolicyProbe* permit = nullptr;
    const utility::AccessPolicyProbe* deny = nullptr;
    for (const auto& probe : utility::accessPolicyProbes()) {
      auto outcome = BaseT::probeOutcome(
          probe, lookupClass, true /*accessPolicyProgrammed*/, omitRules);
      auto& slot = outcome.permit ? permit : deny;
      if (!slot) {
        slot = &probe;
      }
    }
    for (const auto* probe : {permit, deny}) {
      if (probe) {
        this->verifyProbe(
            *probe,
            ingressPort,
            lookupClass,
            true /*accessPolicyProgrammed*/,
            omitRules);
      }
    }
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

  AclResourceFree waitForAclResourceFree() {
    AclResourceFree free;
    WITH_RETRIES({
      free = aclResourceFree();
      EXPECT_EVENTUALLY_TRUE(free.valid);
    });
    return free;
  }

  void runPortClassChurnTest() {
    auto setup = [this]() { this->programRouteToEgressPort(); };
    auto verify = [this]() {
      this->applyClassAssignment(0);
      auto before = this->waitForAclResourceFree();
      this->verifyAccessPolicy(true /*accessPolicyProgrammed*/, {});
      int checkpoint = 0;
      for (int i = 1; i <= FLAGS_access_policy_stress_iterations; ++i) {
        this->applyClassAssignment(i);
        if (i % kVerifyEveryIterations == 0) {
          auto slot = checkpoint++ % kChurnPortIdxs.size();
          this->spotCheck(kChurnPortIdxs[slot], churnClass(slot, i), {});
        }
      }
      this->applyClassAssignment(0);
      this->verifyAccessPolicy(true /*accessPolicyProgrammed*/, {});
      this->expectNoAclResourceLeak(before);
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
