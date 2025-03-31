// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/platform/fan_service/if/gen-cpp2/fan_service_config_types.h"
#include "fboss/platform/platform_manager/gen-cpp2/platform_manager_config_types.h"
#include "fboss/platform/sensor_service/if/gen-cpp2/sensor_config_types.h"

namespace facebook::fboss::platform {

class CrossConfigValidator {
 public:
  explicit CrossConfigValidator(
      const platform_manager::PlatformConfig& pmConfig);
  bool isValidSensorConfig(const sensor_config::SensorConfig& sensorConfig);
  bool isValidFanServiceConfig(const fan_service::FanServiceConfig& fanConfig);

 private:
  // Sensor Service
  bool isValidPmSensors(
      const std::string& slotPath,
      const std::string& pmUnitName,
      const std::vector<sensor_config::PmSensor>& pmSensors);
  bool isValidVersionedPmSensors(
      const std::string& slotPath,
      const std::string& pmUnitName,
      const std::vector<sensor_config::VersionedPmSensor>& versionedPmSensors);

  // Common
  bool isValidRuntimePath(const std::string& path);

  platform_manager::PlatformConfig pmConfig_;
};

} // namespace facebook::fboss::platform
