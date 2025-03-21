// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/platform/platform_manager/gen-cpp2/platform_manager_config_types.h"
#include "fboss/platform/sensor_service/if/gen-cpp2/sensor_config_types.h"

namespace facebook::fboss::platform::cross_service {

class ConfigValidator {
 public:
  explicit ConfigValidator(const platform_manager::PlatformConfig& pmConfig);
  bool isValid(const sensor_config::SensorConfig& sensorConfig);

 private:
  // Sensor Service
  bool isPmValidPmUnitSensorList(
      const std::vector<sensor_config::PmUnitSensors>& pmUnitSensorsList);
  bool isPmValidPmSensors(
      const std::string& pmUnitSensorsSlotPath,
      const std::vector<sensor_config::PmSensor>& pmSensors);
  bool isPmValidVersionedPmSensors(
      const std::string& slotPath,
      const std::string& pmUnitName,
      const std::vector<sensor_config::VersionedPmSensor>& versionedPmSensors);

  platform_manager::PlatformConfig pmConfig_;
};

} // namespace facebook::fboss::platform::cross_service
