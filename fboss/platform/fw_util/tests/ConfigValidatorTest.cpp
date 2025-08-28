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

TEST(ConfigValidatorTest, InvalidConfigInvalidSha1Sum) {
  auto config = FwUtilConfig();

  // Create config with invalid sha1sum when provided - this should be invalid
  std::map<std::string, FwConfig> fwConfigs;
  auto fwConfig = createValidNewFwConfig();
  fwConfig.sha1sum() = "invalid_sha1_format"; // Invalid SHA1 format
  fwConfigs["bios"] = fwConfig;

  config.fwConfigs() = fwConfigs;

  EXPECT_FALSE(ConfigValidator().isValid(config));
}

TEST(ConfigValidatorTest, ValidPreUpgradeFlashrom) {
  auto config = FwUtilConfig();

  // Create config with valid flashrom preUpgrade (using upgrade section example
  // since flashrom not used in preUpgrade in real configs)
  std::map<std::string, FwConfig> fwConfigs;
  auto fwConfig = createValidNewFwConfig();

  std::vector<PreFirmwareOperationConfig> preUpgrade;
  PreFirmwareOperationConfig preUpgradeConfig;
  preUpgradeConfig.commandType() = "flashrom";

  // Based on real config upgrade sections since flashrom not used in preUpgrade
  FlashromConfig flashromConfig;
  flashromConfig.programmer_type() = "linux_spi:dev=";
  flashromConfig.programmer() = "/run/devmap/flashes/SMB_SPI_MASTER_2_DEVICE_1";
  preUpgradeConfig.flashromArgs() = flashromConfig;

  preUpgrade.push_back(preUpgradeConfig);
  fwConfig.preUpgrade() = preUpgrade;

  fwConfigs["bios"] = fwConfig;
  config.fwConfigs() = fwConfigs;

  EXPECT_TRUE(ConfigValidator().isValid(config));
}

TEST(ConfigValidatorTest, ValidPreUpgradeJtag) {
  auto config = FwUtilConfig();

  // Create config with valid jtag preUpgrade based on real darwin config
  std::map<std::string, FwConfig> fwConfigs;
  auto fwConfig = createValidNewFwConfig();

  std::vector<PreFirmwareOperationConfig> preUpgrade;
  PreFirmwareOperationConfig preUpgradeConfig;
  preUpgradeConfig.commandType() = "jtag";

  // Based on real darwin/darwin48v config
  JtagConfig jtagConfig;
  jtagConfig.path() = "/run/devmap/cplds/BLACKHAWK_CPLD/scd_jtag_sel";
  jtagConfig.value() = 0;
  preUpgradeConfig.jtagArgs() = jtagConfig;

  preUpgrade.push_back(preUpgradeConfig);
  fwConfig.preUpgrade() = preUpgrade;

  fwConfigs["bios"] = fwConfig;
  config.fwConfigs() = fwConfigs;

  EXPECT_TRUE(ConfigValidator().isValid(config));
}

TEST(ConfigValidatorTest, ValidPreUpgradeGpioset) {
  auto config = FwUtilConfig();

  // Create config with valid gpioset preUpgrade based on real icecube config
  std::map<std::string, FwConfig> fwConfigs;
  auto fwConfig = createValidNewFwConfig();

  std::vector<PreFirmwareOperationConfig> preUpgrade;
  PreFirmwareOperationConfig preUpgradeConfig;
  preUpgradeConfig.commandType() = "gpioset";

  // Based on real icecube/icetea config
  GpiosetConfig gpiosetConfig;
  gpiosetConfig.gpioChip() = "fboss_iob_pci.gpiochip.*";
  gpiosetConfig.gpioChipPin() = "66";
  gpiosetConfig.gpioChipValue() = "1";
  preUpgradeConfig.gpiosetArgs() = gpiosetConfig;

  preUpgrade.push_back(preUpgradeConfig);
  fwConfig.preUpgrade() = preUpgrade;

  fwConfigs["bios"] = fwConfig;
  config.fwConfigs() = fwConfigs;

  EXPECT_TRUE(ConfigValidator().isValid(config));
}

TEST(ConfigValidatorTest, ValidPreUpgradeWriteToPort) {
  auto config = FwUtilConfig();

  // Create config with valid writeToPort preUpgrade
  std::map<std::string, FwConfig> fwConfigs;
  auto fwConfig = createValidNewFwConfig();

  std::vector<PreFirmwareOperationConfig> preUpgrade;
  PreFirmwareOperationConfig preUpgradeConfig;
  preUpgradeConfig.commandType() = "writeToPort";

  WriteToPortConfig writeToPortConfig;
  writeToPortConfig.portFile() = "/sys/kernel/debug/test_port";
  writeToPortConfig.hexByteValue() = "0x12";
  writeToPortConfig.hexOffset() = "0x100";
  preUpgradeConfig.writeToPortArgs() = writeToPortConfig;

  preUpgrade.push_back(preUpgradeConfig);
  fwConfig.preUpgrade() = preUpgrade;

  fwConfigs["bios"] = fwConfig;
  config.fwConfigs() = fwConfigs;

  EXPECT_TRUE(ConfigValidator().isValid(config));
}

TEST(ConfigValidatorTest, ValidUpgradeFlashrom) {
  auto config = FwUtilConfig();

  // Create config with valid flashrom upgrade based on real platform configs
  std::map<std::string, FwConfig> fwConfigs;
  auto fwConfig = createValidNewFwConfig();

  std::vector<UpgradeConfig> upgrade;
  UpgradeConfig upgradeConfig;
  upgradeConfig.commandType() = "flashrom";

  // Based on real platform flashrom upgrade configurations
  FlashromConfig flashromConfig;
  flashromConfig.programmer_type() = "linux_spi:dev=";
  flashromConfig.programmer() = "/run/devmap/flashes/MCB_SPI_MASTER_4_DEVICE_1";
  upgradeConfig.flashromArgs() = flashromConfig;

  upgrade.push_back(upgradeConfig);
  fwConfig.upgrade() = upgrade;

  fwConfigs["bios"] = fwConfig;
  config.fwConfigs() = fwConfigs;

  EXPECT_TRUE(ConfigValidator().isValid(config));
}

TEST(ConfigValidatorTest, ValidPostUpgradeGpioget) {
  auto config = FwUtilConfig();

  // Create config with valid gpioget postUpgrade
  std::map<std::string, FwConfig> fwConfigs;
  auto fwConfig = createValidNewFwConfig();

  std::vector<PostFirmwareOperationConfig> postUpgrade;
  PostFirmwareOperationConfig postUpgradeConfig;
  postUpgradeConfig.commandType() = "gpioget";

  // Based on typical GPIO configuration pattern
  GpiogetConfig gpiogetConfig;
  gpiogetConfig.gpioChip() = "fboss_iob_pci.gpiochip.*";
  gpiogetConfig.gpioChipPin() = "66";
  postUpgradeConfig.gpiogetArgs() = gpiogetConfig;

  postUpgrade.push_back(postUpgradeConfig);
  fwConfig.postUpgrade() = postUpgrade;

  fwConfigs["bios"] = fwConfig;
  config.fwConfigs() = fwConfigs;

  EXPECT_TRUE(ConfigValidator().isValid(config));
}

TEST(ConfigValidatorTest, ValidVerifyFlashrom) {
  auto config = FwUtilConfig();

  // Create config with valid flashrom verify based on real platform configs
  std::map<std::string, FwConfig> fwConfigs;
  auto fwConfig = createValidNewFwConfig();

  VerifyFirmwareOperationConfig verifyConfig;
  verifyConfig.commandType() = "flashrom";

  // Based on real platform flashrom verify configurations
  FlashromConfig flashromConfig;
  flashromConfig.programmer_type() = "internal";
  verifyConfig.flashromArgs() = flashromConfig;

  fwConfig.verify() = verifyConfig;

  fwConfigs["bios"] = fwConfig;
  config.fwConfigs() = fwConfigs;

  EXPECT_TRUE(ConfigValidator().isValid(config));
}

TEST(ConfigValidatorTest, ValidReadFlashrom) {
  auto config = FwUtilConfig();

  // Create config with valid flashrom read based on real platform configs
  std::map<std::string, FwConfig> fwConfigs;
  auto fwConfig = createValidNewFwConfig();

  ReadFirmwareOperationConfig readConfig;
  readConfig.commandType() = "flashrom";

  // Based on real platform flashrom read configurations
  FlashromConfig flashromConfig;
  flashromConfig.programmer_type() = "linux_spi:dev=";
  flashromConfig.programmer() = "/run/devmap/flashes/MCB_SPI_MASTER_4_DEVICE_1";
  readConfig.flashromArgs() = flashromConfig;

  fwConfig.read() = readConfig;

  fwConfigs["bios"] = fwConfig;
  config.fwConfigs() = fwConfigs;

  EXPECT_TRUE(ConfigValidator().isValid(config));
}
