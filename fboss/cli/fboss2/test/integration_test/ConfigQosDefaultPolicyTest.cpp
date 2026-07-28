// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

/**
 * Integration tests for:
 *   fboss2-dev config qos default-policy <name>  (set)
 *   fboss2-dev delete qos default-policy  (clear)
 *
 * Both commands operate on sw.dataPlaneTrafficPolicy.defaultQosPolicy.
 *
 * Each test creates its own uniquely named QoS policy in the config session
 * (via `config qos policy <name> map tc-to-queue`) and uses that as the
 * default, so the tests run on any DUT config — including one with no QoS
 * policies at all. The original defaultQosPolicy is captured from the running
 * config and restored in TearDown. The test policy itself may remain in the
 * config afterwards (there is no delete verb for QoS policies yet); its
 * timestamped name keeps it from conflicting with anything.
 */

#include <fmt/format.h>
#include <folly/json/dynamic.h>
#include <folly/logging/xlog.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <chrono>
#include <optional>
#include <string>
#include "fboss/cli/fboss2/test/integration_test/Fboss2IntegrationTest.h"

using namespace facebook::fboss;

namespace {
// Unique per run so stale policies from previous runs never conflict.
std::string generateTestPolicyName() {
  auto epochMs = std::chrono::duration_cast<std::chrono::milliseconds>(
                     std::chrono::system_clock::now().time_since_epoch())
                     .count();
  return fmt::format("cli_e2e_default_policy_test_{}", epochMs);
}
} // namespace

class ConfigQosDefaultPolicyTest : public Fboss2IntegrationTest {
 protected:
  std::optional<std::string> originalDefaultPolicy_;
  std::string testPolicyName_ = generateTestPolicyName();

  void SetUp() override {
    Fboss2IntegrationTest::SetUp();
    captureOriginalDefaultPolicy(getRunningConfig());
    createTestPolicy();
  }

  void TearDown() override {
    restoreOriginalPolicy();
    Fboss2IntegrationTest::TearDown();
  }

  // Returns sw.dataPlaneTrafficPolicy.defaultQosPolicy, or nullopt if absent.
  static std::optional<std::string> readDefaultPolicy(
      const folly::dynamic& config) {
    if (config.isObject() && config.count("sw")) {
      const auto& sw = config["sw"];
      if (sw.isObject() && sw.count("dataPlaneTrafficPolicy")) {
        const auto& policy = sw["dataPlaneTrafficPolicy"];
        if (policy.isObject() && policy.count("defaultQosPolicy")) {
          return policy["defaultQosPolicy"].asString();
        }
      }
    }
    return std::nullopt;
  }

  // Capture original defaultQosPolicy (may be absent) for restore.
  void captureOriginalDefaultPolicy(const folly::dynamic& config) {
    originalDefaultPolicy_ = readDefaultPolicy(config);
  }

  // Create the test QoS policy in the config session. A map subcommand
  // auto-creates the policy; default-policy validates names against the
  // session config, so no commit is needed before using it.
  void createTestPolicy() {
    XLOG(INFO) << "Creating test QoS policy: " << testPolicyName_;
    auto result = runCli(
        {"config",
         "qos",
         "policy",
         testPolicyName_,
         "map",
         "tc-to-queue",
         "0",
         "0"});
    ASSERT_EQ(result.exitCode, 0)
        << "Failed to create test QoS policy: " << result.stderr;
  }

  void restoreOriginalPolicy() {
    discardSession();
    // Skip when the committed state already matches (e.g. after the delete
    // test on a DUT that had no default policy): a no-op delete stages no
    // session change, and committing an empty session fails.
    if (readDefaultPolicy(getRunningConfig()) == originalDefaultPolicy_) {
      return;
    }
    Result result;
    if (originalDefaultPolicy_.has_value()) {
      result =
          runCli({"config", "qos", "default-policy", *originalDefaultPolicy_});
    } else {
      result = runCli({"delete", "qos", "default-policy"});
    }
    if (result.exitCode == 0) {
      commitConfig();
      waitForAgentReady();
    } else {
      XLOG(WARN) << "Failed to restore original default QoS policy: "
                 << result.stderr;
      discardSession();
    }
  }
};

TEST_F(ConfigQosDefaultPolicyTest, SetAndVerifyDefaultPolicy) {
  XLOG(INFO) << "Setting default QoS policy to: " << testPolicyName_;

  auto result = runCli({"config", "qos", "default-policy", testPolicyName_});
  ASSERT_EQ(result.exitCode, 0) << "CLI failed: " << result.stderr;
  EXPECT_THAT(result.stdout, ::testing::HasSubstr("Successfully set"));

  commitConfig();

  auto afterConfig = getRunningConfig();
  ASSERT_TRUE(afterConfig.isObject() && afterConfig.count("sw"));
  const auto& sw = afterConfig["sw"];
  ASSERT_TRUE(sw.isObject() && sw.count("dataPlaneTrafficPolicy"));
  const auto& policy = sw["dataPlaneTrafficPolicy"];
  ASSERT_TRUE(policy.isObject() && policy.count("defaultQosPolicy"));
  EXPECT_EQ(policy["defaultQosPolicy"].asString(), testPolicyName_);
}

TEST_F(ConfigQosDefaultPolicyTest, DeleteAndVerifyDefaultPolicy) {
  // First set a policy so there's something to delete
  XLOG(INFO) << "Setting default QoS policy to: " << testPolicyName_;
  auto setResult = runCli({"config", "qos", "default-policy", testPolicyName_});
  ASSERT_EQ(setResult.exitCode, 0) << "Set failed: " << setResult.stderr;
  commitConfig();

  // Now delete it
  auto delResult = runCli({"delete", "qos", "default-policy"});
  ASSERT_EQ(delResult.exitCode, 0) << "Delete failed: " << delResult.stderr;
  EXPECT_THAT(delResult.stdout, ::testing::HasSubstr("Successfully removed"));
  commitConfig();
  // delete commits at AGENT_COLDBOOT; wait for the restart before the next
  // thrift call, or getRunningConfig() below can hit connection-refused.
  waitForAgentReady();

  // Verify field is absent
  auto afterConfig = getRunningConfig();
  ASSERT_TRUE(afterConfig.isObject() && afterConfig.count("sw"));
  const auto& sw = afterConfig["sw"];
  bool policyAbsent = !sw.count("dataPlaneTrafficPolicy") ||
      !sw["dataPlaneTrafficPolicy"].count("defaultQosPolicy");
  EXPECT_TRUE(policyAbsent)
      << "defaultQosPolicy should be absent after delete-default-policy";
}
