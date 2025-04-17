// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <gtest/gtest.h>
#include <filesystem>
#include <stdexcept>
#include <string>

#include "fboss/platform/helpers/MockPlatformFsUtils.h"
#include "fboss/platform/platform_manager/I2cExplorer.h"

namespace fs = std::filesystem;
using namespace ::testing;
using namespace facebook::fboss::platform;
using namespace facebook::fboss::platform::platform_manager;

namespace {
class MockI2cExplorer : public I2cExplorer {
 public:
  explicit MockI2cExplorer(std::shared_ptr<MockPlatformFsUtils> platformFsUtils)
      : I2cExplorer(platformFsUtils) {}
  MOCK_METHOD(bool, isI2cDevicePresent, (uint16_t, const I2cAddr&), (const));
  MOCK_METHOD(
      std::optional<std::string>,
      getI2cDeviceName,
      (uint16_t, const I2cAddr&));
};

} // namespace

TEST(I2cExplorerTest, createI2cDeviceSuccess) {
  auto platformFsUtils = std::make_shared<MockPlatformFsUtils>();
  auto i2cExplorer = MockI2cExplorer(platformFsUtils);

  // CASE-1: No device present; creation succeeds.
  EXPECT_CALL(i2cExplorer, isI2cDevicePresent(4, I2cAddr(15)))
      .WillOnce(Return(false))
      .WillOnce(Return(true));
  EXPECT_CALL(
      *platformFsUtils,
      writeStringToSysfs(
          "lm73 0x0f", fs::path("/sys/bus/i2c/devices/i2c-4/new_device")))
      .WillOnce(Return(true));
  EXPECT_NO_THROW(
      i2cExplorer.createI2cDevice("TEST_SENSOR", "lm73", 4, I2cAddr(15)));

  // CASE-2: Same device already present; creation skipped
  EXPECT_CALL(i2cExplorer, isI2cDevicePresent(5, I2cAddr(16)))
      .WillOnce(Return(true));
  EXPECT_CALL(i2cExplorer, getI2cDeviceName(5, I2cAddr(16)))
      .WillOnce(Return("lm73"));
  EXPECT_NO_THROW(
      i2cExplorer.createI2cDevice("TEST_SENSOR", "lm73", 5, I2cAddr(16)));
}

TEST(I2cExplorerTest, createI2cDeviceFailure) {
  auto platformFsUtils = std::make_shared<MockPlatformFsUtils>();
  auto i2cExplorer = MockI2cExplorer(platformFsUtils);
  // CASE-1: echoing lm75 15 into the new_device file fails.
  EXPECT_CALL(i2cExplorer, isI2cDevicePresent(4, I2cAddr(15)))
      .WillRepeatedly(Return(false));
  EXPECT_CALL(
      *platformFsUtils,
      writeStringToSysfs(
          "lm73 0x0f", fs::path("/sys/bus/i2c/devices/i2c-4/new_device")))
      .WillOnce(Return(false));
  EXPECT_THROW(
      i2cExplorer.createI2cDevice("TEST_SENSOR", "lm73", 4, I2cAddr(15)),
      std::runtime_error);

  // CASE-2: echoing succeeded but I2C device not created.
  EXPECT_CALL(i2cExplorer, isI2cDevicePresent(3, I2cAddr(16)))
      .WillRepeatedly(Return(false));
  EXPECT_CALL(
      *platformFsUtils,
      writeStringToSysfs(
          "pmbus 0x10", fs::path("/sys/bus/i2c/devices/i2c-3/new_device")))
      .WillOnce(Return(true));
  EXPECT_THROW(
      i2cExplorer.createI2cDevice("TEST_SENSOR", "pmbus", 3, I2cAddr(16)),
      std::runtime_error);

  // CASE-3: different device already present.
  EXPECT_CALL(i2cExplorer, isI2cDevicePresent(5, I2cAddr(16)))
      .WillOnce(Return(true));
  EXPECT_CALL(i2cExplorer, getI2cDeviceName(5, I2cAddr(16)))
      .WillOnce(Return("pca9546"));
  EXPECT_THROW(
      i2cExplorer.createI2cDevice("TEST_SENSOR", "lm73", 5, I2cAddr(16)),
      std::runtime_error);
}

TEST(I2cExplorerTest, getDeviceI2cPath) {
  auto i2cExplorer = I2cExplorer();
  EXPECT_EQ(
      i2cExplorer.getDeviceI2cPath(4, I2cAddr(5)),
      "/sys/bus/i2c/devices/4-0005");
  EXPECT_EQ(
      i2cExplorer.getDeviceI2cPath(4, I2cAddr(15)),
      "/sys/bus/i2c/devices/4-000f");
  EXPECT_EQ(
      i2cExplorer.getDeviceI2cPath(5, I2cAddr(16)),
      "/sys/bus/i2c/devices/5-0010");
}

TEST(I2cExplorerTest, I2cAddr) {
  EXPECT_EQ(I2cAddr("0x2f").hex2Str(), "0x2f");
  EXPECT_EQ(I2cAddr(20).hex2Str(), "0x14");
  EXPECT_THROW(I2cAddr("0xGF"), std::invalid_argument);
  EXPECT_THROW(I2cAddr("20"), std::invalid_argument);
  EXPECT_THROW(I2cAddr("0x"), std::invalid_argument);
  EXPECT_THROW(I2cAddr("0x2F"), std::invalid_argument);
  EXPECT_THROW(I2cAddr("0xf"), std::invalid_argument);
  EXPECT_NO_THROW(I2cAddr("0x0f"));
  EXPECT_NO_THROW(I2cAddr("0xaf"));
}
