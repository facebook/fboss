// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <folly/logging/xlog.h>

#include "fboss/platform/platform_manager/PlatformValidator.h"

namespace facebook::fboss::platform::platform_manager {

bool PlatformValidator::isValidSlotTypeConfig(
    const SlotTypeConfig& slotTypeConfig) {
  if (!slotTypeConfig.idpromConfig_ref() && !slotTypeConfig.fruType_ref()) {
    XLOG(ERR) << "SlotTypeConfig must have either EEPROM or FRUType name";
    return false;
  }
  return true;
}

bool PlatformValidator::isValid(const PlatformConfig& config) {
  XLOG(INFO) << "Validating the config";

  // Verify presence of platform name
  if (config.platformName()->empty()) {
    XLOG(ERR) << "Platform name cannot be empty";
    return false;
  }

  // TODO: Validate platformName matches what is set in dmidecode on BIOS

  // Verify presence of CHASSIS_SLOT SlotTypeConfig
  if (config.slotTypeConfigs()->find("CHASSIS_SLOT") ==
      config.slotTypeConfigs()->end()) {
    XLOG(ERR) << "CHASSIS_SLOT SlotTypeConfig is not found";
    return false;
  }

  // Verify presence of CHASSIS FruTypeConfig
  int count(0);
  for (const auto& [fruTypeName, fruTypeConfig] : *config.fruTypeConfigs()) {
    if (*fruTypeConfig.pluggedInSlotType() == "CHASSIS_SLOT") {
      count++;
    }
  }
  if (count != 1) {
    XLOG(ERR) << "Exactly one CHASSIS FruTypeConfig is expected";
    return false;
  }

  for (const auto& [slotName, slotTypeConfig] : *config.slotTypeConfigs()) {
    if (!isValidSlotTypeConfig(slotTypeConfig)) {
      return false;
    }
  }

  return true;
}

} // namespace facebook::fboss::platform::platform_manager
