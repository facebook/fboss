// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/platform/fw_util/if/gen-cpp2/fw_util_config_types.h"

namespace facebook::fboss::platform::fw_util {

class ConfigValidator {
 public:
  bool isValid(const fw_util_config::FwUtilConfig& config);

 private:
  bool isValidFwConfig(
      const std::string& deviceName,
      const fw_util_config::FwConfig& fwConfig);

  bool isValidVersionConfig(
      const std::string& deviceName,
      const fw_util_config::VersionConfig& versionConfig);

  // Utility validation methods
  bool isValidVersionType(const std::string& versionType);
  bool isValidPath(const std::string& path);
};
} // namespace facebook::fboss::platform::fw_util
