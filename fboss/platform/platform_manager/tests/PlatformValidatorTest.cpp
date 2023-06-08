// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <folly/logging/xlog.h>

#include <gtest/gtest.h>

#include <thrift/lib/cpp2/protocol/Serializer.h>

#include "fboss/platform/platform_manager/PlatformValidator.h"

using namespace ::testing;
using namespace apache::thrift;
using namespace facebook::fboss::platform;
using namespace facebook::fboss::platform::platform_manager;

TEST(PlatformValidatorTest, InvalidPlatformName) {
  auto config = PlatformConfig();
  config.platformName() = "";
  EXPECT_FALSE(PlatformValidator().isValid(config));
}

TEST(PlatformValidatorTest, MissingChassisSlotTypeConfig) {
  auto config = PlatformConfig();
  config.platformName() = "meru";
  EXPECT_FALSE(PlatformValidator().isValid(config));
}

TEST(PlatformValidatorTest, ValidConfig) {
  auto config = PlatformConfig();
  config.platformName() = "meru";
  config.slotTypeConfigs()["CHASSIS"] = SlotTypeConfig{};
  EXPECT_TRUE(PlatformValidator().isValid(config));
}
