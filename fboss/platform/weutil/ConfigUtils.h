// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.
#pragma once

#include <string>
#include <unordered_map>

#include "fboss/platform/platform_manager/gen-cpp2/platform_manager_config_types.h"

namespace facebook::fboss::platform::weutil {

struct FruEeprom {
  std::string path;
  int offset = 0;
};

class ConfigUtils {
 public:
  explicit ConfigUtils(
      const std::optional<std::string>& platformName = std::nullopt);

  std::unordered_map<std::string, FruEeprom> getFruEepromList();
  std::string getChassisEepromName();
  FruEeprom getFruEeprom(const std::string& eepromName);

 private:
  platform_manager::PlatformConfig config_;
};

} // namespace facebook::fboss::platform::weutil
