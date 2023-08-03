// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <gtest/gtest.h>
#include <stdexcept>
#include <string>

#include "fboss/platform/helpers/MockPlatformUtils.h"
#include "fboss/platform/platform_manager/PlatformI2cExplorer.h"

using namespace ::testing;
using namespace facebook::fboss::platform;
using namespace facebook::fboss::platform::platform_manager;

TEST(PlatformI2cExplorerTest, createI2cDeviceSuccess) {
  auto platformUtils = std::make_shared<MockPlatformUtils>();
  auto i2cExplorer = PlatformI2cExplorer(platformUtils);
  EXPECT_CALL(
      *platformUtils,
      execCommand("echo lm73 15 > /sys/bus/i2c/devices/i2c-4/new_device"))
      .WillOnce(Return(std::pair(0, "")));
  EXPECT_NO_THROW(i2cExplorer.createI2cDevice("lm73", "i2c-4", 15));
}

TEST(PlatformI2cExplorerTest, createI2cDeviceFailure) {
  auto platformUtils = std::make_shared<MockPlatformUtils>();
  auto i2cExplorer = PlatformI2cExplorer(platformUtils);
  EXPECT_CALL(
      *platformUtils,
      execCommand("echo lm73 15 > /sys/bus/i2c/devices/i2c-4/new_device"))
      .WillOnce(Return(std::pair(-1, "")));
  EXPECT_THROW(
      i2cExplorer.createI2cDevice("lm73", "i2c-4", 15), std::runtime_error);
}
