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
