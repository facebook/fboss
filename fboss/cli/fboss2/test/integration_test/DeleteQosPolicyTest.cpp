// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

/**
 * End-to-end test for `delete qos policy`:
 *
 *   - delete qos policy <name> map dscp <value>    — drops a codepoint from
 *     sw.qosPolicies[*].qosMap.dscpMaps
 *   - delete qos policy <name> map tc-to-queue <tc> — drops a key from
 *     sw.qosPolicies[*].qosMap.trafficClassToQueueId
 *   - delete qos policy <name>                     — removes the policy
 *
 * The three run in one test because each is the inverse of a step that set the
 * policy up: the scratch policy is created here, dismantled entry by entry,
 * then removed, so nothing is left behind and the test does not depend on DUT
 * state. A stock switch has no qosPolicies at all.
 *
 * The refusal path (deleting a policy still named by dataPlaneTrafficPolicy or
 * cpuTrafficPolicy.trafficPolicy) is not covered here: no CLI command writes
 * defaultQosPolicy or portIdToQosPolicy, so a reference cannot be created from
 * the command line. That path is covered by CmdDeleteQosPolicyTest, which
 * seeds the reference directly into the config.
 *
 * QoS map edits commit at HITLESS, so the agents do not restart; post-commit
 * state is still read through waitForRunningConfig() to avoid racing the
 * config reload.
 */

#include <folly/json/dynamic.h>
#include <folly/logging/xlog.h>
#include <gtest/gtest.h>
#include <string>

#include "fboss/cli/fboss2/test/integration_test/Fboss2IntegrationTest.h"

using namespace facebook::fboss;

class DeleteQosPolicyTest : public Fboss2IntegrationTest {
 protected:
  // Name used for the scratch policy this test creates and removes.
  static constexpr auto kPolicy = "fboss2-it-scratch-qos";
  static constexpr auto kDscp = "40";
  static constexpr auto kTrafficClass = "5";
  static constexpr auto kQueueId = "0";

  static const folly::dynamic* findPolicy(
      const folly::dynamic& config,
      const std::string& name) {
    if (!config.count("sw") || !config["sw"].count("qosPolicies")) {
      return nullptr;
    }
    for (const auto& policy : config["sw"]["qosPolicies"]) {
      if (policy.count("name") && policy["name"].asString() == name) {
        return &policy;
      }
    }
    return nullptr;
  }

  static bool hasPolicy(const folly::dynamic& config, const std::string& name) {
    return findPolicy(config, name) != nullptr;
  }

  // True when the policy maps `dscp` to some traffic class.
  static bool
  hasDscp(const folly::dynamic& config, const std::string& name, int dscp) {
    const auto* policy = findPolicy(config, name);
    if (policy == nullptr || !policy->count("qosMap") ||
        !(*policy)["qosMap"].count("dscpMaps")) {
      return false;
    }
    for (const auto& entry : (*policy)["qosMap"]["dscpMaps"]) {
      if (!entry.count("fromDscpToTrafficClass")) {
        continue;
      }
      for (const auto& value : entry["fromDscpToTrafficClass"]) {
        if (value.asInt() == dscp) {
          return true;
        }
      }
    }
    return false;
  }

  // True when the policy maps traffic class `tc` to a queue.
  static bool hasTcToQueue(
      const folly::dynamic& config,
      const std::string& name,
      const std::string& tc) {
    const auto* policy = findPolicy(config, name);
    if (policy == nullptr || !policy->count("qosMap") ||
        !(*policy)["qosMap"].count("trafficClassToQueueId")) {
      return false;
    }
    return (*policy)["qosMap"]["trafficClassToQueueId"].count(tc) > 0;
  }
};

TEST_F(DeleteQosPolicyTest, DeleteMapEntriesThenPolicy) {
  // 1. Create a scratch policy carrying one dscp mapping and one tc-to-queue
  //    mapping. `config qos policy <name> map ...` creates the policy when it
  //    does not already exist.
  XLOG(INFO) << "Creating qos policy " << kPolicy;
  auto result = runCli(
      {"config",
       "qos",
       "policy",
       kPolicy,
       "map",
       "dscp",
       kDscp,
       "traffic-class",
       kTrafficClass});
  ASSERT_EQ(result.exitCode, 0)
      << "stdout=" << result.stdout << " stderr=" << result.stderr;

  result = runCli(
      {"config",
       "qos",
       "policy",
       kPolicy,
       "map",
       "tc-to-queue",
       kTrafficClass,
       kQueueId});
  ASSERT_EQ(result.exitCode, 0)
      << "stdout=" << result.stdout << " stderr=" << result.stderr;
  commitConfig();

  auto config = waitForRunningConfig([&](const folly::dynamic& cfg) {
    return hasPolicy(cfg, kPolicy) && hasDscp(cfg, kPolicy, std::stoi(kDscp)) &&
        hasTcToQueue(cfg, kPolicy, kTrafficClass);
  });
  ASSERT_TRUE(hasPolicy(config, kPolicy))
      << "qos policy " << kPolicy << " missing after commit";
  ASSERT_TRUE(hasDscp(config, kPolicy, std::stoi(kDscp)));
  ASSERT_TRUE(hasTcToQueue(config, kPolicy, kTrafficClass));

  // 2. Delete the dscp mapping. The tc-to-queue mapping must survive.
  XLOG(INFO) << "Deleting dscp " << kDscp << " from " << kPolicy;
  result = runCli({"delete", "qos", "policy", kPolicy, "map", "dscp", kDscp});
  ASSERT_EQ(result.exitCode, 0)
      << "stdout=" << result.stdout << " stderr=" << result.stderr;
  commitConfig();
  config = waitForRunningConfig([&](const folly::dynamic& cfg) {
    return !hasDscp(cfg, kPolicy, std::stoi(kDscp));
  });
  EXPECT_FALSE(hasDscp(config, kPolicy, std::stoi(kDscp)));
  EXPECT_TRUE(hasTcToQueue(config, kPolicy, kTrafficClass))
      << "deleting the dscp mapping must not touch trafficClassToQueueId";

  // 3. Delete the tc-to-queue mapping.
  XLOG(INFO) << "Deleting tc-to-queue " << kTrafficClass << " from " << kPolicy;
  result = runCli(
      {"delete",
       "qos",
       "policy",
       kPolicy,
       "map",
       "tc-to-queue",
       kTrafficClass});
  ASSERT_EQ(result.exitCode, 0)
      << "stdout=" << result.stdout << " stderr=" << result.stderr;
  commitConfig();
  config = waitForRunningConfig([&](const folly::dynamic& cfg) {
    return !hasTcToQueue(cfg, kPolicy, kTrafficClass);
  });
  EXPECT_FALSE(hasTcToQueue(config, kPolicy, kTrafficClass));

  // 4. Delete the policy itself. Nothing references it, so this is allowed.
  XLOG(INFO) << "Deleting qos policy " << kPolicy;
  result = runCli({"delete", "qos", "policy", kPolicy});
  ASSERT_EQ(result.exitCode, 0)
      << "stdout=" << result.stdout << " stderr=" << result.stderr;
  commitConfig();
  config = waitForRunningConfig(
      [&](const folly::dynamic& cfg) { return !hasPolicy(cfg, kPolicy); });
  EXPECT_FALSE(hasPolicy(config, kPolicy));
}
