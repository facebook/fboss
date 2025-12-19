// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <gtest/gtest.h>

#include "fboss/platform/platform_manager/DataStore.h"

using namespace ::testing;

namespace facebook::fboss::platform::platform_manager {
TEST(DataStoreTest, I2CBusNum) {
  PlatformConfig config;
  DataStore dataStore(config);
  dataStore.updateI2cBusNum(std::nullopt, "CPU_BUS", 0);
  dataStore.updateI2cBusNum("/", "INCOMING@0", 1);
  dataStore.updateI2cBusNum("/SMB_SLOT@1", "INCOMING@0", 2);
  EXPECT_EQ(dataStore.getI2cBusNum(std::nullopt, "CPU_BUS"), 0);
  EXPECT_EQ(dataStore.getI2cBusNum("/", "CPU_BUS"), 0);
  EXPECT_EQ(dataStore.getI2cBusNum("/", "INCOMING@0"), 1);
  EXPECT_EQ(dataStore.getI2cBusNum("/SMB_SLOT@1", "INCOMING@0"), 2);
  EXPECT_EQ(dataStore.getI2cBusNum("/SMB_SLOT@1", "CPU_BUS"), 0);
  EXPECT_THROW(dataStore.getI2cBusNum("/", "INCOMING@1"), std::runtime_error);
}

TEST(DataStoreTest, GetPmUnitInfo) {
  PlatformConfig config;
  DataStore dataStore(config);

  PmUnitVersion version;
  version.productProductionState() = 2;
  version.productVersion() = 13;
  version.productSubVersion() = 1;

  dataStore.updatePmUnitName("/", "MCB_FAN_CPLD");
  dataStore.updatePmUnitVersion("/", version);
  EXPECT_EQ(*dataStore.getPmUnitInfo("/").name(), "MCB_FAN_CPLD");
  EXPECT_EQ(
      *dataStore.getPmUnitInfo("/").version()->productProductionState(), 2);
  EXPECT_EQ(*dataStore.getPmUnitInfo("/").version()->productVersion(), 13);
  EXPECT_EQ(*dataStore.getPmUnitInfo("/").version()->productSubVersion(), 1);
  EXPECT_THROW(dataStore.getPmUnitInfo("/SMB_SLOT@1"), std::runtime_error);
}

TEST(DataStoreTest, HasPmUnit) {
  PlatformConfig config;
  DataStore dataStore(config);

  // Case 1: No PmUnit info exists for the slot path - should return false
  EXPECT_FALSE(dataStore.hasPmUnit("/NONEXISTENT_SLOT@0"));

  // Case 2: PmUnit info exists but no presence info - should return true
  dataStore.updatePmUnitName("/SLOT_NO_PRESENCE@0", "TEST_UNIT");
  EXPECT_TRUE(dataStore.hasPmUnit("/SLOT_NO_PRESENCE@0"));

  // Case 3: PmUnit info exists with presence info indicating present - should
  // return true
  dataStore.updatePmUnitName("/SLOT_PRESENT@0", "TEST_UNIT_PRESENT");
  PresenceInfo presenceInfoPresent;
  presenceInfoPresent.isPresent() = true;
  dataStore.updatePmUnitPresenceInfo("/SLOT_PRESENT@0", presenceInfoPresent);
  EXPECT_TRUE(dataStore.hasPmUnit("/SLOT_PRESENT@0"));

  // Case 4: PmUnit info exists with presence info indicating not present -
  // should return false
  dataStore.updatePmUnitName("/SLOT_NOT_PRESENT@0", "TEST_UNIT_NOT_PRESENT");
  PresenceInfo presenceInfoNotPresent;
  presenceInfoNotPresent.isPresent() = false;
  dataStore.updatePmUnitPresenceInfo(
      "/SLOT_NOT_PRESENT@0", presenceInfoNotPresent);
  EXPECT_FALSE(dataStore.hasPmUnit("/SLOT_NOT_PRESENT@0"));

  // Case 5: PmUnit info exists with version and presence info indicating
  // present
  dataStore.updatePmUnitName("/SLOT_WITH_VERSION_PRESENT", "TEST_UNIT_VERSION");
  PmUnitVersion version;
  version.productProductionState() = 2;
  version.productVersion() = 1;
  version.productSubVersion() = 1;
  dataStore.updatePmUnitVersion("/SLOT_WITH_VERSION_PRESENT", version);
  dataStore.updatePmUnitPresenceInfo(
      "/SLOT_WITH_VERSION_PRESENT", presenceInfoPresent);
  EXPECT_TRUE(dataStore.hasPmUnit("/SLOT_WITH_VERSION_PRESENT"));

  // Case 6: PmUnit info exists with version but presence info indicating not
  // present
  dataStore.updatePmUnitName(
      "/SLOT_WITH_VERSION_NOT_PRESENT@0", "TEST_UNIT_VERSION_NOT_PRESENT");
  dataStore.updatePmUnitVersion("/SLOT_WITH_VERSION_NOT_PRESENT", version);
  dataStore.updatePmUnitPresenceInfo(
      "/SLOT_WITH_VERSION_NOT_PRESENT", presenceInfoNotPresent);
  EXPECT_FALSE(dataStore.hasPmUnit("/SLOT_WITH_VERSION_NOT_PRESENT"));
}

TEST(DataStoreTest, PmUnitUnavailableVersion) {
  PlatformConfig config;
  DataStore dataStore(config);

  dataStore.updatePmUnitName("/", "MCB_FAN_CPLD");
  EXPECT_FALSE(dataStore.getPmUnitInfo("/").version().has_value());
}

TEST(DataStoreTest, FpgaIpBlockPciDevicePath) {
  PlatformConfig config;
  DataStore dataStore(config);
  dataStore.updateSysfsPath(
      "/[SMB_DOM2_I2C_MASTER_PORT2]",
      "/sys/devices/pci0000:14/0000:14:04.0/0000:17:00.0/");
  EXPECT_EQ(
      dataStore.getSysfsPath("/[SMB_DOM2_I2C_MASTER_PORT2]"),
      "/sys/devices/pci0000:14/0000:14:04.0/0000:17:00.0/");
  EXPECT_THROW(
      dataStore.getSysfsPath("/[SMB_DOM2_I2C_MASTER_PORT3]"),
      std::runtime_error);
}

TEST(DataStoreTest, ResolvePmUnitConfig) {
  auto pmUnitName{"SCM"};
  auto slotPath{"/"};
  PlatformConfig config;
  config.pmUnitConfigs() = {{"SCM", PmUnitConfig()}};
  DataStore dataStore(config);
  dataStore.updatePmUnitName(slotPath, pmUnitName);

  // Case 1 -- Resolve when No versionedPmUnitConfigs
  PmUnitVersion version1;
  version1.productProductionState() = 2;
  version1.productVersion() = 1;
  version1.productSubVersion() = 1;

  dataStore.updatePmUnitVersion(slotPath, version1);
  EXPECT_TRUE(
      dataStore.resolvePmUnitConfig(slotPath).i2cDeviceConfigs()->empty());

  // Case 2 -- Resolve to versionedPmUnitConfigs
  PmUnitVersion version2;
  version2.productProductionState() = 3;
  version2.productVersion() = 1;
  version2.productSubVersion() = 2;

  dataStore.updatePmUnitVersion(slotPath, version2);
  VersionedPmUnitConfig versionedPmUnitConfig;
  versionedPmUnitConfig.productSubVersion() = 2;
  versionedPmUnitConfig.pmUnitConfig()->i2cDeviceConfigs() = {
      I2cDeviceConfig()};
  config.versionedPmUnitConfigs() = {{"SCM", {versionedPmUnitConfig}}};
  EXPECT_EQ(
      dataStore.resolvePmUnitConfig(slotPath).i2cDeviceConfigs()->size(),
      versionedPmUnitConfig.pmUnitConfig()->i2cDeviceConfigs()->size());

  // Case 3 -- Resolve to default PmUnitConfig if no version matches.
  PmUnitVersion version3;
  version3.productProductionState() = 2;
  version3.productVersion() = 1;
  version3.productSubVersion() = 3;

  dataStore.updatePmUnitVersion(slotPath, version3);
  EXPECT_TRUE(
      dataStore.resolvePmUnitConfig(slotPath).i2cDeviceConfigs()->empty());

  // Case 4 -- Resolve to default PmUnitConfig if productVersion is null.

  dataStore.updatePmUnitVersion(slotPath, PmUnitVersion());
  EXPECT_TRUE(
      dataStore.resolvePmUnitConfig(slotPath).i2cDeviceConfigs()->empty());
}

TEST(DataStoreTest, FirmwareVersion) {
  PlatformConfig config;
  DataStore dataStore(config);

  // Test updating and retrieving firmware versions
  dataStore.updateFirmwareVersion("TEST_CPLD", "1.2.3");
  dataStore.updateFirmwareVersion("TEST_FPGA", "4.5");
  dataStore.updateFirmwareVersion("TEST_BMC", "10.20.30");

  // Test getFirmwareVersions returns all firmware versions
  auto allVersions = dataStore.getFirmwareVersions();
  EXPECT_EQ(allVersions.size(), 3);
  EXPECT_EQ(allVersions["TEST_CPLD"], "1.2.3");
  EXPECT_EQ(allVersions["TEST_FPGA"], "4.5");
  EXPECT_EQ(allVersions["TEST_BMC"], "10.20.30");

  // Test updating existing firmware version
  dataStore.updateFirmwareVersion("TEST_CPLD", "2.0.0");
  auto newVersions = dataStore.getFirmwareVersions();
  EXPECT_EQ(newVersions["TEST_CPLD"], "2.0.0");
}

TEST(DataStoreTest, HardwareVersion) {
  PlatformConfig config;
  DataStore dataStore(config);

  // Test updating and retrieving hardware versions
  dataStore.updateHardwareVersion("chassis_eeprom_version", "5");
  dataStore.updateHardwareVersion("production_state", "GA");
  dataStore.updateHardwareVersion("production_sub_state", "V2");
  dataStore.updateHardwareVersion("variant_version", "A1");

  // Test getHardwareVersions returns all hardware versions
  auto allVersions = dataStore.getHardwareVersions();
  EXPECT_EQ(allVersions.size(), 4);
  EXPECT_EQ(allVersions["chassis_eeprom_version"], "5");
  EXPECT_EQ(allVersions["production_state"], "GA");
  EXPECT_EQ(allVersions["production_sub_state"], "V2");
  EXPECT_EQ(allVersions["variant_version"], "A1");

  // Test updating existing hardware version
  dataStore.updateHardwareVersion("chassis_eeprom_version", "6");
  allVersions = dataStore.getHardwareVersions();
  EXPECT_EQ(allVersions["chassis_eeprom_version"], "6");
}

} // namespace facebook::fboss::platform::platform_manager
