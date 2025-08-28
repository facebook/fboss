// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/platform/fw_util/ConfigValidator.h"

#include <folly/logging/xlog.h>

namespace facebook::fboss::platform::fw_util {

bool ConfigValidator::isValid(const fw_util_config::FwUtilConfig& config) {
  XLOG(INFO) << "Validating fw_util config";
  if (config.fwConfigs()->empty()) {
    XLOG(INFO) << "FwUtilConfig is empty. Please check the config file";
  }

  XLOG(INFO) << "FwUtilConfig validation passed";
  return true;
}

} // namespace facebook::fboss::platform::fw_util
