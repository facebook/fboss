// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <gtest/gtest.h>

#include "fboss/platform/fw_util/ConfigValidator.h"

using namespace ::testing;
using namespace facebook::fboss::platform::fw_util;
using namespace facebook::fboss::platform::fw_util_config;

namespace {
FwConfig createValidNewFwConfig() {
  FwConfig fwConfig;

  VersionConfig versionConfig;
  versionConfig.versionType() = "sysfs";
  versionConfig.path() = "/sys/devices/virtual/dmi/id/bios_version";
  fwConfig.version() = versionConfig;

  fwConfig.priority() = 1;
  fwConfig.desiredVersion() = "2.12.g8aeaf1b4";
  fwConfig.sha1sum() = "182f3c1d2d2609f9f0b139aa36fe172193fe491d";

  return fwConfig;
}

} // namespace

TEST(ConfigValidatorTest, ValidConfig) {
  auto config = FwUtilConfig();

  // Create a valid config similar to real configs (darwin, morgan800cc, etc.)
  std::map<std::string, FwConfig> fwConfigs;
  fwConfigs["bios"] = createValidNewFwConfig();

  config.fwConfigs() = fwConfigs;

  EXPECT_TRUE(ConfigValidator().isValid(config));
}

TEST(ConfigValidatorTest, EmptyConfig) {
  auto config = FwUtilConfig();

  // Create config with empty fwConfigs map - this should still be valid (eg
  // SEV)
  std::map<std::string, FwConfig> emptyFwConfigs;
  config.fwConfigs() = emptyFwConfigs;

  EXPECT_TRUE(ConfigValidator().isValid(config));
}

TEST(ConfigValidatorTest, InvalidConfigEmptyDeviceName) {
  auto config = FwUtilConfig();

  // Create config with empty device name - this should be invalid
  std::map<std::string, FwConfig> fwConfigs;
  fwConfigs[""] = createValidNewFwConfig(); // Empty device name

  config.fwConfigs() = fwConfigs;

  EXPECT_FALSE(ConfigValidator().isValid(config));
}

TEST(ConfigValidatorTest, InvalidConfigNegativePriority) {
  auto config = FwUtilConfig();

  // Create config with invalid priority (-1) - this should be invalid
  std::map<std::string, FwConfig> fwConfigs;
  auto fwConfig = createValidNewFwConfig();
  fwConfig.priority() = -1; // Invalid priority (must be >= 0)
  fwConfigs["bios"] = fwConfig;

  config.fwConfigs() = fwConfigs;

  EXPECT_FALSE(ConfigValidator().isValid(config));
}

TEST(ConfigValidatorTest, InvalidConfigInvalidVersionType) {
  auto config = FwUtilConfig();

  // Create config with invalid version type - this should be invalid
  std::map<std::string, FwConfig> fwConfigs;
  auto fwConfig = createValidNewFwConfig();
  auto versionConfig = *fwConfig.version();
  versionConfig.versionType() = "invalid_type"; // Invalid version type
  fwConfig.version() = versionConfig;
  fwConfigs["bios"] = fwConfig;

  config.fwConfigs() = fwConfigs;

  EXPECT_FALSE(ConfigValidator().isValid(config));
}

TEST(ConfigValidatorTest, InvalidConfigSysfsWithoutPath) {
  auto config = FwUtilConfig();

  // Create config with sysfs version type but no path - this should be invalid
  std::map<std::string, FwConfig> fwConfigs;
  auto fwConfig = createValidNewFwConfig();
  auto versionConfig = *fwConfig.version();
  versionConfig.versionType() = "sysfs";
  versionConfig.path().reset(); // Remove path (required for sysfs)
  fwConfig.version() = versionConfig;
  fwConfigs["bios"] = fwConfig;

  config.fwConfigs() = fwConfigs;

  EXPECT_FALSE(ConfigValidator().isValid(config));
}

TEST(ConfigValidatorTest, InvalidConfigEmptyDesiredVersion) {
  auto config = FwUtilConfig();

  // Create config with empty desiredVersion when provided - this should be
  // invalid
  std::map<std::string, FwConfig> fwConfigs;
  auto fwConfig = createValidNewFwConfig();
  fwConfig.desiredVersion() = ""; // Empty when provided (invalid)
  fwConfigs["bios"] = fwConfig;

  config.fwConfigs() = fwConfigs;

  EXPECT_FALSE(ConfigValidator().isValid(config));
}
