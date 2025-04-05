// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/platform/sensor_service/if/gen-cpp2/sensor_config_types.h"

namespace facebook::fboss::platform::sensor_service {
class ConfigValidator {
 public:
  bool isValid(const sensor_config::SensorConfig& sensorConfig);
  // Local validation
  bool isValidPmUnitSensorsList(
      const std::vector<sensor_config::PmUnitSensors>& pmUnitSensorsList);
  bool isValidPmSensors(const std::vector<sensor_config::PmSensor>& pmSensor);
  bool isValidSlotPath(const std::string& slotPath);
  // Cross service validation
  bool isValidSensorName(
      const sensor_config::SensorConfig& sensorConfig,
      const std::string& sensorName);
};
} // namespace facebook::fboss::platform::sensor_service
