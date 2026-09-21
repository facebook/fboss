// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

/**
 * End-to-end test for:
 *   fboss2-dev config ptp transparent-clock <enable|disable>
 *
 * Reads the current ptpTcEnable value, toggles it twice, and restores the
 * original state. The change is HITLESS so no agent restart is needed between
 * steps.
 */

#include <folly/json/dynamic.h>
#include <folly/logging/xlog.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <string>
#include "fboss/cli/fboss2/test/integration_test/Fboss2IntegrationTest.h"

using namespace facebook::fboss;

class ConfigPtpTest : public Fboss2IntegrationTest {
 protected:
  bool getPtpTcEnable() const {
    auto config = getRunningConfig();
    const auto& sw = config["sw"];
    if (!sw.count("switchSettings")) {
      ADD_FAILURE() << "switchSettings key absent from running config";
      return false;
    }
    const auto& settings = sw["switchSettings"];
    if (!settings.count("ptpTcEnable")) {
      ADD_FAILURE() << "ptpTcEnable key absent from running config";
      return false;
    }
    return settings["ptpTcEnable"].asBool();
  }

  void setPtpTcEnable(bool enable) {
    auto result = runCli(
        {"config", "ptp", "transparent-clock", enable ? "enable" : "disable"});
    ASSERT_EQ(result.exitCode, 0)
        << "transparent-clock CLI failed: " << result.stderr;
    commitConfig();
  }
};

TEST_F(ConfigPtpTest, ToggleTransparentClock) {
  XLOG(INFO) << "[Step 1] Reading current PTP transparent clock state...";
  bool originalValue = getPtpTcEnable();
  XLOG(INFO) << "  Current: " << (originalValue ? "enabled" : "disabled");

  bool firstValue = !originalValue;
  bool secondValue = originalValue;

  XLOG(INFO) << "[Step 2] Setting transparent-clock to '"
             << (firstValue ? "enable" : "disable") << "'...";
  setPtpTcEnable(firstValue);
  EXPECT_EQ(getPtpTcEnable(), firstValue);

  XLOG(INFO) << "[Step 3] Restoring to '"
             << (secondValue ? "enable" : "disable") << "'...";
  setPtpTcEnable(secondValue);
  EXPECT_EQ(getPtpTcEnable(), secondValue);

  XLOG(INFO) << "TEST PASSED";
}

TEST_F(ConfigPtpTest, IdempotentToggle) {
  bool originalValue = getPtpTcEnable();

  // Toggle to the opposite value
  setPtpTcEnable(!originalValue);
  ASSERT_EQ(getPtpTcEnable(), !originalValue);

  // Set the same value again — should be a no-op returning "already ..."
  // message
  std::string state = !originalValue ? "enable" : "disable";
  auto result = runCli({"config", "ptp", "transparent-clock", state});
  ASSERT_EQ(result.exitCode, 0) << result.stderr;
  EXPECT_THAT(result.stdout, ::testing::HasSubstr("already"));
  EXPECT_EQ(getPtpTcEnable(), !originalValue);

  // Restore original state
  setPtpTcEnable(originalValue);
}
