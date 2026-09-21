// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/platform/data_corral_service/if/gen-cpp2/led_manager_config_types.h"

namespace facebook::fboss::platform::data_corral_service {

class ConfigValidator {
 public:
  bool isValid(const LedManagerConfig& ledManagerConfig);
  bool isValidLedConfig(const LedConfig& ledConfig);
  bool isValidFruConfig(
      const FruConfig& fruConfig,
      const std::map<std::string, LedConfig>& fruTypeLedConfigs);
  bool isValidFruTypeLedConfigs(
      const std::map<std::string, LedConfig>& fruTypeLedConfigs);
  bool isValidFruConfigs(
      const std::vector<FruConfig>& fruConfigs,
      const std::map<std::string, LedConfig>& fruTypeLedConfigs);
  bool isValidPresenceConfig(const PresenceDetection& presenceConfig);
};
} // namespace facebook::fboss::platform::data_corral_service
