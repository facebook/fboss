// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/platform/platform_manager/utilities/ConfigDiffer.h"

#include <gtest/gtest.h>

using namespace facebook::fboss::platform::platform_manager;

class ConfigDifferTest : public ::testing::Test {
 protected:
  ConfigDiffer differ_;
};

TEST_F(ConfigDifferTest, ComparePmUnitConfigs_Identical) {
  PmUnitConfig config1;
  config1.pluggedInSlotType() = "TEST_SLOT";

  PmUnitConfig config2;
  config2.pluggedInSlotType() = "TEST_SLOT";

  differ_.comparePmUnitConfigs(config1, config2, "TEST_UNIT", "default → v1");
  auto diffs = differ_.getDiffs();
  EXPECT_TRUE(diffs.empty());
}

TEST_F(ConfigDifferTest, ComparePmUnitConfigs_DifferentSlotType) {
  PmUnitConfig config1;
  config1.pluggedInSlotType() = "SLOT_TYPE_A";

  PmUnitConfig config2;
  config2.pluggedInSlotType() = "SLOT_TYPE_B";

  // Only i2cDeviceConfigs are compared now, other fields are ignored
  differ_.comparePmUnitConfigs(config1, config2, "TEST_UNIT", "default → v1");
  auto diffs = differ_.getDiffs();
  EXPECT_TRUE(diffs.empty());
}

TEST_F(ConfigDifferTest, ComparePmUnitConfigs_AddedI2cDevice) {
  PmUnitConfig config1;
  config1.pluggedInSlotType() = "TEST_SLOT";

  PmUnitConfig config2;
  config2.pluggedInSlotType() = "TEST_SLOT";

  I2cDeviceConfig device;
  device.pmUnitScopedName() = "sensor1";
  device.busName() = "INCOMING@0";
  device.address() = "0x12";
  device.kernelDeviceName() = "lm75";

  std::vector<I2cDeviceConfig> devices = {device};
  config2.i2cDeviceConfigs() = devices;

  differ_.comparePmUnitConfigs(config1, config2, "TEST_UNIT", "default → v1");
  auto diffs = differ_.getDiffs();
  ASSERT_EQ(diffs.size(), 1);
  EXPECT_EQ(diffs[0].type, DiffType::ADDED);
  EXPECT_EQ(diffs[0].deviceName, "sensor1");
}

TEST_F(ConfigDifferTest, ComparePmUnitConfigs_RemovedI2cDevice) {
  PmUnitConfig config1;
  config1.pluggedInSlotType() = "TEST_SLOT";

  I2cDeviceConfig device;
  device.pmUnitScopedName() = "sensor1";
  device.busName() = "INCOMING@0";
  device.address() = "0x12";
  device.kernelDeviceName() = "lm75";

  std::vector<I2cDeviceConfig> devices = {device};
  config1.i2cDeviceConfigs() = devices;

  PmUnitConfig config2;
  config2.pluggedInSlotType() = "TEST_SLOT";

  differ_.comparePmUnitConfigs(config1, config2, "TEST_UNIT", "default → v1");
  auto diffs = differ_.getDiffs();
  ASSERT_EQ(diffs.size(), 1);
  EXPECT_EQ(diffs[0].type, DiffType::REMOVED);
  EXPECT_EQ(diffs[0].deviceName, "sensor1");
}

TEST_F(ConfigDifferTest, ComparePmUnitConfigs_ModifiedI2cDevice) {
  PmUnitConfig config1;
  config1.pluggedInSlotType() = "TEST_SLOT";

  I2cDeviceConfig device1;
  device1.pmUnitScopedName() = "sensor1";
  device1.busName() = "INCOMING@0";
  device1.address() = "0x12";
  device1.kernelDeviceName() = "lm75";

  std::vector<I2cDeviceConfig> devices1 = {device1};
  config1.i2cDeviceConfigs() = devices1;

  PmUnitConfig config2;
  config2.pluggedInSlotType() = "TEST_SLOT";

  I2cDeviceConfig device2;
  device2.pmUnitScopedName() = "sensor1";
  device2.busName() = "INCOMING@0";
  device2.address() = "0x13";
  device2.kernelDeviceName() = "lm75";

  std::vector<I2cDeviceConfig> devices2 = {device2};
  config2.i2cDeviceConfigs() = devices2;

  differ_.comparePmUnitConfigs(config1, config2, "TEST_UNIT", "default → v1");
  auto diffs = differ_.getDiffs();
  ASSERT_EQ(diffs.size(), 1);
  EXPECT_EQ(diffs[0].type, DiffType::MODIFIED);
  EXPECT_EQ(diffs[0].deviceName, "sensor1");
  EXPECT_EQ(diffs[0].fieldName, "address");
}

TEST_F(ConfigDifferTest, CompareVersionedConfigs_NoPmUnit) {
  PlatformConfig config;
  config.platformName() = "TEST_PLATFORM";

  differ_.compareVersionedConfigs(config, "NonExistent");
  auto diffs = differ_.getDiffs();
  EXPECT_TRUE(diffs.empty());
}

TEST_F(ConfigDifferTest, CompareVersionedConfigs_SingleVersion) {
  PlatformConfig config;
  config.platformName() = "TEST_PLATFORM";

  VersionedPmUnitConfig versionedConfig;
  versionedConfig.productSubVersion() = 1;
  PmUnitConfig pmUnitConfig;
  pmUnitConfig.pluggedInSlotType() = "TEST_SLOT";
  versionedConfig.pmUnitConfig() = pmUnitConfig;

  std::map<std::string, std::vector<VersionedPmUnitConfig>> versionedConfigs;
  versionedConfigs["TEST_UNIT"] = {versionedConfig};
  config.versionedPmUnitConfigs() = versionedConfigs;

  differ_.compareVersionedConfigs(config, "TEST_UNIT");
  auto diffs = differ_.getDiffs();
  EXPECT_TRUE(diffs.empty());
}

TEST_F(ConfigDifferTest, CompareVersionedConfigs_WithDefaultConfig) {
  PlatformConfig config;
  config.platformName() = "TEST_PLATFORM";

  // Set up default config
  PmUnitConfig defaultConfig;
  defaultConfig.pluggedInSlotType() = "TEST_SLOT";
  I2cDeviceConfig defaultDevice;
  defaultDevice.pmUnitScopedName() = "sensor1";
  defaultDevice.busName() = "INCOMING@0";
  defaultDevice.address() = "0x12";
  defaultDevice.kernelDeviceName() = "lm75";
  std::vector<I2cDeviceConfig> defaultDevices = {defaultDevice};
  defaultConfig.i2cDeviceConfigs() = defaultDevices;

  std::map<std::string, PmUnitConfig> pmUnitConfigs;
  pmUnitConfigs["TEST_UNIT"] = defaultConfig;
  config.pmUnitConfigs() = pmUnitConfigs;

  // Set up versioned config with different i2cDeviceConfig
  VersionedPmUnitConfig versionedConfig;
  versionedConfig.productSubVersion() = 1;
  PmUnitConfig pmUnitConfig;
  pmUnitConfig.pluggedInSlotType() = "TEST_SLOT";
  I2cDeviceConfig device;
  device.pmUnitScopedName() = "sensor1";
  device.busName() = "INCOMING@0";
  device.address() = "0x13"; // Different address
  device.kernelDeviceName() = "lm75";
  std::vector<I2cDeviceConfig> devices = {device};
  pmUnitConfig.i2cDeviceConfigs() = devices;
  versionedConfig.pmUnitConfig() = pmUnitConfig;

  std::map<std::string, std::vector<VersionedPmUnitConfig>> versionedConfigs;
  versionedConfigs["TEST_UNIT"] = {versionedConfig};
  config.versionedPmUnitConfigs() = versionedConfigs;

  differ_.compareVersionedConfigs(config, "TEST_UNIT");
  auto diffs = differ_.getDiffs();
  ASSERT_EQ(diffs.size(), 1);
  EXPECT_EQ(diffs[0].type, DiffType::MODIFIED);
  EXPECT_EQ(diffs[0].deviceName, "sensor1");
  EXPECT_EQ(diffs[0].fieldName, "address");
}

TEST_F(ConfigDifferTest, CompareSlotConfigs_Added) {
  PmUnitConfig config1;
  config1.pluggedInSlotType() = "TEST_SLOT";

  PmUnitConfig config2;
  config2.pluggedInSlotType() = "TEST_SLOT";

  SlotConfig slotConfig;
  slotConfig.slotType() = "PIM_SLOT";
  slotConfig.outgoingI2cBusNames() = std::vector<std::string>{"INCOMING@0"};

  std::map<std::string, SlotConfig> slotConfigs;
  slotConfigs["PIM_SLOT@0"] = slotConfig;
  config2.outgoingSlotConfigs() = slotConfigs;

  // Only i2cDeviceConfigs are compared now, slotConfigs are ignored
  differ_.comparePmUnitConfigs(config1, config2, "TEST_UNIT", "default → v1");
  auto diffs = differ_.getDiffs();
  EXPECT_TRUE(diffs.empty());
}

TEST_F(ConfigDifferTest, ComparePciDeviceConfigs_Added) {
  PmUnitConfig config1;
  config1.pluggedInSlotType() = "TEST_SLOT";

  PmUnitConfig config2;
  config2.pluggedInSlotType() = "TEST_SLOT";

  PciDeviceConfig pciConfig;
  pciConfig.pmUnitScopedName() = "fpga1";
  pciConfig.vendorId() = "1d9b";
  pciConfig.deviceId() = "0011";

  std::vector<PciDeviceConfig> pciConfigs = {pciConfig};
  config2.pciDeviceConfigs() = pciConfigs;

  // Only i2cDeviceConfigs are compared now, pciDeviceConfigs are ignored
  differ_.comparePmUnitConfigs(config1, config2, "TEST_UNIT", "default → v1");
  auto diffs = differ_.getDiffs();
  EXPECT_TRUE(diffs.empty());
}

TEST_F(ConfigDifferTest, CompareEmbeddedSensorConfigs_Modified) {
  PmUnitConfig config1;
  config1.pluggedInSlotType() = "TEST_SLOT";

  EmbeddedSensorConfig sensor1;
  sensor1.pmUnitScopedName() = "cpu_sensor";
  sensor1.sysfsPath() = "/sys/bus/i2c/devices/0-001a/hwmon";

  std::vector<EmbeddedSensorConfig> sensors1 = {sensor1};
  config1.embeddedSensorConfigs() = sensors1;

  PmUnitConfig config2;
  config2.pluggedInSlotType() = "TEST_SLOT";

  EmbeddedSensorConfig sensor2;
  sensor2.pmUnitScopedName() = "cpu_sensor";
  sensor2.sysfsPath() = "/sys/bus/i2c/devices/0-001b/hwmon";

  std::vector<EmbeddedSensorConfig> sensors2 = {sensor2};
  config2.embeddedSensorConfigs() = sensors2;

  // Only i2cDeviceConfigs are compared now, embeddedSensorConfigs are ignored
  differ_.comparePmUnitConfigs(config1, config2, "TEST_UNIT", "default → v1");
  auto diffs = differ_.getDiffs();
  EXPECT_TRUE(diffs.empty());
}
