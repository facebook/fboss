// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <folly/logging/xlog.h>

#include "fboss/platform/platform_manager/PlatformValidator.h"

namespace facebook::fboss::platform::platform_manager {

bool PlatformValidator::isValid(const PlatformConfig& config) {
  XLOG(INFO) << "Validating the config";
  if (config.platformName()->empty()) {
    XLOG(ERR) << "Platform name cannot be empty";
    return false;
  }
  // TODO: Validate platformName matches what is set in dmidecode on BIOS
  if (config.slotTypeConfigs()->find("CHASSIS_SLOT") ==
      config.slotTypeConfigs()->end()) {
    XLOG(ERR) << "CHASSIS_SLOT SlotTypeConfig is not found";
    return false;
  }

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

  return true;
}

} // namespace facebook::fboss::platform::platform_manager
