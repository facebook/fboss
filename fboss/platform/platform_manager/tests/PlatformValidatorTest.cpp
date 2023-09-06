// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <folly/logging/xlog.h>

#include <gtest/gtest.h>

#include <thrift/lib/cpp2/protocol/Serializer.h>

#include "fboss/platform/platform_manager/PlatformValidator.h"

using namespace ::testing;
using namespace apache::thrift;
using namespace facebook::fboss::platform;
using namespace facebook::fboss::platform::platform_manager;

namespace {
SlotTypeConfig getValidSlotTypeConfig() {
  auto slotTypeConfig = SlotTypeConfig();
  slotTypeConfig.fruType_ref() = "FAN_TRAY";
  slotTypeConfig.idpromConfig_ref() = IdpromConfig();
  return slotTypeConfig;
}
} // namespace

TEST(PlatformValidatorTest, InvalidPlatformName) {
  auto config = PlatformConfig();
  config.platformName() = "";
  EXPECT_FALSE(PlatformValidator().isValid(config));
}

TEST(PlatformValidatorTest, MissingChassisSlotTypeConfig) {
  auto config = PlatformConfig();
  config.platformName() = "MERU400BIU";
  auto fruTypeConfig = FruTypeConfig();
  fruTypeConfig.pluggedInSlotType() = "CHASSIS_SLOT";
  config.fruTypeConfigs()["CHASSIS_FRU"] = fruTypeConfig;
  EXPECT_FALSE(PlatformValidator().isValid(config));
}

TEST(PlatformValidatorTest, MissingChassisFruTypeConfig) {
  auto config = PlatformConfig();
  config.platformName() = "MERU400BIU";
  config.slotTypeConfigs()["CHASSIS_SLOT"] = getValidSlotTypeConfig();
  EXPECT_FALSE(PlatformValidator().isValid(config));
}

TEST(PlatformValidatorTest, ValidConfig) {
  auto config = PlatformConfig();
  config.platformName() = "MERU400BIU";
  config.slotTypeConfigs()["CHASSIS_SLOT"] = getValidSlotTypeConfig();
  auto fruTypeConfig = FruTypeConfig();
  fruTypeConfig.pluggedInSlotType() = "CHASSIS_SLOT";
  config.fruTypeConfigs()["CHASSIS_FRU"] = fruTypeConfig;
  EXPECT_TRUE(PlatformValidator().isValid(config));
}

TEST(PlatformValidatorTest, SlotTypeConfig) {
  auto slotTypeConfig = getValidSlotTypeConfig();
  EXPECT_TRUE(PlatformValidator().isValidSlotTypeConfig(slotTypeConfig));
  slotTypeConfig.fruType_ref().reset();
  EXPECT_TRUE(PlatformValidator().isValidSlotTypeConfig(slotTypeConfig));
  slotTypeConfig = getValidSlotTypeConfig();
  slotTypeConfig.idpromConfig_ref().reset();
  EXPECT_TRUE(PlatformValidator().isValidSlotTypeConfig(slotTypeConfig));
  slotTypeConfig.fruType_ref().reset();
  slotTypeConfig.idpromConfig_ref().reset();
  EXPECT_FALSE(PlatformValidator().isValidSlotTypeConfig(slotTypeConfig));
}
