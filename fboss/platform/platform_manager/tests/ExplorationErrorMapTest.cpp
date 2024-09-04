// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "fboss/platform/platform_manager/ExplorationErrorMap.h"

using namespace ::testing;
using namespace facebook::fboss::platform::platform_manager;

class MockExplorationErrorMap : public ExplorationErrorMap {
 public:
  MockExplorationErrorMap(const PlatformConfig& config, const DataStore& store)
      : ExplorationErrorMap(config, store) {}
  MOCK_METHOD(bool, isExpectedDeviceToFail, (const std::string&));
};

class ExplorationErrorMapTest : public testing::Test {
 public:
  PlatformConfig platformConfig_;
  DataStore dataStore_{platformConfig_};
  MockExplorationErrorMap map_{platformConfig_, dataStore_};
};

TEST_F(ExplorationErrorMapTest, EmptyMap) {
  EXPECT_TRUE(map_.empty());
  EXPECT_FALSE(map_.hasExpectedErrors());
  EXPECT_EQ(map_.size(), 0);
  for (const auto& it : map_) {
    // Shouldn't hit here.
    EXPECT_TRUE(false);
  }
}

TEST_F(ExplorationErrorMapTest, OnlyExpectedErrors) {
  EXPECT_CALL(map_, isExpectedDeviceToFail(_))
      .Times(3)
      .WillRepeatedly(Return(true));
  std::vector<std::string> expectedFailedDevicPaths = {
      "/[expected1]", "/[expected2]", "/[expected3]"};
  for (const auto& devicePath : expectedFailedDevicPaths) {
    map_.add(devicePath, "message");
  }
  EXPECT_FALSE(map_.empty());
  EXPECT_TRUE(map_.hasExpectedErrors());
  EXPECT_EQ(map_.numExpectedErrors(), 3);
  EXPECT_EQ(map_.size(), 3);
  for (const auto& [devicePath, errorMessages] : map_) {
    EXPECT_TRUE(errorMessages.isExpected());
    EXPECT_EQ(errorMessages.getMessages().size(), 1);
  }
}

TEST_F(ExplorationErrorMapTest, OnlyErrors) {
  EXPECT_CALL(map_, isExpectedDeviceToFail(_))
      .Times(3)
      .WillRepeatedly(Return(false));
  std::vector<std::string> expectedFailedDevicPaths = {
      "/[!expected1]", "/[!expected2]", "/[!expected3]"};
  for (const auto& devicePath : expectedFailedDevicPaths) {
    map_.add(devicePath, "message");
  }
  EXPECT_FALSE(map_.empty());
  EXPECT_FALSE(map_.hasExpectedErrors());
  EXPECT_EQ(map_.numExpectedErrors(), 0);
  EXPECT_EQ(map_.size(), 3);
  for (const auto& [devicePath, errorMessages] : map_) {
    EXPECT_FALSE(errorMessages.isExpected());
    EXPECT_EQ(errorMessages.getMessages().size(), 1);
  }
}

TEST_F(ExplorationErrorMapTest, MixedErrors) {
  Sequence s;
  EXPECT_CALL(map_, isExpectedDeviceToFail(_))
      .InSequence(s)
      .WillOnce(Return(true))
      .WillOnce(Return(false))
      .WillOnce(Return(false));
  map_.add("/", "expected1", "message");
  map_.add("/[expected1]", "message2");
  map_.add("/", "!expected1", "message");
  map_.add("/[!expected2]", "message");
  EXPECT_FALSE(map_.empty());
  EXPECT_TRUE(map_.hasExpectedErrors());
  EXPECT_EQ(map_.numExpectedErrors(), 1);
  EXPECT_EQ(map_.size(), 3);
  for (const auto& [devicePath, errorMessages] : map_) {
    if (devicePath == "/[expected1]") {
      EXPECT_TRUE(errorMessages.isExpected());
      EXPECT_EQ(errorMessages.getMessages().size(), 2);
    } else {
      EXPECT_FALSE(errorMessages.isExpected());
      EXPECT_EQ(errorMessages.getMessages().size(), 1);
    }
  }
}
