// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/platform/data_corral_service/ConfigValidator.h"

#include <folly/logging/xlog.h>

namespace facebook::fboss::platform::data_corral_service {

bool ConfigValidator::isValid(const LedManagerConfig& config) {
  XLOG(INFO) << "Validating the config";

  XLOG(INFO) << "Validating the system LED config";
  if (!isValidLedConfig(*config.systemLedConfig())) {
    return false;
  }

  if (!isValidFruTypeLedConfigs(*config.fruTypeLedConfigs())) {
    return false;
  }

  if (!isValidFruConfigs(*config.fruConfigs(), *config.fruTypeLedConfigs())) {
    return false;
  }

  return true;
}

bool ConfigValidator::isValidLedConfig(const LedConfig& ledConfig) {
  if (ledConfig.presentLedSysfsPath()->empty()) {
    XLOG(ERR) << "Present LED sysfs path cannot be empty";
    return false;
  }
  if (ledConfig.absentLedSysfsPath()->empty()) {
    XLOG(ERR) << "Absent LED sysfs path cannot be empty";
    return false;
  }
  return true;
}

bool ConfigValidator::isValidFruConfig(
    const FruConfig& fruConfig,
    const std::map<std::string, LedConfig>& fruTypeLedConfigs) {
  if (fruConfig.fruType()->empty() ||
      fruTypeLedConfigs.find(*fruConfig.fruType()) == fruTypeLedConfigs.end()) {
    XLOG(ERR)
        << "FruType cannot be empty and must be present in fruTypeLedConfigs";
    return false;
  }
  if (fruConfig.fruName()->empty()) {
    XLOG(ERR) << "FruName cannot be empty";
    return false;
  }
  if (fruConfig.presenceSysfsPath()->empty()) {
    XLOG(ERR) << "PresenceSysfsPath cannot be empty";
    return false;
  }
  return true;
}

bool ConfigValidator::isValidFruTypeLedConfigs(
    const std::map<std::string, LedConfig>& fruTypeLedConfigs) {
  for (const auto& [fruType, ledConfig] : fruTypeLedConfigs) {
    XLOG(INFO) << "Validating the LED config for fruType: " << fruType;
    if (!isValidLedConfig(ledConfig)) {
      return false;
    }
  }
  return true;
}

bool ConfigValidator::isValidFruConfigs(
    const std::vector<FruConfig>& fruConfigs,
    const std::map<std::string, LedConfig>& fruTypeLedConfigs) {
  for (const auto& fruConfig : fruConfigs) {
    XLOG(INFO) << "Validating the FRU config for fru: " << *fruConfig.fruName();
    if (!isValidFruConfig(fruConfig, fruTypeLedConfigs)) {
      return false;
    }
  }
  return true;
}

} // namespace facebook::fboss::platform::data_corral_service
