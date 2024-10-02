// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "fboss/platform/helpers/MockPlatformFsUtils.h"
#include "fboss/platform/platform_manager/PresenceChecker.h"
#include "fboss/platform/platform_manager/Utils.h"

using namespace ::testing;
using namespace facebook::fboss::platform;
using namespace facebook::fboss::platform::platform_manager;

class MockDevicePathResolver : public DevicePathResolver {
 public:
  explicit MockDevicePathResolver(DataStore& dataStore)
      : DevicePathResolver(dataStore) {
    std::cout << "Constructing MockDevicePathResolver" << std::endl;
  }
  MOCK_METHOD(
      std::string,
      resolvePciSubDevCharDevPath,
      (const std::string&),
      (const));
  MOCK_METHOD(
      std::optional<std::string>,
      resolvePresencePath,
      (const std::string&, const std::string&),
      (const));
};

class MockUtils : public Utils {
 public:
  MOCK_METHOD(int, getGpioLineValue, (const std::string&, int), (const));
};

class PresenceCheckerTest : public testing::Test {
 public:
  void SetUp() override {}

  PlatformConfig platformConfig_;
  DataStore dataStore_{platformConfig_};

  MockDevicePathResolver devicePathResolver_{dataStore_};
};

TEST_F(PresenceCheckerTest, gpioPresent) {
  auto utils = std::make_shared<MockUtils>();
  auto platformFsUtils = std::make_shared<MockPlatformFsUtils>();
  PresenceChecker presenceChecker(devicePathResolver_, utils, platformFsUtils);

  PresenceDetection presenceDetection = PresenceDetection();
  presenceDetection.gpioLineHandle() = GpioLineHandle();
  presenceDetection.gpioLineHandle()->devicePath() = "device/path";
  presenceDetection.gpioLineHandle()->lineIndex() = 0;
  presenceDetection.gpioLineHandle()->desiredValue() = 1;

  EXPECT_CALL(devicePathResolver_, resolvePciSubDevCharDevPath("device/path"))
      .WillRepeatedly(Return("/dev/gpiochip0"));

  EXPECT_CALL(*utils, getGpioLineValue("/dev/gpiochip0", 0))
      .WillOnce(Return(1));
  EXPECT_TRUE(presenceChecker.isPresent(presenceDetection, "/slot/path"));

  EXPECT_CALL(*utils, getGpioLineValue("/dev/gpiochip0", 0))
      .WillOnce(Return(0));
  EXPECT_FALSE(presenceChecker.isPresent(presenceDetection, "/slot/path"));

  EXPECT_CALL(devicePathResolver_, resolvePciSubDevCharDevPath("device/path"))
      .WillOnce(Throw(std::runtime_error("error")));
  EXPECT_THROW(
      presenceChecker.isPresent(presenceDetection, "/slot/path"),
      std::runtime_error);
}

TEST_F(PresenceCheckerTest, sysfsPresent) {
  auto utils = std::make_shared<MockUtils>();
  auto platformFsUtils = std::make_shared<MockPlatformFsUtils>();
  PresenceChecker presenceChecker(devicePathResolver_, utils, platformFsUtils);

  PresenceDetection presenceDetection = PresenceDetection();
  presenceDetection.sysfsFileHandle() = SysfsFileHandle();
  presenceDetection.sysfsFileHandle()->devicePath() = "device/path";
  presenceDetection.sysfsFileHandle()->presenceFileName() = "presenceName";
  presenceDetection.sysfsFileHandle()->desiredValue() = 1;

  EXPECT_CALL(
      devicePathResolver_, resolvePresencePath("device/path", "presenceName"))
      .WillRepeatedly(Return("/file/path"));

  EXPECT_CALL(*platformFsUtils, getStringFileContent({"/file/path"}))
      .WillOnce(Return(std::make_optional("1")));
  EXPECT_TRUE(presenceChecker.isPresent(presenceDetection, "/slot/path"));

  EXPECT_CALL(*platformFsUtils, getStringFileContent({"/file/path"}))
      .WillOnce(Return(std::make_optional("0")));
  EXPECT_FALSE(presenceChecker.isPresent(presenceDetection, "/slot/path"));

  EXPECT_CALL(
      devicePathResolver_, resolvePresencePath("device/path", "presenceName"))
      .WillOnce(Return(std::nullopt));
  EXPECT_THROW(
      presenceChecker.isPresent(presenceDetection, "/slot/path"),
      std::runtime_error);
}
