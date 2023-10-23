// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/platform/platform_manager/ConfigValidator.h"

#include <folly/logging/xlog.h>
#include <re2/re2.h>

#include "fboss/platform/platform_manager/I2cExplorer.h"
#include "fboss/platform/platform_manager/PciExplorer.h"

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
  for (const auto& [_, i2cAdapterConfig] :
       *pciDeviceConfig.i2cAdapterConfigs()) {
    if (!isValidFpgaIpBlockConfig(*i2cAdapterConfig.fpgaIpBlockConfig())) {
      return false;
    }
  }
  for (const auto& [_, spiMasterConfig] : *pciDeviceConfig.spiMasterConfigs()) {
    if (!isValidFpgaIpBlockConfig(*spiMasterConfig.fpgaIpBlockConfig())) {
      return false;
    }
  }
  for (const auto& [_, gpioChipConfig] : *pciDeviceConfig.gpioChipConfigs()) {
    if (!isValidFpgaIpBlockConfig(gpioChipConfig)) {
      return false;
    }
  }
  for (const auto& [_, watchdogConfig] : *pciDeviceConfig.watchdogConfigs()) {
    if (!isValidFpgaIpBlockConfig(watchdogConfig)) {
      return false;
    }
  }
  for (const auto& [_, fanTachoPwmConfig] :
       *pciDeviceConfig.fanTachoPwmConfigs()) {
    if (!isValidFpgaIpBlockConfig(fanTachoPwmConfig)) {
      return false;
    }
  }
  for (const auto& [_, ledCtrlConfig] : *pciDeviceConfig.ledCtrlConfigs()) {
    if (!isValidFpgaIpBlockConfig(*ledCtrlConfig.fpgaIpBlockConfig())) {
      return false;
    }
  }
  for (const auto& [_, xcvrCtrlConfig] : *pciDeviceConfig.xcvrCtrlConfigs()) {
    if (!isValidFpgaIpBlockConfig(*xcvrCtrlConfig.fpgaIpBlockConfig())) {
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
