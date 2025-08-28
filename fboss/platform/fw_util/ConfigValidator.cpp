// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/platform/fw_util/ConfigValidator.h"

#include <fmt/format.h>
#include <folly/logging/xlog.h>

namespace facebook::fboss::platform::fw_util {

bool ConfigValidator::isValid(const fw_util_config::FwUtilConfig& config) {
  XLOG(INFO) << "Validating fw_util config";
  if (config.fwConfigs()->empty()) {
    XLOG(INFO) << "FwUtilConfig is empty. Please check the config file";
  }
  // Validate each firmware configuration
  for (const auto& [deviceName, _] : *config.fwConfigs()) {
    if (!isValidFwConfig(deviceName)) {
      return false;
    }
  }

  XLOG(INFO) << "FwUtilConfig validation passed";
  return true;
}

bool ConfigValidator::isValidFwConfig(const std::string& deviceName) {
  XLOG(INFO) << fmt::format("Validating FwConfig for device: {}", deviceName);

  if (deviceName.empty()) {
    XLOG(ERR) << "Device name cannot be empty";
    return false;
  }
  return true;
}

} // namespace facebook::fboss::platform::fw_util
