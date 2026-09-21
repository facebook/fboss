// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

/**
 * End-to-end test for:
 *   fboss2-dev config l2 learning-mode <hardware|software|disabled>
 *
 * Exercises get / set / verify across two transitions and restores the
 * original mode. Each commit triggers a coldboot, so the test waits for
 * agent recovery between steps. Complementary to
 * ConfigSessionRestartTest::CommitTriggersColdboot which asserts the
 * restart *signal*; this test asserts the *value* round-trips through
 * the agent's running config.
 */

#include <folly/json/dynamic.h>
#include <folly/logging/xlog.h>
#include <gtest/gtest.h>
#include <string>
#include "fboss/cli/fboss2/test/integration_test/Fboss2IntegrationTest.h"

using namespace facebook::fboss;

namespace {
// L2LearningMode enum values from switch_config.thrift
constexpr int kHardware = 0;
constexpr int kSoftware = 1;

std::string modeToStr(int mode) {
  switch (mode) {
    case 0:
      return "hardware";
    case 1:
      return "software";
    case 2:
      return "disabled";
    default:
      return "unknown";
  }
}
} // namespace

class ConfigL2LearningModeTest : public Fboss2IntegrationTest {
 protected:
  int getLearningMode() const {
    auto config = getRunningConfig();
    const auto& sw = config["sw"];
    if (!sw.count("switchSettings")) {
      return kHardware; // default
    }
    const auto& settings = sw["switchSettings"];
    if (!settings.count("l2LearningMode")) {
      return kHardware;
    }
    return settings["l2LearningMode"].asInt();
  }

  void setLearningMode(const std::string& mode) {
    auto result = runCli({"config", "l2", "learning-mode", mode});
    ASSERT_EQ(result.exitCode, 0)
        << "learning-mode CLI failed: " << result.stderr;
    commitConfig();
    waitForAgentReady();
  }
};

TEST_F(ConfigL2LearningModeTest, TransitionBetweenHardwareAndSoftware) {
  XLOG(INFO) << "[Step 1] Reading current L2 learning mode...";
  int originalMode = getLearningMode();
  XLOG(INFO) << "  Current: " << modeToStr(originalMode);

  std::string first = (originalMode == kHardware) ? "software" : "hardware";
  std::string second = (first == "software") ? "hardware" : "software";
  int firstInt = (first == "software") ? kSoftware : kHardware;
  int secondInt = (second == "software") ? kSoftware : kHardware;

  XLOG(INFO) << "[Step 2] Setting mode to '" << first << "'...";
  setLearningMode(first);
  EXPECT_EQ(getLearningMode(), firstInt);

  XLOG(INFO) << "[Step 3] Setting mode to '" << second << "'...";
  setLearningMode(second);
  EXPECT_EQ(getLearningMode(), secondInt);

  if (originalMode != secondInt) {
    XLOG(INFO) << "[Cleanup] Restoring original mode '"
               << modeToStr(originalMode) << "'...";
    setLearningMode(modeToStr(originalMode));
  }

  XLOG(INFO) << "TEST PASSED";
}
