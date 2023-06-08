// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <folly/logging/xlog.h>

#include "fboss/platform/platform_manager/PlatformValidator.h"

namespace facebook::fboss::platform::platform_manager {

bool PlatformValidator::isValid(const PlatformConfig& platformConfig) {
  XLOG(INFO) << "Validating the config";
  if (platformConfig.platformName()->empty()) {
    XLOG(ERR) << "Platform name cannot be empty";
    return false;
  }
  // TODO: Validate platformName matches what is set in dmedicode on BIOS
  if (platformConfig.slotTypeConfigs()->find("CHASSIS") ==
      platformConfig.slotTypeConfigs()->end()) {
    XLOG(ERR) << "CHASSIS SlotTypeConfig is not found";
    return false;
  }
  return true;
}

} // namespace facebook::fboss::platform::platform_manager
