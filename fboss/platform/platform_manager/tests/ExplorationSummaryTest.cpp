// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <unordered_map>

#include <fb303/ServiceData.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "fboss/platform/platform_manager/ExplorationSummary.h"

using namespace ::testing;
using namespace facebook::fboss::platform::platform_manager;

class MockExplorationSummaryTest : public ExplorationSummary {
 public:
  explicit MockExplorationSummaryTest(const PlatformConfig& config)
      : ExplorationSummary(config) {}
};

class ExplorationSummaryTest : public testing::Test {
 public:
  PlatformConfig platformConfig_;
  MockExplorationSummaryTest summary_{platformConfig_};
};

TEST_F(ExplorationSummaryTest, GoodExploration) {
  EXPECT_EQ(summary_.summarize({}, {}), ExplorationStatus::SUCCEEDED);
  EXPECT_EQ(
      facebook::fb303::fbData->getCounter(ExplorationSummary::kExplorationFail),
      0);
  EXPECT_EQ(
      facebook::fb303::fbData->getCounter(
          ExplorationSummary::kExplorationSucceeded),
      1);
  EXPECT_EQ(
      facebook::fb303::fbData->getCounter(
          ExplorationSummary::kExplorationSucceededWithExpectedErrors),
      0);
  EXPECT_EQ(
      facebook::fb303::fbData->getCounter(ExplorationSummary::kTotalFailures),
      0);
  EXPECT_EQ(
      facebook::fb303::fbData->getCounter(
          ExplorationSummary::kTotalExpectedFailures),
      0);
  for (const auto& errorType : explorationErrorTypeList()) {
    const auto key = fmt::format(
        ExplorationSummary::kExplorationFailByType,
        toExplorationErrorTypeStr(errorType));
    EXPECT_EQ(facebook::fb303::fbData->getCounter(key), 0);
  }
}

TEST_F(ExplorationSummaryTest, ExplorationWithExpectedErrors) {
  summary_.addError(
      ExplorationErrorType::SLOT_PM_UNIT_ABSENCE,
      "/SMB_SLOT@0/PSU_SLOT@0/[PSU_EEPROM]",
      "expect");
  summary_.addError(
      ExplorationErrorType::SLOT_PM_UNIT_ABSENCE,
      "/SMB_SLOT@1/PSU_SLOT@1/[PSU_EEPROM]",
      "expect");
  summary_.addError(
      ExplorationErrorType::SLOT_PM_UNIT_ABSENCE,
      "/PSU_SLOT@2/[PSU_EEPROM]",
      "expect");
  EXPECT_EQ(
      summary_.summarize({}, {}),
      ExplorationStatus::SUCCEEDED_WITH_EXPECTED_ERRORS);
  EXPECT_EQ(
      facebook::fb303::fbData->getCounter(ExplorationSummary::kExplorationFail),
      0);
  EXPECT_EQ(
      facebook::fb303::fbData->getCounter(
          ExplorationSummary::kExplorationSucceeded),
      0);
  EXPECT_EQ(
      facebook::fb303::fbData->getCounter(
          ExplorationSummary::kExplorationSucceededWithExpectedErrors),
      1);
  EXPECT_EQ(
      facebook::fb303::fbData->getCounter(ExplorationSummary::kTotalFailures),
      0);
  EXPECT_EQ(
      facebook::fb303::fbData->getCounter(
          ExplorationSummary::kTotalExpectedFailures),
      3);
  for (const auto& errorType : explorationErrorTypeList()) {
    const auto key = fmt::format(
        ExplorationSummary::kExplorationFailByType,
        toExplorationErrorTypeStr(errorType));
    EXPECT_EQ(facebook::fb303::fbData->getCounter(key), 0);
  }
}

TEST_F(ExplorationSummaryTest, ExplorationWithOnlyErrors) {
  summary_.addError(
      ExplorationErrorType::I2C_DEVICE_EXPLORE, "/[MCB_MUX_A]", "fail");
  summary_.addError(
      ExplorationErrorType::I2C_DEVICE_EXPLORE, "/[MCB_MUX_B]", "fail");
  summary_.addError(
      ExplorationErrorType::I2C_DEVICE_EXPLORE, "/[NVME]", "fail");
  EXPECT_EQ(summary_.summarize({}, {}), ExplorationStatus::FAILED);
  EXPECT_EQ(
      facebook::fb303::fbData->getCounter(ExplorationSummary::kExplorationFail),
      1);
  EXPECT_EQ(
      facebook::fb303::fbData->getCounter(
          ExplorationSummary::kExplorationSucceeded),
      0);
  EXPECT_EQ(
      facebook::fb303::fbData->getCounter(
          ExplorationSummary::kExplorationSucceededWithExpectedErrors),
      0);
  EXPECT_EQ(
      facebook::fb303::fbData->getCounter(ExplorationSummary::kTotalFailures),
      3);
  EXPECT_EQ(
      facebook::fb303::fbData->getCounter(
          ExplorationSummary::kTotalExpectedFailures),
      0);
  for (const auto& errorType : explorationErrorTypeList()) {
    const auto key = fmt::format(
        ExplorationSummary::kExplorationFailByType,
        toExplorationErrorTypeStr(errorType));
    if (errorType == ExplorationErrorType::I2C_DEVICE_EXPLORE) {
      EXPECT_EQ(facebook::fb303::fbData->getCounter(key), 3);
    } else {
      EXPECT_EQ(facebook::fb303::fbData->getCounter(key), 0);
    }
  }
}

TEST_F(ExplorationSummaryTest, ExplorationWithMixedErrors) {
  // Add expected errors (PSU slot devices)
  summary_.addError(
      ExplorationErrorType::SLOT_PM_UNIT_ABSENCE,
      "/SMB_SLOT@0/PSU_SLOT@0/[PSU_EEPROM]",
      "expect");
  summary_.addError(
      ExplorationErrorType::SLOT_PM_UNIT_ABSENCE,
      "/PSU_SLOT@1/[PSU_EEPROM]",
      "expect1");
  // Add unexpected errors (non-PSU devices)
  summary_.addError(
      ExplorationErrorType::I2C_DEVICE_EXPLORE, "/[MCB_MUX_A]", "fail");
  summary_.addError(
      ExplorationErrorType::I2C_DEVICE_EXPLORE, "/[MCB_MUX_B]", "fail");
  EXPECT_EQ(summary_.summarize({}, {}), ExplorationStatus::FAILED);
  EXPECT_EQ(
      facebook::fb303::fbData->getCounter(ExplorationSummary::kExplorationFail),
      1);
  EXPECT_EQ(
      facebook::fb303::fbData->getCounter(
          ExplorationSummary::kExplorationSucceeded),
      0);
  EXPECT_EQ(
      facebook::fb303::fbData->getCounter(
          ExplorationSummary::kExplorationSucceededWithExpectedErrors),
      0);
  EXPECT_EQ(
      facebook::fb303::fbData->getCounter(ExplorationSummary::kTotalFailures),
      2);
  EXPECT_EQ(
      facebook::fb303::fbData->getCounter(
          ExplorationSummary::kTotalExpectedFailures),
      2);
  for (const auto& errorType : explorationErrorTypeList()) {
    const auto key = fmt::format(
        ExplorationSummary::kExplorationFailByType,
        toExplorationErrorTypeStr(errorType));
    if (errorType == ExplorationErrorType::I2C_DEVICE_EXPLORE) {
      EXPECT_EQ(facebook::fb303::fbData->getCounter(key), 2);
    } else {
      EXPECT_EQ(facebook::fb303::fbData->getCounter(key), 0);
    }
  }
}

TEST_F(ExplorationSummaryTest, SummarizeWithVersionFields) {
  // Set up firmware and hardware version maps to test structured logging
  std::unordered_map<std::string, std::string> firmwareVersions = {
      {"bios", "1.2.3"},
      {"cpld", "4.5.6"},
      {"fpga", "7.8.9"},
  };
  std::unordered_map<std::string, std::string> hardwareVersions = {
      {"board_rev", "v2"},
      {"chassis_type", "wedge400"},
  };

  // Test with successful exploration
  EXPECT_EQ(
      summary_.summarize(firmwareVersions, hardwareVersions),
      ExplorationStatus::SUCCEEDED);
}

TEST_F(ExplorationSummaryTest, SummarizeWithVersionFieldsAndErrors) {
  // Set up firmware and hardware version maps
  std::unordered_map<std::string, std::string> firmwareVersions = {
      {"bios", "1.0.0"},
      {"bmc", "2.0.0"},
  };
  std::unordered_map<std::string, std::string> hardwareVersions = {
      {"platform", "montblanc"},
  };

  // Add an unexpected error to test logging with version fields
  summary_.addError(
      ExplorationErrorType::I2C_DEVICE_EXPLORE,
      "/[SENSOR_1]",
      "I2C read failed");

  EXPECT_EQ(
      summary_.summarize(firmwareVersions, hardwareVersions),
      ExplorationStatus::FAILED);
}
