// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/platform/sensor_service/if/gen-cpp2/sensor_config_types.h"

namespace facebook::fboss::platform::sensor_service {
class ConfigValidator {
 public:
  bool isValid(const sensor_config::SensorConfig& sensorConfig);
  bool isValidPmUnitSensors(
      const sensor_config::PmUnitSensors& PmUnitSensors,
      std::unordered_set<std::string>& usedSlotPaths);
  bool isValidPmSensor(
      const sensor_config::PmSensor& pmSensor,
      std::unordered_set<std::string>& usedSensorNames);
  bool isValidSlotPath(const std::string& slotPath);
};

} // namespace facebook::fboss::platform::sensor_service
