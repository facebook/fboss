// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <gtest/gtest.h>

#include "fboss/platform/platform_manager/DataStore.h"

using namespace ::testing;

namespace facebook::fboss::platform::platform_manager {
TEST(DataStoreTest, I2CBusNum) {
  DataStore dataStore;
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

TEST(DataStoreTest, GpioChipNum) {
  DataStore dataStore;
  dataStore.updateGpioChipNum("/", "gpiochip0", 0);
  EXPECT_EQ(dataStore.getGpioChipNum("/", "gpiochip0"), 0);
  EXPECT_THROW(
      dataStore.getGpioChipNum("/", "bad_gpiochip"), std::runtime_error);
}

TEST(DataStoreTest, PmUnitAtSlotPath) {
  DataStore dataStore;
  dataStore.updatePmUnitName("/", "MCB_FAN_CPLD");
  EXPECT_TRUE(dataStore.hasPmUnit("/"));
  EXPECT_FALSE(dataStore.hasPmUnit("/SMB_SLOT@1"));
  EXPECT_EQ(dataStore.getPmUnitName("/"), "MCB_FAN_CPLD");
  EXPECT_THROW(dataStore.getPmUnitName("/SMB_SLOT@1"), std::runtime_error);
}

TEST(DataStoreTest, FpgaIpBlockPciDevicePath) {
  DataStore dataStore;
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

TEST(DataStoreTest, FpgaIpBlockInstanceId) {
  DataStore dataStore;
  dataStore.updateInstanceId("/[SMB_DOM2_I2C_MASTER_PORT2]", 12);
  EXPECT_EQ(dataStore.getInstanceId("/[SMB_DOM2_I2C_MASTER_PORT2]"), 12);
  EXPECT_THROW(
      dataStore.getInstanceId("/[SMB_DOM2_I2C_MASTER_PORT3]"),
      std::runtime_error);
}
} // namespace facebook::fboss::platform::platform_manager
