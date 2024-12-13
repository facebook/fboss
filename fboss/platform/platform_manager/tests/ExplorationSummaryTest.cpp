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
      const DataStore& store)
      : ExplorationSummary(config, store) {}
  MOCK_METHOD(bool, isDeviceExpectedToFail, (const std::string&));
};

class ExplorationSummaryTest : public testing::Test {
 public:
  PlatformConfig platformConfig_;
  DataStore dataStore_{platformConfig_};
  MockExplorationSummaryTest summary_{platformConfig_, dataStore_};
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
  EXPECT_CALL(summary_, isDeviceExpectedToFail(_))
      .Times(3)
      .WillRepeatedly(Return(true));
  summary_.addError(
      ExplorationErrorType::I2C_DEVICE_EXPLORE, "/[SCM_EEPROM]", "expect");
  summary_.addError(
      ExplorationErrorType::I2C_DEVICE_EXPLORE, "/[SCM_EEPROM_2]", "expect");
  summary_.addError(
      ExplorationErrorType::I2C_DEVICE_EXPLORE, "/[SCM_EEPROM_3]", "expect");
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
  EXPECT_CALL(summary_, isDeviceExpectedToFail(_))
      .Times(3)
      .WillRepeatedly(Return(false));
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
  Sequence s;
  EXPECT_CALL(summary_, isDeviceExpectedToFail(_))
      .InSequence(s)
      .WillOnce(Return(true))
      .WillOnce(Return(true))
      .WillOnce(Return(false))
      .WillOnce(Return(false));
  summary_.addError(
      ExplorationErrorType::I2C_DEVICE_EXPLORE, "/", "SCM_EEPROM", "expect");
  summary_.addError(
      ExplorationErrorType::I2C_DEVICE_EXPLORE, "/", "SCM_EEPROM", "expect1");
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
