// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <gtest/gtest.h>
#include <stdexcept>
#include <string>

#include "fboss/platform/helpers/MockPlatformUtils.h"
#include "fboss/platform/platform_manager/PlatformI2cExplorer.h"

using namespace ::testing;
using namespace facebook::fboss::platform;
using namespace facebook::fboss::platform::platform_manager;

namespace {
class MockPlatformI2cExplorer : public PlatformI2cExplorer {
 public:
  explicit MockPlatformI2cExplorer(
      std::shared_ptr<MockPlatformUtils> platformUtils)
      : PlatformI2cExplorer(platformUtils) {}
  MOCK_METHOD(bool, isI2cDevicePresent, (const std::string&, uint8_t));
  MOCK_METHOD(
      std::optional<std::string>,
      getI2cDeviceName,
      (const std::string&, uint8_t));
};

} // namespace

TEST(PlatformI2cExplorerTest, createI2cDeviceSuccess) {
  auto platformUtils = std::make_shared<MockPlatformUtils>();
  auto i2cExplorer = MockPlatformI2cExplorer(platformUtils);

  // CASE-1: No device present; creation succeeds.
  EXPECT_CALL(i2cExplorer, isI2cDevicePresent("i2c-4", 15))
      .WillOnce(Return(false));
  EXPECT_CALL(
      *platformUtils,
      execCommand("echo lm73 15 > /sys/bus/i2c/devices/i2c-4/new_device"))
      .WillOnce(Return(std::pair(0, "")));
  EXPECT_NO_THROW(i2cExplorer.createI2cDevice("lm73", "i2c-4", 15));

  // CASE-2: Same device already present; creation skipped
  EXPECT_CALL(i2cExplorer, isI2cDevicePresent("i2c-5", 16))
      .WillOnce(Return(true));
  EXPECT_CALL(i2cExplorer, getI2cDeviceName("i2c-5", 16))
      .WillOnce(Return("lm73"));
  EXPECT_NO_THROW(i2cExplorer.createI2cDevice("lm73", "i2c-5", 16));
}

TEST(PlatformI2cExplorerTest, createI2cDeviceFailure) {
  auto platformUtils = std::make_shared<MockPlatformUtils>();
  auto i2cExplorer = MockPlatformI2cExplorer(platformUtils);
  // CASE-1: echoing lm75 15 into the new_device file fails.
  EXPECT_CALL(i2cExplorer, isI2cDevicePresent("i2c-4", 15))
      .WillOnce(Return(false));
  EXPECT_CALL(
      *platformUtils,
      execCommand("echo lm73 15 > /sys/bus/i2c/devices/i2c-4/new_device"))
      .WillOnce(Return(std::pair(-1, "")));
  EXPECT_THROW(
      i2cExplorer.createI2cDevice("lm73", "i2c-4", 15), std::runtime_error);

  // CASE-2: different device already present.
  EXPECT_CALL(i2cExplorer, isI2cDevicePresent("i2c-5", 16))
      .WillOnce(Return(true));
  EXPECT_CALL(i2cExplorer, getI2cDeviceName("i2c-5", 16))
      .WillOnce(Return("pca9546"));
  EXPECT_THROW(
      i2cExplorer.createI2cDevice("lm73", "i2c-5", 16), std::runtime_error);
}

TEST(PlatformI2cExplorerTest, getDeviceI2cPath) {
  auto i2cExplorer = PlatformI2cExplorer();
  EXPECT_EQ(
      i2cExplorer.getDeviceI2cPath("i2c-4", 15), "/sys/bus/i2c/devices/4-0015");
  EXPECT_THROW(i2cExplorer.getDeviceI2cPath("i2dc-4", 15), std::runtime_error);
  EXPECT_THROW(i2cExplorer.getDeviceI2cPath("i2c--4", 15), std::runtime_error);
  EXPECT_THROW(i2cExplorer.getDeviceI2cPath("i2c-4d", 15), std::runtime_error);
  EXPECT_THROW(i2cExplorer.getDeviceI2cPath("i2c4", 15), std::runtime_error);
}
