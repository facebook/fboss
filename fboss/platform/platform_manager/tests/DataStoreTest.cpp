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
  dataStore.updatePmUnitInfo("/", "MCB_FAN_CPLD", 2, 13, 1);
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
  dataStore.updatePmUnitInfo("/", "MCB_FAN_CPLD", 2, std::nullopt, 1);
  EXPECT_FALSE(dataStore.getPmUnitInfo("/").version().has_value());
  dataStore.updatePmUnitInfo("/", "MCB_FAN_CPLD");
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
  // Case 1 -- Resolve when No versionedPmUnitConfigs
  dataStore.updatePmUnitInfo(slotPath, pmUnitName, 2, 1, 1);
  EXPECT_TRUE(
      dataStore.resolvePmUnitConfig(slotPath).i2cDeviceConfigs()->empty());
  // Case 2 -- Resolve to versionedPmUnitConfigs
  dataStore.updatePmUnitInfo(slotPath, pmUnitName, 3, 1, 2);
  VersionedPmUnitConfig versionedPmUnitConfig;
  versionedPmUnitConfig.productSubVersion() = 2;
  versionedPmUnitConfig.pmUnitConfig()->i2cDeviceConfigs() = {
      I2cDeviceConfig()};
  config.versionedPmUnitConfigs() = {{"SCM", {versionedPmUnitConfig}}};
  EXPECT_EQ(
      dataStore.resolvePmUnitConfig(slotPath).i2cDeviceConfigs()->size(),
      versionedPmUnitConfig.pmUnitConfig()->i2cDeviceConfigs()->size());
  // Case 3 -- Resolve to default PmUnitConfig if no version matches.
  dataStore.updatePmUnitInfo(slotPath, pmUnitName, 2, 1, 3);
  EXPECT_TRUE(
      dataStore.resolvePmUnitConfig(slotPath).i2cDeviceConfigs()->empty());
  // Case 4 -- Resolve to default PmUnitConfig if productVersion is null.
  dataStore.updatePmUnitInfo(slotPath, pmUnitName);
  EXPECT_TRUE(
      dataStore.resolvePmUnitConfig(slotPath).i2cDeviceConfigs()->empty());
}

} // namespace facebook::fboss::platform::platform_manager
