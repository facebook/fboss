// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <folly/logging/xlog.h>
#include <gtest/gtest.h>
#include <thrift/lib/cpp2/protocol/Serializer.h>

#include "fboss/platform/platform_manager/ConfigValidator.h"

using namespace ::testing;
using namespace apache::thrift;
using namespace facebook::fboss::platform;
using namespace facebook::fboss::platform::platform_manager;

namespace {
SlotTypeConfig getValidSlotTypeConfig() {
  auto slotTypeConfig = SlotTypeConfig();
  slotTypeConfig.pmUnitName() = "FAN_TRAY";
  slotTypeConfig.idpromConfig_ref() = IdpromConfig();
  slotTypeConfig.idpromConfig_ref()->address_ref() = "0x14";
  return slotTypeConfig;
}

GpioLineHandle getValidGpioLineHandle() {
  auto gpioLineHandle = GpioLineHandle();
  gpioLineHandle.devicePath() = "/SMB_SLOT@0/[SMB_PCA]";
  gpioLineHandle.desiredValue() = 1;
  gpioLineHandle.lineIndex() = 4;
  return gpioLineHandle;
}

SysfsFileHandle getValidSysfsFileHandle() {
  auto sysfsFileHandle = SysfsFileHandle();
  sysfsFileHandle.devicePath() = "/SMB_SLOT@0/SMB_FPGA";
  sysfsFileHandle.presenceFileName() = "fan1_present";
  sysfsFileHandle.desiredValue() = 1;
  return sysfsFileHandle;
}

PciDeviceConfig getValidPciDeviceConfig() {
  PciDeviceConfig pciDevConfig;
  pciDevConfig.pmUnitScopedName() = "MCB_IOB";
  pciDevConfig.vendorId() = "0xab29";
  pciDevConfig.deviceId() = "0xaf29";
  pciDevConfig.subSystemVendorId() = "0xa329";
  pciDevConfig.subSystemDeviceId() = "0x1b29";
  return pciDevConfig;
}
} // namespace

TEST(ConfigValidatorTest, InvalidPlatformName) {
  auto config = PlatformConfig();
  config.platformName() = "";
  EXPECT_FALSE(ConfigValidator().isValid(config));
}

TEST(ConfigValidatorTest, InvalidRootSlotType) {
  auto config = PlatformConfig();
  config.platformName() = "MERU400BIU";
  config.rootSlotType() = "SCM_SLOT";
  config.bspKmodsRpmName() = "sample_bsp_rpm";
  config.bspKmodsRpmVersion() = "1.0.0-4";
  EXPECT_FALSE(ConfigValidator().isValid(config));
}

TEST(ConfigValidatorTest, ValidConfig) {
  auto config = PlatformConfig();
  config.platformName() = "MERU400BIU";
  config.rootSlotType() = "SCM_SLOT";
  config.slotTypeConfigs() = {{"SCM_SLOT", getValidSlotTypeConfig()}};
  auto pmUnitConfig = PmUnitConfig();
  pmUnitConfig.pluggedInSlotType() = "SCM_SLOT";
  config.pmUnitConfigs() = {{"FAN_TRAY", pmUnitConfig}};
  config.bspKmodsRpmName() = "sample_bsp_rpm";
  config.bspKmodsRpmVersion() = "1.0.0-4";
  EXPECT_TRUE(ConfigValidator().isValid(config));
}

TEST(ConfigValidatorTest, InvalidVersionedPmUnitConfigs) {
  auto config = PlatformConfig();
  config.platformName() = "MERU400BIU";
  config.rootSlotType() = "SCM_SLOT";
  config.slotTypeConfigs() = {{"SCM_SLOT", getValidSlotTypeConfig()}};
  config.bspKmodsRpmName() = "sample_bsp_rpm";
  config.bspKmodsRpmVersion() = "1.0.0-4";
  config.versionedPmUnitConfigs() = {{"FAN_TRAY", {}}};
  EXPECT_FALSE(ConfigValidator().isValid(config));
  auto versionedPmUnitConfig = VersionedPmUnitConfig();
  versionedPmUnitConfig.productSubVersion() = -1;
  config.versionedPmUnitConfigs() = {{"FAN_TRAY", {versionedPmUnitConfig}}};
  EXPECT_FALSE(ConfigValidator().isValid(config));
}

TEST(ConfigValidatorTest, ValidVersionedPmUnitConfigs) {
  auto config = PlatformConfig();
  config.platformName() = "MERU400BIU";
  config.rootSlotType() = "SCM_SLOT";
  config.slotTypeConfigs() = {{"SCM_SLOT", getValidSlotTypeConfig()}};
  config.bspKmodsRpmName() = "sample_bsp_rpm";
  config.bspKmodsRpmVersion() = "1.0.0-4";
  auto versionedPmUnitConfig = VersionedPmUnitConfig();
  versionedPmUnitConfig.pmUnitConfig()->pluggedInSlotType() = "SCM_SLOT";
  config.versionedPmUnitConfigs() = {{"FAN_TRAY", {versionedPmUnitConfig}}};
  EXPECT_TRUE(ConfigValidator().isValid(config));
}

TEST(ConfigValidatorTest, SlotTypeConfig) {
  auto slotTypeConfig = getValidSlotTypeConfig();
  EXPECT_TRUE(ConfigValidator().isValidSlotTypeConfig(slotTypeConfig));
  slotTypeConfig.pmUnitName().reset();
  EXPECT_TRUE(ConfigValidator().isValidSlotTypeConfig(slotTypeConfig));
  slotTypeConfig = getValidSlotTypeConfig();
  slotTypeConfig.idpromConfig_ref().reset();
  EXPECT_TRUE(ConfigValidator().isValidSlotTypeConfig(slotTypeConfig));
  slotTypeConfig.pmUnitName().reset();
  slotTypeConfig.idpromConfig_ref().reset();
  EXPECT_FALSE(ConfigValidator().isValidSlotTypeConfig(slotTypeConfig));
  slotTypeConfig = getValidSlotTypeConfig();
  slotTypeConfig.idpromConfig_ref()->address_ref() = "0xK4";
  EXPECT_FALSE(ConfigValidator().isValidSlotTypeConfig(slotTypeConfig));
}

TEST(ConfigValidatorTest, SlotConfig) {
  auto slotConfig = SlotConfig();
  slotConfig.presenceDetection() = PresenceDetection();
  slotConfig.presenceDetection()->gpioLineHandle() = getValidGpioLineHandle();
  EXPECT_FALSE(ConfigValidator().isValidSlotConfig(slotConfig));
  slotConfig.slotType() = "MCB_SLOT";
  EXPECT_TRUE(ConfigValidator().isValidSlotConfig(slotConfig));
}

TEST(ConfigValidatorTest, PresenceDetection) {
  auto presenceDetection = PresenceDetection();
  EXPECT_FALSE(ConfigValidator().isValidPresenceDetection(presenceDetection));
  presenceDetection.gpioLineHandle() = getValidGpioLineHandle();
  EXPECT_TRUE(ConfigValidator().isValidPresenceDetection(presenceDetection));
  presenceDetection.gpioLineHandle()->devicePath() = "";
  EXPECT_FALSE(ConfigValidator().isValidPresenceDetection(presenceDetection));
  presenceDetection.gpioLineHandle() = getValidGpioLineHandle();
  presenceDetection.gpioLineHandle()->desiredValue() = -1;
  EXPECT_FALSE(ConfigValidator().isValidPresenceDetection(presenceDetection));
  presenceDetection.gpioLineHandle() = getValidGpioLineHandle();
  presenceDetection.sysfsFileHandle() = getValidSysfsFileHandle();
  EXPECT_FALSE(ConfigValidator().isValidPresenceDetection(presenceDetection));
  presenceDetection.gpioLineHandle().reset();
  EXPECT_TRUE(ConfigValidator().isValidPresenceDetection(presenceDetection));
  presenceDetection.sysfsFileHandle()->devicePath() = "";
  EXPECT_FALSE(ConfigValidator().isValidPresenceDetection(presenceDetection));
  presenceDetection.sysfsFileHandle() = getValidSysfsFileHandle();
  presenceDetection.sysfsFileHandle()->desiredValue() = 0;
  EXPECT_FALSE(ConfigValidator().isValidPresenceDetection(presenceDetection));
  presenceDetection.sysfsFileHandle() = getValidSysfsFileHandle();
  presenceDetection.sysfsFileHandle()->presenceFileName() = "";
  EXPECT_FALSE(ConfigValidator().isValidPresenceDetection(presenceDetection));
}

TEST(ConfigValidatorTest, FpgaIpBlockConfig) {
  auto fpgaIpBlockConfig = FpgaIpBlockConfig{};
  EXPECT_FALSE(ConfigValidator().isValidFpgaIpBlockConfig(fpgaIpBlockConfig));
  fpgaIpBlockConfig.iobufOffset() = "0xab29";
  fpgaIpBlockConfig.csrOffset() = "0xaf29";
  EXPECT_FALSE(ConfigValidator().isValidFpgaIpBlockConfig(fpgaIpBlockConfig));
  fpgaIpBlockConfig.pmUnitScopedName() = "PM_UNIT";
  EXPECT_TRUE(ConfigValidator().isValidFpgaIpBlockConfig(fpgaIpBlockConfig));
  fpgaIpBlockConfig.csrOffset() = "0xaF2";
  EXPECT_FALSE(ConfigValidator().isValidFpgaIpBlockConfig(fpgaIpBlockConfig));
  fpgaIpBlockConfig.csrOffset() = "0xaf2";
  EXPECT_TRUE(ConfigValidator().isValidFpgaIpBlockConfig(fpgaIpBlockConfig));
  fpgaIpBlockConfig.iobufOffset() = "";
  fpgaIpBlockConfig.csrOffset() = "0xaf20";
  EXPECT_TRUE(ConfigValidator().isValidFpgaIpBlockConfig(fpgaIpBlockConfig));
  fpgaIpBlockConfig.pmUnitScopedName() = "pm_unit";
  EXPECT_FALSE(ConfigValidator().isValidFpgaIpBlockConfig(fpgaIpBlockConfig));
}

TEST(ConfigValidatorTest, PciDeviceConfig) {
  auto pciDevConfig = PciDeviceConfig{};
  EXPECT_FALSE(ConfigValidator().isValidPciDeviceConfig(pciDevConfig));
  pciDevConfig.vendorId() = "0xAb29";
  pciDevConfig.deviceId() = "0xaf2x";
  EXPECT_FALSE(ConfigValidator().isValidPciDeviceConfig(pciDevConfig));
  pciDevConfig.vendorId() = "0xab29";
  pciDevConfig.deviceId() = "0xaf29";
  pciDevConfig.subSystemVendorId() = "0xa3F9";
  EXPECT_FALSE(ConfigValidator().isValidPciDeviceConfig(pciDevConfig));
  pciDevConfig.subSystemVendorId() = "0xa329";
  pciDevConfig.subSystemDeviceId() = "0x1b";
  EXPECT_FALSE(ConfigValidator().isValidPciDeviceConfig(pciDevConfig));
  pciDevConfig.vendorId() = "0xab29";
  pciDevConfig.deviceId() = "0xaf29";
  EXPECT_FALSE(ConfigValidator().isValidPciDeviceConfig(pciDevConfig));
  pciDevConfig.pmUnitScopedName() = "PM_UNIT";
  pciDevConfig.subSystemVendorId() = "0xa329";
  pciDevConfig.subSystemDeviceId() = "0x1b29";
  EXPECT_TRUE(ConfigValidator().isValidPciDeviceConfig(pciDevConfig));
  pciDevConfig.subSystemDeviceId() = "0x1b29";
  EXPECT_TRUE(ConfigValidator().isValidPciDeviceConfig(pciDevConfig));
  auto fpgaIpBlockConfig = FpgaIpBlockConfig{};
  fpgaIpBlockConfig.iobufOffset() = "0xab29";
  fpgaIpBlockConfig.csrOffset() = "0xaf29";
  fpgaIpBlockConfig.pmUnitScopedName() = "MCB_WDOG_1";
  pciDevConfig.watchdogConfigs()->push_back(fpgaIpBlockConfig);
  fpgaIpBlockConfig.iobufOffset() = "0xab20";
  fpgaIpBlockConfig.csrOffset() = "0xaf39";
  fpgaIpBlockConfig.pmUnitScopedName() = "MCB_WDOG_1";
  pciDevConfig.watchdogConfigs()->push_back(fpgaIpBlockConfig);
  EXPECT_FALSE(ConfigValidator().isValidPciDeviceConfig(pciDevConfig));
  pciDevConfig.watchdogConfigs()->pop_back();
  fpgaIpBlockConfig.pmUnitScopedName() = "MCB_WDOG_2";
  pciDevConfig.watchdogConfigs()->push_back(fpgaIpBlockConfig);
  EXPECT_TRUE(ConfigValidator().isValidPciDeviceConfig(pciDevConfig));
  pciDevConfig.pmUnitScopedName() = "pm_unit";
  EXPECT_FALSE(ConfigValidator().isValidPciDeviceConfig(pciDevConfig));
}

TEST(ConfigValidatorTest, XcvrCtrlConfig) {
  auto pciDevConfig = getValidPciDeviceConfig();
  XcvrCtrlConfig xcvrCtrlConfig;
  xcvrCtrlConfig.portNumber() = 3;
  xcvrCtrlConfig.fpgaIpBlockConfig()->pmUnitScopedName() =
      "SMB_DOM1_XCVR_CTRL_PORT_3";
  xcvrCtrlConfig.fpgaIpBlockConfig()->iobufOffset() = "0xab29";
  xcvrCtrlConfig.fpgaIpBlockConfig()->csrOffset() = "0xaf29";
  xcvrCtrlConfig.fpgaIpBlockConfig()->deviceName() = "xcvr_ctrl";
  pciDevConfig.xcvrCtrlConfigs() = {xcvrCtrlConfig};
  EXPECT_TRUE(ConfigValidator().isValidPciDeviceConfig(pciDevConfig));
  xcvrCtrlConfig.fpgaIpBlockConfig()->deviceName() = "osfp_xcvr";
  pciDevConfig.xcvrCtrlConfigs() = {xcvrCtrlConfig};
  EXPECT_FALSE(ConfigValidator().isValidPciDeviceConfig(pciDevConfig));
}

TEST(ConfigValidator, InfoRomConfig) {
  auto pciDevConfig = getValidPciDeviceConfig();
  auto fpgaIpBlockConfig = FpgaIpBlockConfig{};
  fpgaIpBlockConfig.pmUnitScopedName() = "MCB_IOB_INFO_ROM";
  fpgaIpBlockConfig.iobufOffset() = "0xab29";
  fpgaIpBlockConfig.csrOffset() = "0xaf29";
  fpgaIpBlockConfig.deviceName() = "fpga_info_iob";
  pciDevConfig.infoRomConfigs() = {fpgaIpBlockConfig};
  EXPECT_TRUE(ConfigValidator().isValidPciDeviceConfig(pciDevConfig));
  fpgaIpBlockConfig.deviceName() = "info_iob";
  pciDevConfig.infoRomConfigs() = {fpgaIpBlockConfig};
  EXPECT_FALSE(ConfigValidator().isValidPciDeviceConfig(pciDevConfig));
  fpgaIpBlockConfig.deviceName() = "fpga_info_bad_name";
  pciDevConfig.infoRomConfigs() = {fpgaIpBlockConfig};
  EXPECT_FALSE(ConfigValidator().isValidPciDeviceConfig(pciDevConfig));
}

TEST(ConfigValidatorTest, SpiDeviceConfig) {
  SpiDeviceConfig spiDevConfig;
  // Invalid pmUnitScopedName
  spiDevConfig.pmUnitScopedName() = "mCB_SPI_MASTER_1_DEVICE_1";
  spiDevConfig.modalias() = "spidev";
  spiDevConfig.chipSelect() = 0;
  spiDevConfig.maxSpeedHz() = 25000000;
  EXPECT_FALSE(ConfigValidator().isValidSpiDeviceConfigs({spiDevConfig}));
  spiDevConfig.pmUnitScopedName() = "MCB_SPI_MASTER_1_DEVICE_1";
  EXPECT_TRUE(ConfigValidator().isValidSpiDeviceConfigs({spiDevConfig}));
  // Invalid modalias
  spiDevConfig.modalias() = "something-invalid";
  EXPECT_FALSE(ConfigValidator().isValidSpiDeviceConfigs({spiDevConfig}));
  // Exceed charLimit modalias
  spiDevConfig.modalias() = std::string(NAME_MAX + 1, 'a');
  EXPECT_FALSE(ConfigValidator().isValidSpiDeviceConfigs({spiDevConfig}));
  // Out of ranged chipSelect
  spiDevConfig.modalias() = "spidev";
  spiDevConfig.chipSelect() = 1;
  EXPECT_FALSE(ConfigValidator().isValidSpiDeviceConfigs({spiDevConfig}));
  // Duplicate chipselects
  auto spiDevConfigCopy = spiDevConfig;
  spiDevConfigCopy.chipSelect() = 0;
  spiDevConfig.chipSelect() = 0;
  EXPECT_FALSE(ConfigValidator().isValidSpiDeviceConfigs(
      {spiDevConfigCopy, spiDevConfig}));
  // Empty spiDeviceConfigs
  EXPECT_FALSE(ConfigValidator().isValidSpiDeviceConfigs({}));
}

TEST(ConfigValidatorTest, I2cDeviceConfig) {
  auto i2cConfig = I2cDeviceConfig{};
  EXPECT_FALSE(ConfigValidator().isValidI2cDeviceConfig(i2cConfig));
  i2cConfig.address_ref() = "029";
  EXPECT_FALSE(ConfigValidator().isValidI2cDeviceConfig(i2cConfig));
  i2cConfig.address_ref() = "29";
  EXPECT_FALSE(ConfigValidator().isValidI2cDeviceConfig(i2cConfig));
  i2cConfig.address_ref() = "0x";
  EXPECT_FALSE(ConfigValidator().isValidI2cDeviceConfig(i2cConfig));
  i2cConfig.address_ref() = "0x2F";
  EXPECT_FALSE(ConfigValidator().isValidI2cDeviceConfig(i2cConfig));
  i2cConfig.address_ref() = "0x2f";
  EXPECT_FALSE(ConfigValidator().isValidI2cDeviceConfig(i2cConfig));
  i2cConfig.pmUnitScopedName() = "PM_UNIT";
  EXPECT_TRUE(ConfigValidator().isValidI2cDeviceConfig(i2cConfig));
  i2cConfig.address_ref() = "0x20";
  EXPECT_TRUE(ConfigValidator().isValidI2cDeviceConfig(i2cConfig));
  i2cConfig.pmUnitScopedName() = "pm_unit";
  EXPECT_FALSE(ConfigValidator().isValidI2cDeviceConfig(i2cConfig));
}

TEST(ConfigValidatorTest, Symlink) {
  EXPECT_FALSE(ConfigValidator().isValidSymlink(""));
  EXPECT_FALSE(ConfigValidator().isValidSymlink("/run/devma/sensors/sensor1"));
  EXPECT_FALSE(ConfigValidator().isValidSymlink("run/devmap/sensors/sensor2"));
  EXPECT_FALSE(ConfigValidator().isValidSymlink("/run/devmap/sensor/sensor2"));
  EXPECT_FALSE(ConfigValidator().isValidSymlink("/run/devmap/gpio/sensor2"));
  EXPECT_FALSE(ConfigValidator().isValidSymlink("/run/devmap/eeproms"));
  EXPECT_FALSE(ConfigValidator().isValidSymlink("/run/devmap/eeproms/"));
  EXPECT_TRUE(ConfigValidator().isValidSymlink("/run/devmap/eeproms/EEPROM"));
  EXPECT_TRUE(ConfigValidator().isValidSymlink("/run/devmap/sensors/sensor1"));
  EXPECT_TRUE(ConfigValidator().isValidSymlink("/run/devmap/i2c-busses/bus"));
}

TEST(ConfigValidatorTest, DevicePath) {
  EXPECT_FALSE(ConfigValidator().isValidDevicePath("/[]"));
  EXPECT_FALSE(ConfigValidator().isValidDevicePath("/SCM_SLOT@1/[a"));
  EXPECT_FALSE(ConfigValidator().isValidDevicePath("/SMB_SLOT@1/SCM_SLOT/[a]"));
  EXPECT_TRUE(ConfigValidator().isValidDevicePath("/[chassis_eeprom]"));
  EXPECT_TRUE(ConfigValidator().isValidDevicePath("/SMB_SLOT@0/[MCB_MUX_1]"));
  EXPECT_TRUE(
      ConfigValidator().isValidDevicePath("/SMB_SLOT@0/SCM_SLOT@1/[sensor]"));
  EXPECT_TRUE(ConfigValidator().isValidDevicePath("/SMB_FRU_SLOT@0/[idprom]"));
  EXPECT_FALSE(
      ConfigValidator().isValidDevicePath("/COME@SCM_SLOT@0/[idprom]"));
  EXPECT_FALSE(
      ConfigValidator().isValidDevicePath("/COME_SLOT@SCM_SLOT@0/[SCM_MUX_5]"));
}

TEST(ConfigValidatorTest, BspRpm) {
  EXPECT_FALSE(ConfigValidator().isValidBspKmodsRpmVersion(""));
  EXPECT_FALSE(ConfigValidator().isValidBspKmodsRpmVersion("5.4.6"));
  EXPECT_TRUE(ConfigValidator().isValidBspKmodsRpmVersion("5.4.6-1"));
  EXPECT_TRUE(ConfigValidator().isValidBspKmodsRpmVersion("11.44.63-14"));
}
