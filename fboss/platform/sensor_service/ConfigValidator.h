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
  // Local validation
  bool isValidPmUnitSensorsList(
      const std::vector<sensor_config::PmUnitSensors>& pmUnitSensorsList);
  bool isValidPmSensors(const std::vector<sensor_config::PmSensor>& pmSensor);
  bool isValidSlotPath(const std::string& slotPath);
  // PlatformManager validation
  bool isPmValidPmUnitSensorList(
      const platform_manager::PlatformConfig& platformConfig,
      const std::vector<sensor_config::PmUnitSensors>& pmUnitSensorsList);
  bool isPmValidPmSensors(
      const platform_manager::PlatformConfig& platformConfig,
      const std::string& pmUnitSensorsSlotPath,
      const std::vector<sensor_config::PmSensor>& pmSensors);
  bool isPmValidVersionedPmSensors(
      const platform_manager::PlatformConfig& platformConfig,
      const std::string& slotPath,
      const std::string& pmUnitName,
      const std::vector<sensor_config::VersionedPmSensor>& versionedPmSensors);

 private:
  const std::shared_ptr<platform_manager::ConfigValidator> pmConfigValidator_;
};

} // namespace facebook::fboss::platform::sensor_service
