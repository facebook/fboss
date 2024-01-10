// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "fboss/platform/platform_manager/DevicePathResolver.h"

using namespace ::testing;
using namespace facebook::fboss::platform::platform_manager;

class MockI2cExplorer : public I2cExplorer {
 public:
  explicit MockI2cExplorer() : I2cExplorer() {}
  MOCK_METHOD(bool, isI2cDevicePresent, (uint16_t, const I2cAddr&), (const));
};

class DevicePathResolverTest : public testing::Test {
 public:
  void SetUp() override {
    auto idpromConfig = IdpromConfig();
    idpromConfig.busName() = "INCOMING@2";
    idpromConfig.address() = "0x11";
    idpromConfig.kernelDeviceName() = "24c02";
    auto smbSlotTypeConfig = SlotTypeConfig();
    smbSlotTypeConfig.idpromConfig() = idpromConfig;

    auto sensorDevice = I2cDeviceConfig();
    sensorDevice.busName() = "SMB_IOB_I2C_0";
    sensorDevice.address() = "0x54";
    sensorDevice.kernelDeviceName() = "lm75";
    sensorDevice.pmUnitScopedName() = "inlet_sensor";
    auto eepromDevice = I2cDeviceConfig();
    eepromDevice.busName() = "SMB_MUX_A@4";
    eepromDevice.address() = "0x01";
    eepromDevice.kernelDeviceName() = "24c02";
    eepromDevice.pmUnitScopedName() = "chassis_eeprom";

    auto smbUnitConfig = PmUnitConfig();
    smbUnitConfig.pluggedInSlotType() = "SMB_SLOT";
    smbUnitConfig.i2cDeviceConfigs() =
        std::vector<I2cDeviceConfig>{sensorDevice, eepromDevice};
    auto mcbUnitConfig = PmUnitConfig();
    mcbUnitConfig.pluggedInSlotType() = "MCB_SLOT";

    platformConfig_ = PlatformConfig();
    platformConfig_.pmUnitConfigs()["SMB"] = smbUnitConfig;
    platformConfig_.pmUnitConfigs()["MCB"] = mcbUnitConfig;
    platformConfig_.slotTypeConfigs()["SMB_SLOT"] = smbSlotTypeConfig;
    platformConfig_.slotTypeConfigs()["MCB_SLOT"] = SlotTypeConfig();
  }

  PlatformConfig platformConfig_;
  DataStore dataStore_;
  MockI2cExplorer i2cExplorer_;
};

TEST_F(DevicePathResolverTest, ResolveI2cDevicePath) {
  DevicePathResolver resolver(platformConfig_, dataStore_, i2cExplorer_);

  EXPECT_THROW(
      resolver.resolveI2cDevicePath("/SMB_SLOT@1/[IDPROM]"),
      std::runtime_error);
  EXPECT_THROW(
      resolver.resolveI2cDevicePath("SMB_SLOT@1/[inlet_sensor]"),
      std::runtime_error);

  dataStore_.updatePmUnitName("/MCB_SLOT@0", "MCB");
  dataStore_.updatePmUnitName("/MCB_SLOT@0/SMB_SLOT@1", "SMB");
  dataStore_.updateI2cBusNum("/MCB_SLOT@0/SMB_SLOT@1", "INCOMING@2", 0);
  dataStore_.updateI2cBusNum("/MCB_SLOT@0/SMB_SLOT@1", "SMB_IOB_I2C_0", 1);
  dataStore_.updateI2cBusNum("/MCB_SLOT@0/SMB_SLOT@1", "SMB_MUX_A@4", 2);
  EXPECT_CALL(i2cExplorer_, isI2cDevicePresent(_, _))
      .WillRepeatedly(Return(true));
  EXPECT_EQ(
      resolver.resolveI2cDevicePath("/MCB_SLOT@0/SMB_SLOT@1/[IDPROM]"),
      "/sys/bus/i2c/devices/0-0011");
  EXPECT_EQ(
      resolver.resolveI2cDevicePath("/MCB_SLOT@0/SMB_SLOT@1/[inlet_sensor]"),
      "/sys/bus/i2c/devices/1-0054");
  EXPECT_EQ(
      resolver.resolveI2cDevicePath("/MCB_SLOT@0/SMB_SLOT@1/[chassis_eeprom]"),
      "/sys/bus/i2c/devices/2-0001");
  EXPECT_THROW(
      resolver.resolveI2cDevicePath(
          "/MCB_SLOT@0/SMB_SLOT@1/[non_existent_device]"),
      std::runtime_error);
  EXPECT_THROW(
      resolver.resolveI2cDevicePath("/MCB_SLOT@0/[IDPROM]"),
      std::runtime_error);

  EXPECT_CALL(i2cExplorer_, isI2cDevicePresent(_, _))
      .WillRepeatedly(Return(false));
  EXPECT_THROW(
      resolver.resolveI2cDevicePath("/MCB_SLOT@0/SMB_SLOT@1/[IDPROM]"),
      std::runtime_error);
  EXPECT_THROW(
      resolver.resolveI2cDevicePath("/MCB_SLOT@0/SMB_SLOT@1/[inlet_sensor]"),
      std::runtime_error);
  EXPECT_THROW(
      resolver.resolveI2cDevicePath("/MCB_SLOT@0/SMB_SLOT@1/[chassis_eeprom]"),
      std::runtime_error);
}
