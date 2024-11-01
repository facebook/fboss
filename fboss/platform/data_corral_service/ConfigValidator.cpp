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
  if (!isValidPresenceConfig(*fruConfig.presenceDetection())) {
    XLOG(ERR) << "PresenceDetection is invalid";
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

bool ConfigValidator::isValidPresenceConfig(const PresenceDetection& config) {
  // Both cannot be present
  if (config.sysfsFileHandle() && config.gpioLineHandle()) {
    XLOG(ERR)
        << "Only one of GpioLineHandle or SysfsFileHandle must be set for PresenceDetection";
    return false;
  }
  if (!config.sysfsFileHandle() && !config.gpioLineHandle()) {
    XLOG(ERR)
        << "GpioLineHandle or SysfsFileHandle must be set for PresenceDetection";
    return false;
  }

  if (config.sysfsFileHandle()) {
    if (config.sysfsFileHandle()->presenceFilePath()->empty()) {
      XLOG(ERR) << "Sysfs path cannot be empty";
      return false;
    }
    if (config.sysfsFileHandle()->desiredValue() < 0) {
      XLOG(ERR)
          << "desiredValue for SysfsFileHandle cannot be < 0. Typically 0 or 1";
      return false;
    }
  }
  if (config.gpioLineHandle()) {
    if (config.gpioLineHandle()->charDevPath()->empty()) {
      XLOG(ERR) << "charDevPath for GpioLineHandle cannot be empty";
      return false;
    }
    if (config.gpioLineHandle()->desiredValue() < 0) {
      XLOG(ERR)
          << "desiredValue for GpioLineHandle cannot be < 0. Typically 0 or 1";
      return false;
    }
    if (config.gpioLineHandle()->lineIndex() < 0) {
      XLOG(ERR) << "lineIndex for GpioLineHandle cannot be < 0.";
      return false;
    }
  }
  return true;
}

} // namespace facebook::fboss::platform::data_corral_service
