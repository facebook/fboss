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
  // TODO: Validate platformName matches what is set in dmedicode on BIOS
  if (config.slotTypeConfigs()->find("MAIN_BOARD") ==
      config.slotTypeConfigs()->end()) {
    XLOG(ERR) << "MAIN_BOARD SlotTypeConfig is not found";
    return false;
  }

  int count(0);
  for (const auto& [fruTypeName, fruTypeConfig] : *config.fruTypeConfigs()) {
    if (*fruTypeConfig.pluggedInSlotType() == "MAIN_BOARD") {
      count++;
    }
  }
  if (count != 1) {
    XLOG(ERR) << "Exactly one MAIN_BOARD FruTypeConfig is expected";
    return false;
  }

  return true;
}

} // namespace facebook::fboss::platform::platform_manager
