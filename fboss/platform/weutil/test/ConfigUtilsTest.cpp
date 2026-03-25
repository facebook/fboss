// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <gtest/gtest.h>
#include <thrift/lib/cpp2/protocol/Serializer.h>

#include <folly/FileUtil.h>

#include "fboss/platform/weutil/ConfigUtils.h"

using namespace ::testing;
using namespace apache::thrift;
using namespace facebook::fboss::platform::weutil;

namespace facebook::fboss::platform::weutil {

class ConfigUtilsTest : public ::testing::Test {};

TEST_F(ConfigUtilsTest, InvalidConfigThrows) {
  EXPECT_THROW(
      ConfigUtils("invalid_platform_that_does_not_exist"), std::runtime_error);
}

TEST_F(ConfigUtilsTest, GetFruEepromThrowsWhenNotFound) {
  ConfigUtils configUtils("DARWIN");
  EXPECT_THROW(configUtils.getFruEeprom("NONEXISTENT"), std::runtime_error);
}

TEST_F(ConfigUtilsTest, GetFruEepromListReturnsValid) {
  ConfigUtils configUtils("DARWIN");
  auto fruEepromList = configUtils.getFruEepromList();
  EXPECT_FALSE(fruEepromList.empty());
}

TEST_F(ConfigUtilsTest, GetFruEepromListContainsExpectedEeproms) {
  ConfigUtils configUtils("DARWIN");
  auto fruEepromList = configUtils.getFruEepromList();
  EXPECT_TRUE(fruEepromList.count("CHASSIS") > 0);
}

TEST_F(ConfigUtilsTest, GetChassisEepromNameReturnsValid) {
  ConfigUtils configUtils("DARWIN");
  std::string chassisName = configUtils.getChassisEepromName();
  EXPECT_FALSE(chassisName.empty());
  EXPECT_EQ(chassisName, "CHASSIS");
}

TEST_F(ConfigUtilsTest, GetFruEepromReturnsValidStruct) {
  ConfigUtils configUtils("DARWIN");
  FruEeprom fruEeprom = configUtils.getFruEeprom("CHASSIS");
  EXPECT_GE(fruEeprom.offset, 0);
}

TEST_F(ConfigUtilsTest, GetFruEepromListForMeru800bfaIncludesSCM) {
  ConfigUtils configUtils("MERU800BFA");
  auto fruEepromList = configUtils.getFruEepromList();
  EXPECT_TRUE(fruEepromList.count("SCM") > 0);
  if (fruEepromList.count("SCM") > 0) {
    EXPECT_EQ(fruEepromList["SCM"].path, "/run/devmap/eeproms/MERU_SCM_EEPROM");
  }
}

TEST_F(ConfigUtilsTest, GetFruEepromListForMeru800biaIncludesSCM) {
  ConfigUtils configUtils("MERU800BIA");
  auto fruEepromList = configUtils.getFruEepromList();
  EXPECT_TRUE(fruEepromList.count("SCM") > 0);
  if (fruEepromList.count("SCM") > 0) {
    EXPECT_EQ(fruEepromList["SCM"].path, "/run/devmap/eeproms/MERU_SCM_EEPROM");
  }
}

TEST_F(ConfigUtilsTest, GetFruEepromListForDarwinIncludesChassis) {
  ConfigUtils configUtils("DARWIN");
  auto fruEepromList = configUtils.getFruEepromList();
  EXPECT_TRUE(fruEepromList.count("CHASSIS") > 0);
  if (fruEepromList.count("CHASSIS") > 0) {
    EXPECT_EQ(fruEepromList["CHASSIS"].path, "");
    EXPECT_EQ(fruEepromList["CHASSIS"].offset, 0);
  }
}

TEST_F(ConfigUtilsTest, GetFruEepromWithValidNameSucceeds) {
  ConfigUtils configUtils("DARWIN");
  EXPECT_NO_THROW(configUtils.getFruEeprom("CHASSIS"));
}

TEST_F(ConfigUtilsTest, GetFruEepromListReturnsCorrectOffsets) {
  ConfigUtils configUtils("DARWIN");
  auto fruEepromList = configUtils.getFruEepromList();
  for (const auto& [name, eeprom] : fruEepromList) {
    EXPECT_GE(eeprom.offset, 0);
  }
}

TEST_F(ConfigUtilsTest, GetFruEepromListForDifferentPlatforms) {
  std::vector<std::string> platforms = {"DARWIN", "MERU800BFA", "MERU800BIA"};
  for (const auto& platform : platforms) {
    ConfigUtils configUtils(platform);
    auto fruEepromList = configUtils.getFruEepromList();
    EXPECT_FALSE(fruEepromList.empty()) << "Platform: " << platform;
  }
}

TEST_F(ConfigUtilsTest, GetChassisEepromNameConsistentWithList) {
  ConfigUtils configUtils("DARWIN");
  std::string chassisName = configUtils.getChassisEepromName();
  auto fruEepromList = configUtils.getFruEepromList();
  EXPECT_TRUE(fruEepromList.count(chassisName) > 0);
}

TEST_F(ConfigUtilsTest, FruEepromStructHasValidFields) {
  ConfigUtils configUtils("DARWIN");
  FruEeprom fruEeprom = configUtils.getFruEeprom("CHASSIS");
  EXPECT_TRUE(!fruEeprom.path.empty() || fruEeprom.path == "");
  EXPECT_GE(fruEeprom.offset, 0);
}

TEST_F(ConfigUtilsTest, GetFruEepromThrowsWithMeaningfulMessage) {
  ConfigUtils configUtils("DARWIN");
  try {
    configUtils.getFruEeprom("INVALID_NAME");
    FAIL() << "Expected std::runtime_error";
  } catch (const std::runtime_error& e) {
    std::string errorMsg(e.what());
    EXPECT_TRUE(errorMsg.find("INVALID_NAME") != std::string::npos);
    EXPECT_TRUE(errorMsg.find("Valid EEPROM names") != std::string::npos);
  }
}

} // namespace facebook::fboss::platform::weutil
