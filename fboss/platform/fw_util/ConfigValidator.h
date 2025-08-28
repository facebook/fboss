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

  bool isValidFlashromConfig(
      const fw_util_config::FlashromConfig& flashromConfig);

  bool isValidJamConfig(const fw_util_config::JamConfig& jamConfig);

  bool isValidXappConfig(const fw_util_config::XappConfig& xappConfig);

  bool isValidJtagConfig(const fw_util_config::JtagConfig& jtagConfig);

  bool isValidGpiosetConfig(const fw_util_config::GpiosetConfig& gpiosetConfig);

  bool isValidGpiogetConfig(const fw_util_config::GpiogetConfig& gpiogetConfig);

  bool isValidWriteToPortConfig(
      const fw_util_config::WriteToPortConfig& writeToPortConfig);

  bool isValidVersionConfig(
      const std::string& deviceName,
      const fw_util_config::VersionConfig& versionConfig);

  bool isValidPreUpgradeConfig(
      const fw_util_config::PreFirmwareOperationConfig& preUpgradeConfig);

  bool isValidUpgradeConfig(const fw_util_config::UpgradeConfig& upgradeConfig);

  bool isValidPostUpgradeConfig(
      const fw_util_config::PostFirmwareOperationConfig& postUpgradeConfig);

  bool isValidVerifyConfig(
      const fw_util_config::VerifyFirmwareOperationConfig& verifyConfig);

  // Utility validation methods
  bool isValidCommandType(const std::string& commandType);
  bool isValidVersionType(const std::string& versionType);
  bool isValidProgrammerType(const std::string& programmerType);
  bool isValidPath(const std::string& path);
  bool isValidSha1Sum(const std::string& sha1sum);
};
} // namespace facebook::fboss::platform::fw_util
