// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/platform/platform_manager/ConfigValidator.h"
#include "fboss/platform/sensor_service/if/gen-cpp2/sensor_config_types.h"

namespace facebook::fboss::platform::sensor_service {
class ConfigValidator {
 public:
  ConfigValidator(
      const std::shared_ptr<platform_manager::ConfigValidator>&
          pmConfigValidator =
              std::make_shared<platform_manager::ConfigValidator>());
  bool isValid(
      const sensor_config::SensorConfig& sensorConfig,
      const std::optional<platform_manager::PlatformConfig>& platformConfig);
  bool isValidPmUnitSensors(
      const sensor_config::PmUnitSensors& PmUnitSensors,
      std::unordered_set<std::pair<std::string, std::string>>& usedSlotPaths);
  bool isValidPmSensor(
      const sensor_config::PmSensor& pmSensor,
      std::unordered_set<std::string>& usedSensorNames);
  bool isValidSlotPath(const std::string& slotPath);

 private:
  const std::shared_ptr<platform_manager::ConfigValidator> pmConfigValidator_;
};

} // namespace facebook::fboss::platform::sensor_service
