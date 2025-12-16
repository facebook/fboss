// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <fb303/ServiceData.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "fboss/platform/platform_manager/ExplorationSummary.h"

using namespace ::testing;
using namespace facebook::fboss::platform::platform_manager;

class MockExplorationSummaryTest : public ExplorationSummary {
 public:
  MockExplorationSummaryTest(
      const PlatformConfig& config,
      ScubaLogger& scubaLogger)
      : ExplorationSummary(config, scubaLogger) {}
};

class ExplorationSummaryTest : public testing::Test {
 public:
  PlatformConfig platformConfig_;
  DataStore dataStore_{platformConfig_};
  ScubaLogger scubaLogger_{*platformConfig_.platformName(), dataStore_};
  MockExplorationSummaryTest summary_{
      platformConfig_,
      scubaLogger_,
  };
};

TEST_F(ExplorationSummaryTest, GoodExploration) {
  EXPECT_EQ(summary_.summarize(), ExplorationStatus::SUCCEEDED);
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
      summary_.summarize(), ExplorationStatus::SUCCEEDED_WITH_EXPECTED_ERRORS);
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
  EXPECT_EQ(summary_.summarize(), ExplorationStatus::FAILED);
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
  EXPECT_EQ(summary_.summarize(), ExplorationStatus::FAILED);
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

TEST_F(ExplorationSummaryTest, IsSlotExpectedToBeEmpty) {
  ExplorationSummary realSummary{platformConfig_, scubaLogger_};

  // Test PSU slot paths - should return true
  EXPECT_TRUE(realSummary.isSlotExpectedToBeEmpty(
      "/SMB_SLOT@0/PSU_SLOT@0/[PSU_EEPROM]"));
  EXPECT_TRUE(realSummary.isSlotExpectedToBeEmpty(
      "/SMB_SLOT@1/PSU_SLOT@1/[PSU_EEPROM]"));
  EXPECT_TRUE(
      realSummary.isSlotExpectedToBeEmpty("/CHASSIS/PSU_SLOT@2/[PSU_DEVICE]"));
  EXPECT_TRUE(realSummary.isSlotExpectedToBeEmpty("/PSU_SLOT@0/[PSU_EEPROM]"));
  EXPECT_TRUE(realSummary.isSlotExpectedToBeEmpty("/PSU_SLOT@99/[PSU_DEVICE]"));

  // Test non-PSU slot paths - should return false
  EXPECT_FALSE(realSummary.isSlotExpectedToBeEmpty("/SMB_SLOT@0/[SCM_EEPROM]"));
  EXPECT_FALSE(realSummary.isSlotExpectedToBeEmpty("/[MCB_MUX_A]"));
  EXPECT_FALSE(realSummary.isSlotExpectedToBeEmpty("/SMB_SLOT@1/[NVME]"));
  EXPECT_FALSE(
      realSummary.isSlotExpectedToBeEmpty("/CHASSIS/[FAN_CONTROLLER]"));

  // Test edge cases - should return false (these paths don't match expected
  // format)
  EXPECT_FALSE(realSummary.isSlotExpectedToBeEmpty(
      "/PSU_SLOT@/[PSU_EEPROM]")); // Missing number
  EXPECT_FALSE(realSummary.isSlotExpectedToBeEmpty(
      "/PSU_SLOT@abc/[PSU_EEPROM]")); // Non-numeric
  EXPECT_FALSE(realSummary.isSlotExpectedToBeEmpty(
      "/SMB_SLOT@0/PSU_SLOT@0/EXTRA/[PSU_EEPROM]")); // PSU_SLOT not at end
}
