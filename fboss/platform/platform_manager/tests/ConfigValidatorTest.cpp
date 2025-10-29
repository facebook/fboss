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
  slotTypeConfig.idpromConfig() = IdpromConfig();
  slotTypeConfig.idpromConfig()->address() = "0x14";
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
  config.bspKmodsRpmName() = "sample_bsp_kmods";
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
  config.bspKmodsRpmName() = "sample_bsp_kmods";
  config.bspKmodsRpmVersion() = "1.0.0-4";
  EXPECT_TRUE(ConfigValidator().isValid(config));
}

TEST(ConfigValidatorTest, InvalidVersionedPmUnitConfigs) {
  auto config = PlatformConfig();
  // Add pmUnitConfig to make pmUnitConfigName reference valid
  auto pmUnitConfig = PmUnitConfig();
  pmUnitConfig.pluggedInSlotType() = "SCM_SLOT";
  config.pmUnitConfigs() = {{"FAN_TRAY", pmUnitConfig}};

  config.platformName() = "MERU400BIU";
  config.rootSlotType() = "SCM_SLOT";
  config.slotTypeConfigs() = {{"SCM_SLOT", getValidSlotTypeConfig()}};
  config.bspKmodsRpmName() = "sample_bsp_kmods";
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
  // Add pmUnitConfig to make pmUnitConfigName reference valid
  auto pmUnitConfig = PmUnitConfig();
  pmUnitConfig.pluggedInSlotType() = "SCM_SLOT";
  config.pmUnitConfigs() = {{"FAN_TRAY", pmUnitConfig}};

  config.platformName() = "MERU400BIU";
  config.rootSlotType() = "SCM_SLOT";
  config.slotTypeConfigs() = {{"SCM_SLOT", getValidSlotTypeConfig()}};
  config.bspKmodsRpmName() = "sample_bsp_kmods";
  config.bspKmodsRpmVersion() = "1.0.0-4";
  auto versionedPmUnitConfig = VersionedPmUnitConfig();
  versionedPmUnitConfig.pmUnitConfig()->pluggedInSlotType() = "SCM_SLOT";
  config.versionedPmUnitConfigs() = {{"FAN_TRAY", {versionedPmUnitConfig}}};
  EXPECT_TRUE(ConfigValidator().isValid(config));
}

TEST(ConfigValidatorTest, PmUnitNameReferentialIntegrity) {
  auto config = PlatformConfig();
  config.platformName() = "MERU400BIU";
  config.rootSlotType() = "SCM_SLOT";
  config.bspKmodsRpmName() = "sample_bsp_kmods";
  config.bspKmodsRpmVersion() = "1.0.0-4";

  // Add a valid pmUnitConfig
  auto pmUnitConfig = PmUnitConfig();
  pmUnitConfig.pluggedInSlotType() = "SCM_SLOT";
  config.pmUnitConfigs() = {{"SCM", pmUnitConfig}};

  // Test 1: slotTypeConfig.pmUnitName references non-existent PMUnit name
  auto slotTypeConfig = SlotTypeConfig();
  slotTypeConfig.pmUnitName() = "NON_EXISTENT_PMUNIT";
  slotTypeConfig.idpromConfig() = IdpromConfig();
  slotTypeConfig.idpromConfig()->address() = "0x14";
  config.slotTypeConfigs() = {{"SCM_SLOT", slotTypeConfig}};
  EXPECT_FALSE(ConfigValidator().isValid(config));

  // Test 2: slotTypeConfig.pmUnitName references existing PMUnit name
  slotTypeConfig.pmUnitName() = "SCM";
  config.slotTypeConfigs() = {{"SCM_SLOT", slotTypeConfig}};
  EXPECT_TRUE(ConfigValidator().isValid(config));

  // Test 3: slotTypeConfig without pmUnitName
  slotTypeConfig.pmUnitName().reset();
  config.slotTypeConfigs() = {{"SCM_SLOT", slotTypeConfig}};
  EXPECT_TRUE(ConfigValidator().isValid(config));

  // Test 4: versionedPmUnitConfigs references non-existent PMUnit name
  auto versionedPmUnitConfig = VersionedPmUnitConfig();
  versionedPmUnitConfig.pmUnitConfig()->pluggedInSlotType() = "SCM_SLOT";
  config.versionedPmUnitConfigs() = {
      {"NON_EXISTENT_PMUNIT", {versionedPmUnitConfig}}};
  EXPECT_FALSE(ConfigValidator().isValid(config));

  // Test 5: versionedPmUnitConfigs references existing PMUnit name
  config.versionedPmUnitConfigs() = {{"SCM", {versionedPmUnitConfig}}};
  EXPECT_TRUE(ConfigValidator().isValid(config));
}

TEST(ConfigValidatorTest, PmUnitNameAllowedListValidation) {
  auto config = PlatformConfig();
  config.platformName() = "MERU400BIU";
  config.rootSlotType() = "PIM_SLOT";
  config.bspKmodsRpmName() = "sample_bsp_kmods";
  config.bspKmodsRpmVersion() = "1.0.0-4";

  // Create a basic SlotTypeConfig without pmUnitName to avoid referential
  // integrity issues
  auto slotTypeConfig = SlotTypeConfig();
  slotTypeConfig.idpromConfig() = IdpromConfig();
  slotTypeConfig.idpromConfig()->address() = "0x14";
  config.slotTypeConfigs() = {{"PIM_SLOT", slotTypeConfig}};

  auto pmUnitConfig = PmUnitConfig();
  pmUnitConfig.pluggedInSlotType() = "PIM_SLOT";

  // Test 1: Valid PMUnit name from allowed list (should pass)
  config.pmUnitConfigs() = {{"PIM_8DD", pmUnitConfig}};
  EXPECT_TRUE(ConfigValidator().isValid(config));

  // Test 2: Another valid PMUnit name from allowed list (should pass)
  config.pmUnitConfigs() = {{"PIM_16Q", pmUnitConfig}};
  EXPECT_TRUE(ConfigValidator().isValid(config));

  // Test 3: Invalid PMUnit name not in allowed list (should fail)
  config.pmUnitConfigs() = {{"INVALID_PMUNIT_NAME", pmUnitConfig}};
  EXPECT_FALSE(ConfigValidator().isValid(config));

  // Test 4: Another invalid PMUnit name (should fail)
  config.pmUnitConfigs() = {{"RANDOM_NAME", pmUnitConfig}};
  EXPECT_FALSE(ConfigValidator().isValid(config));

  // Test 5: Multiple PMUnits - all valid (should pass)
  config.pmUnitConfigs() = {
      {"FAN", pmUnitConfig},
      {"PIM_16Q", pmUnitConfig},
      {"PIM_8DD", pmUnitConfig}};
  EXPECT_TRUE(ConfigValidator().isValid(config));

  // Test 6: Multiple PMUnits - one invalid (should fail)
  config.pmUnitConfigs() = {
      {"PIM_8DD", pmUnitConfig},
      {"INVALID_NAME", pmUnitConfig},
      {"PIM_8DD", pmUnitConfig}};
  EXPECT_FALSE(ConfigValidator().isValid(config));
}

TEST(ConfigValidatorTest, SlotTypeConfig) {
  auto slotTypeConfig = getValidSlotTypeConfig();
  EXPECT_TRUE(ConfigValidator().isValidSlotTypeConfig(slotTypeConfig));
  slotTypeConfig.pmUnitName().reset();
  EXPECT_TRUE(ConfigValidator().isValidSlotTypeConfig(slotTypeConfig));
  slotTypeConfig = getValidSlotTypeConfig();
  slotTypeConfig.idpromConfig().reset();
  EXPECT_TRUE(ConfigValidator().isValidSlotTypeConfig(slotTypeConfig));
  slotTypeConfig.pmUnitName().reset();
  slotTypeConfig.idpromConfig().reset();
  EXPECT_FALSE(ConfigValidator().isValidSlotTypeConfig(slotTypeConfig));
  slotTypeConfig = getValidSlotTypeConfig();
  slotTypeConfig.idpromConfig()->address() = "0xK4";
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

TEST(ConfigValidatorTest, OutgoingSlotConfig) {
  std::map<std::string, SlotTypeConfig> slotTypeConfigs = {
      {"MCB_SLOT", SlotTypeConfig()}};
  PmUnitConfig pmUnitConfig;
  pmUnitConfig.pluggedInSlotType() = "MCB_SLOT";
  SlotConfig smbSlotConfig;
  smbSlotConfig.slotType() = "SMB_SLOT";
  // Valid OutgoingSlotConfig
  pmUnitConfig.outgoingSlotConfigs() = {{"SMB_SLOT@0", smbSlotConfig}};
  EXPECT_TRUE(
      ConfigValidator().isValidPmUnitConfig(slotTypeConfigs, pmUnitConfig));
  // Invalid SlotName format
  pmUnitConfig.outgoingSlotConfigs() = {{"SMB_SLOT", smbSlotConfig}};
  EXPECT_FALSE(
      ConfigValidator().isValidPmUnitConfig(slotTypeConfigs, pmUnitConfig));
  // Invalid unmatching SlotType; expect SMB_SLOT but has SCM_SLOT.
  pmUnitConfig.outgoingSlotConfigs() = {{"SCM_SLOT@0", smbSlotConfig}};
  EXPECT_FALSE(
      ConfigValidator().isValidPmUnitConfig(slotTypeConfigs, pmUnitConfig));
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

TEST(ConfigValidatorTest, PciDeviceConfigWithLedCtrlBlockConfigs) {
  auto pciDevConfig = getValidPciDeviceConfig();

  // Test case: Invalid LedCtrlBlockConfig in PciDeviceConfig
  LedCtrlBlockConfig invalidLedConfig;
  invalidLedConfig.pmUnitScopedNamePrefix() = ""; // This will make it invalid
  invalidLedConfig.deviceName() = "port_led";
  invalidLedConfig.csrOffsetCalc() = "0x1000";
  invalidLedConfig.numPorts() = 1;
  invalidLedConfig.ledPerPort() = 1;
  invalidLedConfig.startPort() = 1;
  pciDevConfig.ledCtrlBlockConfigs() = {invalidLedConfig};
  EXPECT_FALSE(ConfigValidator().isValidPciDeviceConfig(pciDevConfig));

  // Test case: Invalid port ranges in LedCtrlBlockConfigs
  LedCtrlBlockConfig config1, config2;
  config1.pmUnitScopedNamePrefix() = "MCB_LED";
  config1.deviceName() = "port_led";
  config1.csrOffsetCalc() = "0x1000";
  config1.numPorts() = 8;
  config1.ledPerPort() = 2;
  config1.startPort() = 1;

  config2.pmUnitScopedNamePrefix() = "MCB_LED";
  config2.deviceName() = "port_led";
  config2.csrOffsetCalc() = "0x1000";
  config2.numPorts() = 8;
  config2.ledPerPort() = 2;
  config2.startPort() = 5; // This creates an overlap with config1 (1-8)

  pciDevConfig.ledCtrlBlockConfigs() = {config1, config2};
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
  EXPECT_FALSE(
      ConfigValidator().isValidSpiDeviceConfigs(
          {spiDevConfigCopy, spiDevConfig}));
  // Empty spiDeviceConfigs
  EXPECT_FALSE(ConfigValidator().isValidSpiDeviceConfigs({}));
}

TEST(ConfigValidatorTest, I2cDeviceConfig) {
  auto i2cConfig = I2cDeviceConfig{};
  EXPECT_FALSE(ConfigValidator().isValidI2cDeviceConfig(i2cConfig));
  i2cConfig.address() = "029";
  EXPECT_FALSE(ConfigValidator().isValidI2cDeviceConfig(i2cConfig));
  i2cConfig.address() = "29";
  EXPECT_FALSE(ConfigValidator().isValidI2cDeviceConfig(i2cConfig));
  i2cConfig.address() = "0x";
  EXPECT_FALSE(ConfigValidator().isValidI2cDeviceConfig(i2cConfig));
  i2cConfig.address() = "0x2F";
  EXPECT_FALSE(ConfigValidator().isValidI2cDeviceConfig(i2cConfig));
  i2cConfig.address() = "0x2f";
  EXPECT_FALSE(ConfigValidator().isValidI2cDeviceConfig(i2cConfig));
  i2cConfig.pmUnitScopedName() = "PM_UNIT";
  EXPECT_TRUE(ConfigValidator().isValidI2cDeviceConfig(i2cConfig));
  i2cConfig.address() = "0x20";
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
  PlatformConfig config;
  config.rootSlotType() = "MCB_SLOT";
  config.rootPmUnitName() = "MCB";
  PmUnitConfig mcb, jumper, scm;
  // Define PmUnit's SlotType
  mcb.pluggedInSlotType() = "MCB_SLOT";
  jumper.pluggedInSlotType() = "JUMPER_SLOT";
  scm.pluggedInSlotType() = "SCM_SLOT";
  // Define SlotConfig's SlotType
  SlotConfig jumperSlotConfig, scmSlotConfig;
  jumperSlotConfig.slotType() = "JUMPER_SLOT";
  scmSlotConfig.slotType() = "SCM_SLOT";
  // Define outgoingSlotConfigs
  mcb.outgoingSlotConfigs() = {
      {"JUMPER_SLOT@0", jumperSlotConfig}, {"JUMPER_SLOT@1", jumperSlotConfig}};
  jumper.outgoingSlotConfigs() = {{"SCM_SLOT@0", scmSlotConfig}};
  // Define devices
  I2cDeviceConfig mcbMux;
  mcbMux.pmUnitScopedName() = "MCB_MUX_A";
  mcb.i2cDeviceConfigs() = {mcbMux};
  EmbeddedSensorConfig jumperNvme;
  jumperNvme.pmUnitScopedName() = "NVME";
  jumper.embeddedSensorConfigs() = {jumperNvme};
  PciDeviceConfig scmPci;
  FpgaIpBlockConfig gpioChipFpgaIpBlock;
  gpioChipFpgaIpBlock.pmUnitScopedName() = "SCM_FPGA_GPIO_1";
  scmPci.gpioChipConfigs() = {gpioChipFpgaIpBlock};
  LedCtrlConfig scmLedCtrl;
  scmLedCtrl.fpgaIpBlockConfig() = FpgaIpBlockConfig();
  scmLedCtrl.fpgaIpBlockConfig()->pmUnitScopedName() = "SCM_SYS_LED";
  scmPci.ledCtrlConfigs() = {scmLedCtrl};
  scm.pciDeviceConfigs() = {scmPci};
  config.pmUnitConfigs() = {{"MCB", mcb}, {"JUMPER", jumper}, {"SCM", scm}};
  // Case-1 Syntatically incorrect
  EXPECT_FALSE(ConfigValidator().isValidDevicePath(config, "/SCM_SLOT@1/[a"));
  EXPECT_FALSE(
      ConfigValidator().isValidDevicePath(config, "/SMB_SLOT@1/SCM_SLOT/[a]"));
  EXPECT_FALSE(ConfigValidator().isValidDevicePath(config, "/[]"));
  EXPECT_FALSE(
      ConfigValidator().isValidDevicePath(config, "/COME@SCM_SLOT@0/[idprom]"));
  EXPECT_FALSE(
      ConfigValidator().isValidDevicePath(
          config, "/COME_SLOT@SCM_SLOT@0/[SCM_MUX_5]"));
  // Case-2 Topologically incorrect SlotPath
  EXPECT_FALSE(
      ConfigValidator().isValidDevicePath(config, "/SMB_SLOT@0/[sensor1]"));
  EXPECT_FALSE(
      ConfigValidator().isValidDevicePath(config, "/JUMPER_SLOT@2/[fpga]"));
  EXPECT_FALSE(
      ConfigValidator().isValidDevicePath(
          config, "/JUMPER_SLOT@0/SCM_SLOT@1/[IDPROM]"));
  // Case-4 Topologically correct SlotPath, incorrect DeviceName
  EXPECT_FALSE(ConfigValidator().isValidDevicePath(config, "/[sensor1]"));
  EXPECT_FALSE(
      ConfigValidator().isValidDevicePath(config, "/JUMPER_SLOT@0/[fpga]"));
  EXPECT_FALSE(
      ConfigValidator().isValidDevicePath(
          config, "/JUMPER_SLOT@0/SCM_SLOT@0/[IDPROM]"));
  // Case-5 Corrects
  EXPECT_TRUE(ConfigValidator().isValidDevicePath(config, "/[MCB_MUX_A]"));
  EXPECT_TRUE(
      ConfigValidator().isValidDevicePath(config, "/JUMPER_SLOT@0/[NVME]"));
  EXPECT_TRUE(
      ConfigValidator().isValidDevicePath(config, "/JUMPER_SLOT@1/[NVME]"));
  EXPECT_TRUE(
      ConfigValidator().isValidDevicePath(
          config, "/JUMPER_SLOT@0/SCM_SLOT@0/[SCM_SYS_LED]"));
  EXPECT_TRUE(
      ConfigValidator().isValidDevicePath(
          config, "/JUMPER_SLOT@0/SCM_SLOT@0/[SCM_FPGA_GPIO_1]"));
}

TEST(ConfigValidatorTest, BspRpm) {
  EXPECT_FALSE(ConfigValidator().isValidBspKmodsRpmVersion(""));
  EXPECT_FALSE(ConfigValidator().isValidBspKmodsRpmVersion("5.4.6"));
  EXPECT_TRUE(ConfigValidator().isValidBspKmodsRpmVersion("5.4.6-1"));
  EXPECT_TRUE(ConfigValidator().isValidBspKmodsRpmVersion("11.44.63-14"));
  EXPECT_FALSE(ConfigValidator().isValidBspKmodsRpmName(""));
  EXPECT_FALSE(ConfigValidator().isValidBspKmodsRpmName("fboss_bsp_kmod"));
  EXPECT_FALSE(ConfigValidator().isValidBspKmodsRpmName("invalid"));
  EXPECT_TRUE(ConfigValidator().isValidBspKmodsRpmName("fboss_bsp_kmods"));
}

TEST(ConfigValidatorTest, PmUnitName) {
  PlatformConfig config;
  config.rootSlotType() = "MCB_SLOT";
  // Invalid missing PmUnitConfig definition
  EXPECT_FALSE(
      ConfigValidator().isValidPmUnitName(config, "/SCM_SLOT@0", "SCM"));
  // Define PmUnitConfig for SCM
  PmUnitConfig scmConfig;
  scmConfig.pluggedInSlotType() = "SCM_SLOT";
  config.pmUnitConfigs() = {{"SCM", scmConfig}};
  // Invalid unmatching SlotType MCB_SLOT vs SCM_SLOT
  EXPECT_FALSE(ConfigValidator().isValidPmUnitName(config, "/", "SCM"));
  // Valid PmUnitName
  EXPECT_TRUE(
      ConfigValidator().isValidPmUnitName(config, "/SCM_SLOT@0", "SCM"));
}

TEST(ConfigValidatorTest, XcvrSymlinks) {
  ConfigValidator validator;
  int16_t numXcvrs = 2;
  std::vector<std::string> symlinks{};

  // Test case: Valid symlinks
  symlinks = {
      "/run/devmap/xcvrs/xcvr_ctrl_1",
      "/run/devmap/xcvrs/xcvr_ctrl_2",
      "/run/devmap/xcvrs/xcvr_io_1",
      "/run/devmap/xcvrs/xcvr_io_2"};
  EXPECT_TRUE(validator.isValidXcvrSymlinks(numXcvrs, symlinks));

  // Test case: Missing control symlink
  symlinks = {
      "/run/devmap/xcvrs/xcvr_ctrl_1",
      "/run/devmap/xcvrs/xcvr_io_1",
      "/run/devmap/xcvrs/xcvr_io_2"};
  EXPECT_FALSE(validator.isValidXcvrSymlinks(numXcvrs, symlinks));

  // Test case: Missing IO symlink
  symlinks = {
      "/run/devmap/xcvrs/xcvr_ctrl_1",
      "/run/devmap/xcvrs/xcvr_ctrl_2",
      "/run/devmap/xcvrs/xcvr_io_1"};
  EXPECT_FALSE(validator.isValidXcvrSymlinks(numXcvrs, symlinks));

  // Test case: Extra symlink
  symlinks = {
      "/run/devmap/xcvrs/xcvr_ctrl_1",
      "/run/devmap/xcvrs/xcvr_ctrl_2",
      "/run/devmap/xcvrs/xcvr_io_1",
      "/run/devmap/xcvrs/xcvr_io_2",
      "/run/devmap/xcvrs/xcvr_ctrl_3"};
  EXPECT_FALSE(validator.isValidXcvrSymlinks(numXcvrs, symlinks));

  // Test case: Unexpected format in symlinks
  symlinks = {
      "/run/devmap/xcvrs/xcvr_control_1",
      "/run/devmap/xcvrs/xcvr_ctrl_2",
      "/run/devmap/xcvrs/xcvr_io_1",
      "/run/devmap/xcvrs/xcvr_io_2"};
  EXPECT_FALSE(validator.isValidXcvrSymlinks(numXcvrs, symlinks));
  symlinks = {
      "/run/devmap/xcvrs/xcvr_ctrl_1",
      "/run/devmap/xcvrs/xcvr_ctrl_2",
      "/run/devmap/xcvrs/xcvr_iod_1",
      "/run/devmap/xcvrs/xcvr_io_2"};
  EXPECT_FALSE(validator.isValidXcvrSymlinks(numXcvrs, symlinks));
  symlinks = {
      "/run/devmap/xcvrs/xcvr_ctrl_1",
      "/run/devmap/xcvrs/xcvr_ctrl_2",
      "/run/devmap/xcvrs/xcvr_io_11",
      "/run/devmap/xcvrs/xcvr_io_2"};
  EXPECT_FALSE(validator.isValidXcvrSymlinks(numXcvrs, symlinks));
}

TEST(ConfigValidatorTest, LedCtrlBlockConfig) {
  ConfigValidator validator;
  LedCtrlBlockConfig config;

  // Test case: Valid config
  config.pmUnitScopedNamePrefix() = "MCB_LED";
  config.deviceName() = "port_led";
  config.csrOffsetCalc() =
      "0x1000 + ({portNum} - {startPort})*0x100 + ({ledNum}-1)*0x10";
  config.numPorts() = 32;
  config.ledPerPort() = 2;
  config.startPort() = 1;
  validator.numXcvrs_ = 32;
  EXPECT_TRUE(validator.isValidLedCtrlBlockConfig(config));

  // Test case: Empty pmUnitScopedNamePrefix
  config.pmUnitScopedNamePrefix() = "";
  EXPECT_FALSE(validator.isValidLedCtrlBlockConfig(config));
  config.pmUnitScopedNamePrefix() = "MCB_LED";

  // Test case: pmUnitScopedNamePrefix ends with _
  config.pmUnitScopedNamePrefix() = "MCB_LED_";
  EXPECT_FALSE(validator.isValidLedCtrlBlockConfig(config));
  config.pmUnitScopedNamePrefix() = "MCB_LED";

  // Test case: Empty deviceName
  config.deviceName() = "";
  EXPECT_FALSE(validator.isValidLedCtrlBlockConfig(config));
  config.deviceName() = "port_led";

  // Test case: Empty csrOffsetCalc
  config.csrOffsetCalc() = "";
  EXPECT_FALSE(validator.isValidLedCtrlBlockConfig(config));
  config.csrOffsetCalc() =
      "0x1000 + ({portNum} - {startPort})*0x100 + ({ledNum}-1)*0x10";

  // Test case: Zero numPorts
  config.numPorts() = 0;
  EXPECT_FALSE(validator.isValidLedCtrlBlockConfig(config));
  config.numPorts() = 32;

  // Test case: Negative numPorts
  config.numPorts() = -1;
  EXPECT_FALSE(validator.isValidLedCtrlBlockConfig(config));
  config.numPorts() = 32;

  // Test case: Zero ledPerPort
  config.ledPerPort() = 0;
  EXPECT_FALSE(validator.isValidLedCtrlBlockConfig(config));
  config.ledPerPort() = 2;

  // Test case: Negative ledPerPort
  config.ledPerPort() = -1;
  EXPECT_FALSE(validator.isValidLedCtrlBlockConfig(config));
  config.ledPerPort() = 2;

  // Test case: ledPerPort > 4
  config.ledPerPort() = 5;
  EXPECT_FALSE(validator.isValidLedCtrlBlockConfig(config));
  config.ledPerPort() = 2;

  // Test case: Negative startPort
  config.startPort() = -1;
  EXPECT_FALSE(validator.isValidLedCtrlBlockConfig(config));
  config.startPort() = 0;

  // Test case: Zero startPort
  config.startPort() = 0;
  EXPECT_FALSE(validator.isValidLedCtrlBlockConfig(config));
  config.startPort() = 1;

  // Test case: Large valid values
  config.numPorts() = 128;
  config.ledPerPort() = 2;
  config.startPort() = 1;
  validator.numXcvrs_ = 128;
  EXPECT_TRUE(validator.isValidLedCtrlBlockConfig(config));

  // Test case: More ports than numXcvrs
  config.numPorts() = 128;
  config.ledPerPort() = 4;
  config.startPort() = 10;
  validator.numXcvrs_ = 64;
  EXPECT_FALSE(validator.isValidLedCtrlBlockConfig(config));

  // Test case: startNum greater than numXcvrs
  config.numPorts() = 32;
  config.ledPerPort() = 2;
  config.startPort() = 33;
  validator.numXcvrs_ = 32;
  EXPECT_FALSE(validator.isValidLedCtrlBlockConfig(config));

  // Test case: startNum + numPorts - 1 greater than numXcvrs
  config.numPorts() = 32;
  config.ledPerPort() = 2;
  config.startPort() = 1;
  validator.numXcvrs_ = 31;
  EXPECT_FALSE(validator.isValidLedCtrlBlockConfig(config));

  // Test case: startPort + numPorts - 1 > numXcvrs (different scenario)
  config.startPort() = 2; // 2 + 32 - 1 = 33 > 32
  config.numPorts() = 32;
  validator.numXcvrs_ = 32;
  EXPECT_FALSE(validator.isValidLedCtrlBlockConfig(config));
  config.startPort() = 1;
}

TEST(ConfigValidatorTest, LedCtrlBlockPortRanges) {
  ConfigValidator validator;
  std::vector<LedCtrlBlockConfig> configs;

  // Helper to create a LedCtrlBlockConfig with specific port range
  auto createLedCtrlBlockConfig = [](int startPort, int numPorts) {
    LedCtrlBlockConfig config;
    config.pmUnitScopedNamePrefix() = "MCB_LED";
    config.deviceName() = "port_led";
    config.csrOffsetCalc() =
        "0x1000 + ({portNum} - {startPort})*0x100 + ({ledNum}-1)*0x10";
    config.startPort() = startPort;
    config.numPorts() = numPorts;
    config.ledPerPort() = 2;
    return config;
  };

  // Test case: Empty config list (should be valid)
  EXPECT_TRUE(validator.isValidPortRanges({}));

  // Test case: Single config (should be valid)
  configs = {createLedCtrlBlockConfig(1, 8)};
  EXPECT_TRUE(validator.isValidPortRanges(configs));

  // Test case: Valid non-overlapping sequential ranges
  configs = {
      createLedCtrlBlockConfig(1, 8), // ports 1-8
      createLedCtrlBlockConfig(9, 16), // ports 9-24
      createLedCtrlBlockConfig(25, 8) // ports 25-32
  };
  EXPECT_TRUE(validator.isValidPortRanges(configs));

  // Test case: Non-vaild non-overlapping sequential ranges (unsorted input)
  configs = {
      createLedCtrlBlockConfig(25, 8), // ports 25-32
      createLedCtrlBlockConfig(1, 8), // ports 1-8
      createLedCtrlBlockConfig(9, 16) // ports 9-24
  };
  EXPECT_FALSE(validator.isValidPortRanges(configs));

  // Test case: Overlapping ranges - same start port
  configs = {
      createLedCtrlBlockConfig(1, 8), // ports 1-8
      createLedCtrlBlockConfig(1, 4) // ports 1-4 (overlaps)
  };
  EXPECT_FALSE(validator.isValidPortRanges(configs));

  // Test case: Overlapping ranges - partial overlap
  configs = {
      createLedCtrlBlockConfig(1, 8), // ports 1-8
      createLedCtrlBlockConfig(5, 8) // ports 5-12 (overlaps 5-8)
  };
  EXPECT_FALSE(validator.isValidPortRanges(configs));

  // Test case: Overlapping ranges - second range completely inside first
  configs = {
      createLedCtrlBlockConfig(1, 16), // ports 1-16
      createLedCtrlBlockConfig(5, 4) // ports 5-8 (inside first range)
  };
  EXPECT_FALSE(validator.isValidPortRanges(configs));

  // Test case: Overlapping ranges - first range completely inside second
  configs = {
      createLedCtrlBlockConfig(5, 4), // ports 5-8
      createLedCtrlBlockConfig(1, 16) // ports 1-16 (contains first range)
  };
  EXPECT_FALSE(validator.isValidPortRanges(configs));

  // Test case: Adjacent ranges touching exactly (should be valid)
  configs = {
      createLedCtrlBlockConfig(1, 8), // ports 1-8
      createLedCtrlBlockConfig(9, 8) // ports 9-16 (touches exactly)
  };
  EXPECT_TRUE(validator.isValidPortRanges(configs));

  // Test case: Overlapping by one port
  configs = {
      createLedCtrlBlockConfig(1, 8), // ports 1-8
      createLedCtrlBlockConfig(8, 8) // ports 8-15 (overlaps at port 8)
  };
  EXPECT_FALSE(validator.isValidPortRanges(configs));

  // Test case: Multiple overlapping ranges
  configs = {
      createLedCtrlBlockConfig(1, 8), // ports 1-8
      createLedCtrlBlockConfig(5, 8), // ports 5-12 (overlaps with first)
      createLedCtrlBlockConfig(10, 8) // ports 10-17 (overlaps with second)
  };
  EXPECT_FALSE(validator.isValidPortRanges(configs));

  // Test case: Complex valid scenario with multiple ranges
  configs = {
      createLedCtrlBlockConfig(1, 16), // ports 1-16
      createLedCtrlBlockConfig(17, 16), // ports 17-32
      createLedCtrlBlockConfig(33, 16), // ports 33-48
      createLedCtrlBlockConfig(49, 16) // ports 49-64
  };
  EXPECT_TRUE(validator.isValidPortRanges(configs));

  // Test case: Complex invalid scenario with one overlap in the middle
  configs = {
      createLedCtrlBlockConfig(1, 16), // ports 1-16
      createLedCtrlBlockConfig(17, 16), // ports 17-32
      createLedCtrlBlockConfig(
          30, 16), // ports 30-45 (overlaps with previous: 30-32)
      createLedCtrlBlockConfig(46, 16) // ports 46-61
  };
  EXPECT_FALSE(validator.isValidPortRanges(configs));

  // Test case: Single port ranges (valid)
  configs = {
      createLedCtrlBlockConfig(1, 1), // port 1
      createLedCtrlBlockConfig(2, 1), // port 2
      createLedCtrlBlockConfig(3, 1) // port 3
  };
  EXPECT_TRUE(validator.isValidPortRanges(configs));

  // Test case: Single port ranges with overlap
  configs = {
      createLedCtrlBlockConfig(1, 1), // port 1
      createLedCtrlBlockConfig(1, 1) // port 1 (duplicate)
  };
  EXPECT_FALSE(validator.isValidPortRanges(configs));
}

TEST(ConfigValidatorTest, CsrOffsetCalc) {
  ConfigValidator validator;

  // Test case: Valid  LED control expressions
  EXPECT_TRUE(validator.isValidCsrOffsetCalc(
      "0x40410 + ({portNum} - {startPort})*0x8 + ({ledNum} - 1)*0x4", 2, 1, 1));
  EXPECT_TRUE(validator.isValidCsrOffsetCalc(
      "0x48410 + ({portNum} - {startPort})*0x8 + ({ledNum} - 1)*0x4",
      45,
      2,
      33));
  EXPECT_TRUE(validator.isValidCsrOffsetCalc(
      "0x65c0 + ({portNum} - {startPort})*0x10 + ({ledNum} - 1)*0x10",
      39,
      3,
      39));

  // Test case: Valid expressions with zero values
  EXPECT_TRUE(validator.isValidCsrOffsetCalc(
      "{portNum} + {ledNum} + {startPort}", 0, 0, 0));

  // Test case: Valid expression
  EXPECT_TRUE(validator.isValidCsrOffsetCalc("12345", 1, 1, 1));

  // Test case: Invalid expressions - syntax errors
  EXPECT_FALSE(validator.isValidCsrOffsetCalc("invalid_expression", 1, 1, 1));
  EXPECT_FALSE(validator.isValidCsrOffsetCalc("0x1000 +", 1, 1, 1));
  EXPECT_FALSE(validator.isValidCsrOffsetCalc("0x1000 + * 0x100", 1, 1, 1));
  EXPECT_FALSE(validator.isValidCsrOffsetCalc("", 1, 1, 1));
  EXPECT_FALSE(validator.isValidCsrOffsetCalc("0x + 1000", 1, 1, 1));

  // Test case: Invalid expressions - unbalanced parentheses
  EXPECT_FALSE(
      validator.isValidCsrOffsetCalc("({portNum} + {ledNum}", 1, 1, 1));
  EXPECT_FALSE(
      validator.isValidCsrOffsetCalc("{portNum} + {ledNum})", 1, 1, 1));
  EXPECT_FALSE(validator.isValidCsrOffsetCalc("((({portNum}))", 1, 1, 1));

  // Test case: Invalid expressions - unknown variables
  EXPECT_FALSE(validator.isValidCsrOffsetCalc("{unknownVar}", 1, 1, 1));
  EXPECT_FALSE(
      validator.isValidCsrOffsetCalc("0x1000 + {portNumber}", 1, 1, 1));
  EXPECT_FALSE(validator.isValidCsrOffsetCalc("0x1000 + {ledNumber}", 1, 1, 1));
  EXPECT_FALSE(validator.isValidCsrOffsetCalc("0x1000 + {ledNumber}", 1, 1, 1));
  EXPECT_FALSE(validator.isValidCsrOffsetCalc("{ledNum} * 5 abcd 10", 1, 1, 1));
}
