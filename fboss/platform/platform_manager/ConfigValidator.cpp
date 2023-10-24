// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/platform/platform_manager/ConfigValidator.h"

#include <folly/logging/xlog.h>
#include <re2/re2.h>

#include "fboss/platform/platform_manager/I2cExplorer.h"
#include "fboss/platform/platform_manager/PciExplorer.h"
#include "fboss/platform/platform_manager/gen-cpp2/platform_manager_config_constants.h"

namespace {
using constants = facebook::fboss::platform::platform_manager::
    platform_manager_config_constants;

std::unordered_set<std::string> deviceTypes = {
    constants::DEVICE_TYPE_SENSOR(),
    constants::DEVICE_TYPE_EEPROM()};

} // namespace

namespace {
const re2::RE2 kPciDevOffsetRegex{"0x[0-9a-f]+"};
}

namespace facebook::fboss::platform::platform_manager {

bool ConfigValidator::isValidSlotTypeConfig(
    const SlotTypeConfig& slotTypeConfig) {
  if (!slotTypeConfig.idpromConfig_ref() && !slotTypeConfig.pmUnitName()) {
    XLOG(ERR) << "SlotTypeConfig must have either IDPROM or PmUnit name";
    return false;
  }
  if (slotTypeConfig.idpromConfig_ref()) {
    try {
      I2cAddr(*slotTypeConfig.idpromConfig_ref()->address_ref());
    } catch (std::invalid_argument& e) {
      XLOG(ERR) << "IDPROM has invalid address " << e.what();
      return false;
    }
  }
  return true;
}

bool ConfigValidator::isValidFpgaIpBlockConfig(
    const FpgaIpBlockConfig& fpgaIpBlockConfig) {
  if (!fpgaIpBlockConfig.csrOffset()->empty() &&
      !re2::RE2::FullMatch(
          *fpgaIpBlockConfig.csrOffset(), kPciDevOffsetRegex)) {
    XLOG(ERR) << "Invalid CSR Offset : " << *fpgaIpBlockConfig.csrOffset();
    return false;
  }
  if (!fpgaIpBlockConfig.iobufOffset()->empty() &&
      !re2::RE2::FullMatch(
          *fpgaIpBlockConfig.iobufOffset(), kPciDevOffsetRegex)) {
    XLOG(ERR) << "Invalid IOBuf Offset : " << *fpgaIpBlockConfig.iobufOffset();
    return false;
  }
  return true;
}

bool ConfigValidator::isValidPciDeviceConfig(
    const PciDeviceConfig& pciDeviceConfig) {
  if (!re2::RE2::FullMatch(
          *pciDeviceConfig.vendorId(), PciExplorer().kPciIdRegex)) {
    XLOG(ERR) << "Invalid PCI vendor id : " << *pciDeviceConfig.vendorId();
    return false;
  }
  if (!re2::RE2::FullMatch(
          *pciDeviceConfig.deviceId(), PciExplorer().kPciIdRegex)) {
    XLOG(ERR) << "Invalid PCI device id : " << *pciDeviceConfig.deviceId();
    return false;
  }
  if (!pciDeviceConfig.subSystemDeviceId()->empty() &&
      !re2::RE2::FullMatch(
          *pciDeviceConfig.subSystemVendorId(), PciExplorer().kPciIdRegex)) {
    XLOG(ERR) << "Invalid PCI subsystem vendor id : "
              << *pciDeviceConfig.subSystemVendorId();
    return false;
  }
  if (!pciDeviceConfig.subSystemVendorId()->empty() &&
      !re2::RE2::FullMatch(
          *pciDeviceConfig.subSystemDeviceId(), PciExplorer().kPciIdRegex)) {
    XLOG(ERR) << "Invalid PCI subsystem device id : "
              << *pciDeviceConfig.subSystemDeviceId();
    return false;
  }

  std::vector<FpgaIpBlockConfig> fpgaIpBlockConfigs{};
  for (const auto& config : *pciDeviceConfig.i2cAdapterConfigs()) {
    fpgaIpBlockConfigs.push_back(*config.fpgaIpBlockConfig());
  }
  for (const auto& config : *pciDeviceConfig.spiMasterConfigs()) {
    fpgaIpBlockConfigs.push_back(*config.fpgaIpBlockConfig());
  }
  for (const auto& config : *pciDeviceConfig.gpioChipConfigs()) {
    fpgaIpBlockConfigs.push_back(config);
  }
  for (const auto& config : *pciDeviceConfig.watchdogConfigs()) {
    fpgaIpBlockConfigs.push_back(config);
  }
  for (const auto& config : *pciDeviceConfig.fanTachoPwmConfigs()) {
    fpgaIpBlockConfigs.push_back(config);
  }
  for (const auto& config : *pciDeviceConfig.ledCtrlConfigs()) {
    fpgaIpBlockConfigs.push_back(*config.fpgaIpBlockConfig());
  }
  for (const auto& config : *pciDeviceConfig.xcvrCtrlConfigs()) {
    fpgaIpBlockConfigs.push_back(*config.fpgaIpBlockConfig());
  }

  std::set<std::string> uniqueNames{};
  for (const auto& config : fpgaIpBlockConfigs) {
    auto [it, inserted] = uniqueNames.insert(*config.pmUnitScopedName());
    if (!inserted) {
      XLOG(ERR) << "Duplicate pmUnitScopedName: " << *config.pmUnitScopedName();
      return false;
    }
    if (!isValidFpgaIpBlockConfig(config)) {
      return false;
    }
  }

  return true;
}

bool ConfigValidator::isValidI2cDeviceConfig(
    const I2cDeviceConfig& i2cDeviceConfig) {
  try {
    I2cAddr(*i2cDeviceConfig.address_ref());
  } catch (std::invalid_argument& e) {
    XLOG(ERR) << "IDPROM has invalid address " << e.what();
    return false;
  }

  if (!i2cDeviceConfig.deviceType()->empty() &&
      !deviceTypes.count(*i2cDeviceConfig.deviceType())) {
    XLOG(INFO) << "Invalid device type: " << *i2cDeviceConfig.deviceType();
    return false;
  }

  return true;
}

bool ConfigValidator::isValid(const PlatformConfig& config) {
  XLOG(INFO) << "Validating the config";

  // Verify presence of platform name
  if (config.platformName()->empty()) {
    XLOG(ERR) << "Platform name cannot be empty";
    return false;
  }

  // Validate SlotTypeConfigs.
  for (const auto& [slotName, slotTypeConfig] : *config.slotTypeConfigs()) {
    if (!isValidSlotTypeConfig(slotTypeConfig)) {
      return false;
    }
  }

  for (const auto& [name, pmUnitConfig] : *config.pmUnitConfigs()) {
    // Validate PciDeviceConfigs
    for (const auto& pciDeviceConfig : *pmUnitConfig.pciDeviceConfigs_ref()) {
      if (!isValidPciDeviceConfig(pciDeviceConfig)) {
        return false;
      }
    }

    // Validate I2cDeviceConfigs
    for (const auto& i2cDeviceConfig : *pmUnitConfig.i2cDeviceConfigs_ref()) {
      if (!isValidI2cDeviceConfig(i2cDeviceConfig)) {
        return false;
      }
    }
  }

  return true;
}

} // namespace facebook::fboss::platform::platform_manager
