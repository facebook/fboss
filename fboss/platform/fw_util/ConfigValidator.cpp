// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/platform/fw_util/ConfigValidator.h"

#include <fmt/format.h>
#include <folly/String.h>
#include <folly/logging/xlog.h>
#include <re2/re2.h>

namespace {
// Valid version types based on analysis of existing configurations
const std::unordered_set<std::string> kValidVersionTypes = {
    "sysfs",
    "full_command",
    "Not Applicable"};

const re2::RE2 kDevmapPathRegex{"^/run/devmap/.+"};
const re2::RE2 kSysfsPathRegex{"^/sys/.+"};

// Helper function to check if component is allowed to use sysfs paths
bool isComponentAllowedSysfsAccess(const std::string& deviceName) {
  std::string lowerName = deviceName;
  folly::toLowerAscii(lowerName);
  return lowerName.find("bios") != std::string::npos ||
      lowerName.find("oob") != std::string::npos;
}
} // namespace

namespace facebook::fboss::platform::fw_util {

bool ConfigValidator::isValid(const fw_util_config::FwUtilConfig& config) {
  XLOG(INFO) << "Validating fw_util config";
  if (config.fwConfigs()->empty()) {
    XLOG(INFO) << "FwUtilConfig is empty. Please check the config file";
  }
  // Validate each firmware configuration
  for (const auto& [deviceName, fwConfig] : *config.fwConfigs()) {
    if (!isValidFwConfig(deviceName, fwConfig)) {
      return false;
    }
  }

  XLOG(INFO) << "FwUtilConfig validation passed";
  return true;
}

bool ConfigValidator::isValidFwConfig(
    const std::string& deviceName,
    const fw_util_config::FwConfig& fwConfig) {
  XLOG(INFO) << fmt::format("Validating FwConfig for device: {}", deviceName);

  if (deviceName.empty()) {
    XLOG(ERR) << "Device name cannot be empty";
    return false;
  }

  // Priority must be positive
  if (*fwConfig.priority() < 0) {
    XLOG(ERR) << fmt::format(
        "Priority must be positive for device {}, got: {}",
        deviceName,
        *fwConfig.priority());
    return false;
  }

  if (!isValidVersionConfig(deviceName, *fwConfig.version())) {
    return false;
  }

  return true;
}

bool ConfigValidator::isValidVersionConfig(
    const std::string& deviceName,
    const fw_util_config::VersionConfig& versionConfig) {
  if (versionConfig.versionType()->empty()) {
    XLOG(ERR) << "Version type cannot be empty";
    return false;
  }

  if (!isValidVersionType(*versionConfig.versionType())) {
    XLOG(ERR) << fmt::format(
        "Invalid version type: {}", *versionConfig.versionType());
    return false;
  }

  // For sysfs version types, path is required
  if (*versionConfig.versionType() == "sysfs") {
    if (!versionConfig.path() || versionConfig.path()->empty()) {
      XLOG(ERR) << "Path is required for sysfs version type";
      return false;
    }
    const std::string& path = *versionConfig.path();

    // Always allow /run/devmap paths for any component
    if (isValidPath(path)) {
      return true;
    }

    // Additionally allow sysfs paths for BIOS and OOB components
    if (isComponentAllowedSysfsAccess(deviceName) &&
        re2::RE2::FullMatch(path, kSysfsPathRegex)) {
      return true;
    }

    return false;
  }

  // For full_command version types, getVersionCmd is required
  if (*versionConfig.versionType() == "full_command") {
    if (!versionConfig.getVersionCmd() ||
        versionConfig.getVersionCmd()->empty()) {
      XLOG(ERR) << "getVersionCmd is required for full_command version type";
      return false;
    }
  }

  return true;
}

bool ConfigValidator::isValidVersionType(const std::string& versionType) {
  return kValidVersionTypes.count(versionType) > 0;
}

bool ConfigValidator::isValidPath(const std::string& path) {
  if (path.empty()) {
    return false;
  }

  // Check for devmap paths (runtime paths)
  if (re2::RE2::FullMatch(path, kDevmapPathRegex)) {
    return true;
  }

  return false;
}
} // namespace facebook::fboss::platform::fw_util
