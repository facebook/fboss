// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

/**
 * End-to-end tests for:
 *   fboss2-dev config mac aging-time <seconds>
 *
 * Reads the current l2AgeTimerSeconds, applies CLI changes, verifies that
 * each new value round-trips through the agent's running config, then
 * restores the original.  All commits are HITLESS (no agent restart between
 * steps), unlike l2 learning-mode changes which require a coldboot.
 */

#include <folly/json/dynamic.h>
#include <folly/logging/xlog.h>
#include <gtest/gtest.h>
#include <string>
#include "fboss/cli/fboss2/test/integration_test/Fboss2IntegrationTest.h"

using namespace facebook::fboss;

namespace {
constexpr int64_t kDefaultAgingSeconds = 300;
} // namespace

class ConfigMacTest : public Fboss2IntegrationTest {
 protected:
  // Read l2AgeTimerSeconds from the agent's running config.  Returns
  // kDefaultAgingSeconds when the field is absent (agent default).
  int64_t getAgingTime() const {
    auto config = getRunningConfig();
    const auto& sw = config["sw"];
    if (!sw.count("switchSettings")) {
      return kDefaultAgingSeconds;
    }
    const auto& settings = sw["switchSettings"];
    if (!settings.count("l2AgeTimerSeconds")) {
      return kDefaultAgingSeconds;
    }
    return settings["l2AgeTimerSeconds"].asInt();
  }

  // Issue the CLI command and commit the session.  No waitForAgentReady()
  // is needed because aging-time changes are applied hitlessly by the SAI
  // layer (SaiSwitch::setMacAgingSeconds, no ChangeProhibited guard).
  void setAgingTime(int64_t seconds) {
    auto result =
        runCli({"config", "mac", "aging-time", std::to_string(seconds)});
    ASSERT_EQ(result.exitCode, 0) << "aging-time CLI failed: " << result.stderr;
    commitConfig();
  }
};

// Verify that aging-time can be updated and that the new value round-trips
// through the agent.  A different target is chosen depending on the current
// value so the test is always exercising a real write.
TEST_F(ConfigMacTest, SetAndRestoreAgingTime) {
  XLOG(INFO) << "[Step 1] Reading current MAC aging time...";
  int64_t originalSeconds = getAgingTime();
  XLOG(INFO) << "  Current: " << originalSeconds << "s";

  // Pick a target distinct from the current value.
  int64_t newSeconds = (originalSeconds == 600) ? 300 : 600;

  XLOG(INFO) << "[Step 2] Setting aging-time to " << newSeconds << "s...";
  setAgingTime(newSeconds);
  EXPECT_EQ(getAgingTime(), newSeconds);

  XLOG(INFO) << "[Step 3] Restoring original aging-time " << originalSeconds
             << "s...";
  setAgingTime(originalSeconds);
  EXPECT_EQ(getAgingTime(), originalSeconds);

  XLOG(INFO) << "TEST PASSED";
}

// Verify the minimum accepted value (1 second) is written and visible in
// the running config, then restore the original value.
TEST_F(ConfigMacTest, SetMinimumAgingTime) {
  XLOG(INFO) << "[Step 1] Reading current MAC aging time...";
  int64_t originalSeconds = getAgingTime();
  XLOG(INFO) << "  Current: " << originalSeconds << "s";

  int64_t targetSeconds = (originalSeconds == 1) ? 2 : 1;

  XLOG(INFO) << "[Step 2] Setting aging-time to " << targetSeconds << "s...";
  setAgingTime(targetSeconds);
  EXPECT_EQ(getAgingTime(), targetSeconds);

  XLOG(INFO) << "[Step 3] Restoring original aging-time " << originalSeconds
             << "s...";
  setAgingTime(originalSeconds);
  EXPECT_EQ(getAgingTime(), originalSeconds);

  XLOG(INFO) << "TEST PASSED";
}
