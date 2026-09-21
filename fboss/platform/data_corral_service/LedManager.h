// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/platform/data_corral_service/if/gen-cpp2/led_manager_config_types.h"

namespace facebook::fboss::platform::data_corral_service {

class LedManager {
 public:
  explicit LedManager(
      const LedConfig& systemLedConfig,
      const std::map<std::string, LedConfig>& fruTypeLedConfigs);
  virtual ~LedManager() = default;
  virtual bool programSystemLed(bool presence) const;
  virtual bool programFruLed(const std::string& fruType, bool presence) const;

 private:
  void programLed(
      const std::string& name /* system or fruType*/,
      const std::string& sysfsPath,
      std::string value) const;

  const LedConfig systemLedConfig_;
  const std::map<std::string /*fruType*/, LedConfig> fruTypeLedConfigs_;
};

} // namespace facebook::fboss::platform::data_corral_service
