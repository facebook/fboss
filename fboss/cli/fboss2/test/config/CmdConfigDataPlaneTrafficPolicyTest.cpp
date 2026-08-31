/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <stdexcept>
#include <string>
#include <vector>

#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/cli/fboss2/commands/config/TrafficPolicyUtils.h"
#include "fboss/cli/fboss2/commands/config/data_plane/traffic_policy/CmdConfigDataPlaneTrafficPolicy.h"
#include "fboss/cli/fboss2/session/ConfigSession.h"
#include "fboss/cli/fboss2/test/config/CmdConfigTestBase.h"
#include "fboss/cli/fboss2/utils/HostInfo.h"

using namespace ::testing;

namespace facebook::fboss {

// One ACL table holding the rule the actions attach to, and nothing under
// either traffic policy, so each test starts from an empty matchToAction.
static constexpr auto kSeedConfig = R"({
  "sw": {
    "aclTableGroups": [{
      "name": "ingress-ACL-Table-Group",
      "stage": 0,
      "aclTables": [{
        "name": "AclTable1",
        "priority": 0,
        "aclEntries": [
          {"name": "queue-per-host-queue-0-l2", "actionType": 1}
        ],
        "actionTypes": [],
        "qualifiers": [],
        "udfGroups": []
      }]
    }]
  }
})";

class CmdConfigDataPlaneTrafficPolicyTestFixture : public CmdConfigTestBase {
 public:
  CmdConfigDataPlaneTrafficPolicyTestFixture()
      : CmdConfigTestBase(
            "fboss_dp_traffic_policy_test_%%%%-%%%%-%%%%-%%%%",
            kSeedConfig) {}

 protected:
  const std::string cmdPrefix_ = "config data-plane traffic-policy";
  const std::string kRule = "queue-per-host-queue-0-l2";

  cfg::MatchAction& dataPlaneAction(const std::string& matcher) {
    auto& cfg = ConfigSession::getInstance().getAgentConfig();
    auto policy = cfg.sw()->dataPlaneTrafficPolicy();
    if (!policy) {
      throw std::runtime_error("dataPlaneTrafficPolicy not set");
    }
    for (auto& mta : *policy->matchToAction()) {
      if (*mta.matcher() == matcher) {
        return *mta.action();
      }
    }
    throw std::runtime_error("no matchToAction for " + matcher);
  }

  std::string apply(const std::vector<std::string>& actionTokens) {
    std::vector<std::string> argv{"match", kRule, "action"};
    argv.insert(argv.end(), actionTokens.begin(), actionTokens.end());
    setupTestableConfigSession(cmdPrefix_, "");
    CmdConfigDataPlaneTrafficPolicy cmd;
    HostInfo host("testhost");
    traffic_policy::TrafficPolicyArgs args(argv);
    return cmd.queryClient(host, args);
  }
};

// Real-world queue-per-host example: a lookup-class match paired with a
// send-to-queue action on the dataplane policy.
TEST_F(CmdConfigDataPlaneTrafficPolicyTestFixture, setSendToQueue) {
  apply({"send-to-queue", "0"});
  EXPECT_EQ(*dataPlaneAction(kRule).sendToQueue()->queueId(), 0);
}

TEST_F(CmdConfigDataPlaneTrafficPolicyTestFixture, setCounter) {
  apply({"counter", "queue-per-host-queue-0-l2"});
  EXPECT_EQ(*dataPlaneAction(kRule).counter(), "queue-per-host-queue-0-l2");
}

TEST_F(CmdConfigDataPlaneTrafficPolicyTestFixture, setTcAcceptsNine) {
  // Shipped CPU policies use tc 9, so the acl-rule side's 0-7 cap would be
  // wrong here; the thrift byte range is what applies.
  apply({"set-tc", "9"});
  EXPECT_EQ(*dataPlaneAction(kRule).setTc()->tcValue(), 9);
}

TEST_F(CmdConfigDataPlaneTrafficPolicyTestFixture, setValuelessAction) {
  apply({"trap-to-cpu"});
  EXPECT_EQ(*dataPlaneAction(kRule).toCpuAction(), cfg::ToCpuAction::TRAP);
}

TEST_F(CmdConfigDataPlaneTrafficPolicyTestFixture, setRedirectNexthop) {
  apply({"redirect", "nexthop", "2401:db00::1"});
  const auto& hops =
      *dataPlaneAction(kRule).redirectToNextHop()->redirectNextHops();
  ASSERT_EQ(hops.size(), 1u);
  EXPECT_EQ(*hops[0].ip(), "2401:db00::1");
}

TEST_F(
    CmdConfigDataPlaneTrafficPolicyTestFixture,
    actionsAccumulateOnOneMatcher) {
  apply({"send-to-queue", "0"});
  apply({"counter", "c1"});
  auto& action = dataPlaneAction(kRule);
  EXPECT_EQ(*action.sendToQueue()->queueId(), 0);
  EXPECT_EQ(*action.counter(), "c1");

  auto& policy = *ConfigSession::getInstance()
                      .getAgentConfig()
                      .sw()
                      ->dataPlaneTrafficPolicy();
  EXPECT_EQ(policy.matchToAction()->size(), 1u);
}

TEST_F(CmdConfigDataPlaneTrafficPolicyTestFixture, rejectsUnknownAction) {
  EXPECT_THROW(apply({"drop", "1"}), std::invalid_argument);
}

// The delete command tree has a literal "action" node between the matcher
// name and the action type, so a rule named "action" would be configurable
// here but impossible to target for delete.
TEST_F(CmdConfigDataPlaneTrafficPolicyTestFixture, rejectsRuleNamedAction) {
  EXPECT_THROW(
      traffic_policy::TrafficPolicyArgs(
          {"match", "action", "action", "counter", "c1"}),
      std::invalid_argument);
}

TEST_F(CmdConfigDataPlaneTrafficPolicyTestFixture, rejectsWrongArity) {
  EXPECT_THROW(apply({"send-to-queue"}), std::invalid_argument);
  EXPECT_THROW(apply({"trap-to-cpu", "1"}), std::invalid_argument);
  EXPECT_THROW(apply({"redirect", "nexthop"}), std::invalid_argument);
}

TEST_F(
    CmdConfigDataPlaneTrafficPolicyTestFixture,
    rejectsRedirectWithoutKeyword) {
  EXPECT_THROW(
      apply({"redirect", "gateway", "1.1.1.1"}), std::invalid_argument);
}

// updateAclsImpl resolves a rule to the CPU policy first and only falls back
// to the dataplane one, so a rule in both would have its dataplane actions
// silently ignored.
TEST_F(
    CmdConfigDataPlaneTrafficPolicyTestFixture,
    refusesRuleAlreadyInCoppPolicy) {
  setupTestableConfigSession(cmdPrefix_, "");
  auto& swConfig = *ConfigSession::getInstance().getAgentConfig().sw();
  traffic_policy::applyAction(
      swConfig, traffic_policy::PolicyKind::Cpu, kRule, {"send-to-queue", "9"});

  EXPECT_THROW(
      traffic_policy::applyAction(
          swConfig,
          traffic_policy::PolicyKind::DataPlane,
          kRule,
          {"send-to-queue", "0"}),
      std::runtime_error);
}

TEST_F(
    CmdConfigDataPlaneTrafficPolicyTestFixture,
    deleteRemovesOneActionThenTheEntry) {
  setupTestableConfigSession(cmdPrefix_, "");
  auto& swConfig = *ConfigSession::getInstance().getAgentConfig().sw();
  const auto kind = traffic_policy::PolicyKind::DataPlane;
  traffic_policy::applyAction(swConfig, kind, kRule, {"send-to-queue", "0"});
  traffic_policy::applyAction(swConfig, kind, kRule, {"counter", "c1"});

  traffic_policy::deleteAction(swConfig, kind, kRule, "counter");
  EXPECT_FALSE(dataPlaneAction(kRule).counter().has_value());
  EXPECT_TRUE(dataPlaneAction(kRule).sendToQueue().has_value());

  // Dropping the last action drops the matchToAction entry with it, so no
  // empty entry is left holding checkTrafficPolicyAclsExistInConfig hostage.
  traffic_policy::deleteAction(swConfig, kind, kRule, "send-to-queue");
  EXPECT_TRUE(swConfig.dataPlaneTrafficPolicy()->matchToAction()->empty());
}

TEST_F(
    CmdConfigDataPlaneTrafficPolicyTestFixture,
    deleteAbsentActionIsReported) {
  setupTestableConfigSession(cmdPrefix_, "");
  auto& swConfig = *ConfigSession::getInstance().getAgentConfig().sw();
  const auto kind = traffic_policy::PolicyKind::DataPlane;
  traffic_policy::applyAction(swConfig, kind, kRule, {"send-to-queue", "0"});

  auto msg = traffic_policy::deleteAction(swConfig, kind, kRule, "counter");
  EXPECT_THAT(msg, HasSubstr("already absent"));
}

// A matchToAction entry can pre-exist with no actions set at all (e.g. from a
// hand-edited config). Deleting an already-absent action from it should still
// prune the empty entry, not just report the action as absent.
TEST_F(
    CmdConfigDataPlaneTrafficPolicyTestFixture,
    deleteAbsentActionPrunesPreExistingEmptyEntry) {
  setupTestableConfigSession(cmdPrefix_, "");
  auto& swConfig = *ConfigSession::getInstance().getAgentConfig().sw();
  const auto kind = traffic_policy::PolicyKind::DataPlane;
  traffic_policy::upsertMatcher(
      traffic_policy::policyFor(swConfig, kind), kRule);
  ASSERT_FALSE(swConfig.dataPlaneTrafficPolicy()->matchToAction()->empty());

  auto msg = traffic_policy::deleteAction(swConfig, kind, kRule, "counter");
  EXPECT_THAT(msg, HasSubstr("already absent"));
  EXPECT_TRUE(swConfig.dataPlaneTrafficPolicy()->matchToAction()->empty());
}

} // namespace facebook::fboss
