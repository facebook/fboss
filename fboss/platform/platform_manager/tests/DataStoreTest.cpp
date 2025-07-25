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

TEST(DataStoreTest, PmUnitAtSlotPath) {
  PlatformConfig config;
  DataStore dataStore(config);

  PmUnitVersion version;
  version.productProductionState() = 2;
  version.productVersion() = 13;
  version.productSubVersion() = 1;

  dataStore.updatePmUnitName("/", "MCB_FAN_CPLD");
  dataStore.updatePmUnitVersion("/", version);
  EXPECT_TRUE(dataStore.hasPmUnit("/"));
  EXPECT_FALSE(dataStore.hasPmUnit("/SMB_SLOT@1"));
  EXPECT_EQ(*dataStore.getPmUnitInfo("/").name(), "MCB_FAN_CPLD");
  EXPECT_EQ(
      *dataStore.getPmUnitInfo("/").version()->productProductionState(), 2);
  EXPECT_EQ(*dataStore.getPmUnitInfo("/").version()->productVersion(), 13);
  EXPECT_EQ(*dataStore.getPmUnitInfo("/").version()->productSubVersion(), 1);
  EXPECT_THROW(dataStore.getPmUnitInfo("/SMB_SLOT@1"), std::runtime_error);
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

} // namespace facebook::fboss::platform::platform_manager
