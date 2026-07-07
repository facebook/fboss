// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

/**
 * Integration tests for:
 *   fboss2-dev config qos default-policy <name>  (set)
 *   fboss2-dev delete qos default-policy  (clear)
 *
 * Both commands operate on sw.dataPlaneTrafficPolicy.defaultQosPolicy.
 * Tests derive policy names and restore values from getRunningConfig() so
 * they work portably across DUTs.
 */

#include <folly/json/dynamic.h>
#include <folly/logging/xlog.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <optional>
#include <string>
#include "fboss/cli/fboss2/test/integration_test/Fboss2IntegrationTest.h"

using namespace facebook::fboss;

class ConfigQosDefaultPolicyTest : public Fboss2IntegrationTest {
 protected:
  std::optional<std::string> originalDefaultPolicy_;
  std::optional<std::string> targetPolicyName_;

  void SetUp() override {
    Fboss2IntegrationTest::SetUp();

    auto config = getRunningConfig();
    captureState(config);
  }

  void TearDown() override {
    restoreOriginalPolicy();
    Fboss2IntegrationTest::TearDown();
  }

  void captureState(const folly::dynamic& config) {
    // Capture original defaultQosPolicy (may be absent)
    if (config.isObject() && config.count("sw")) {
      const auto& sw = config["sw"];
      if (sw.isObject() && sw.count("dataPlaneTrafficPolicy")) {
        const auto& policy = sw["dataPlaneTrafficPolicy"];
        if (policy.isObject() && policy.count("defaultQosPolicy")) {
          originalDefaultPolicy_ = policy["defaultQosPolicy"].asString();
        }
      }

      // Pick a policy name from qosPolicies to use as the test target
      if (sw.isObject() && sw.count("qosPolicies")) {
        const auto& policies = sw["qosPolicies"];
        if (policies.isArray() && !policies.empty()) {
          for (const auto& p : policies) {
            if (p.isObject() && p.count("name")) {
              targetPolicyName_ = p["name"].asString();
              break;
            }
          }
        }
      }
    }

    if (!targetPolicyName_.has_value()) {
      GTEST_SKIP() << "No QoS policies in running config; skipping test";
    }
  }

  void restoreOriginalPolicy() {
    if (!targetPolicyName_.has_value()) {
      return;
    }
    discardSession();
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
  XLOG(INFO) << "Setting default QoS policy to: " << *targetPolicyName_;

  auto result = runCli({"config", "qos", "default-policy", *targetPolicyName_});
  ASSERT_EQ(result.exitCode, 0) << "CLI failed: " << result.stderr;
  EXPECT_THAT(result.stdout, ::testing::HasSubstr("Successfully set"));

  commitConfig();

  auto afterConfig = getRunningConfig();
  ASSERT_TRUE(afterConfig.isObject() && afterConfig.count("sw"));
  const auto& sw = afterConfig["sw"];
  ASSERT_TRUE(sw.isObject() && sw.count("dataPlaneTrafficPolicy"));
  const auto& policy = sw["dataPlaneTrafficPolicy"];
  ASSERT_TRUE(policy.isObject() && policy.count("defaultQosPolicy"));
  EXPECT_EQ(policy["defaultQosPolicy"].asString(), *targetPolicyName_);
}

TEST_F(ConfigQosDefaultPolicyTest, DeleteAndVerifyDefaultPolicy) {
  // First set a policy so there's something to delete
  XLOG(INFO) << "Setting default QoS policy to: " << *targetPolicyName_;
  auto setResult =
      runCli({"config", "qos", "default-policy", *targetPolicyName_});
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
