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
  bool isValidPmSensor(const sensor_config::PmSensor& pmSensor);
  bool isValidPlatformName(const sensor_config::SensorConfig& sensorConfig);
  bool isValidTemperatureSensorThresholds(
      const sensor_config::SensorConfig& sensorConfig);
  bool isValidPowerConfig(const sensor_config::SensorConfig& sensorConfig);
  bool isValidTemperatureConfig(
      const sensor_config::SensorConfig& sensorConfig);
  bool isValidAsicCommand(const sensor_config::SensorConfig& sensorConfig);
  // Union of all defined names (base + every versionedSensors entry +
  // asicCommand). Use for collision checks.
  std::unordered_set<std::string> getAllSensorNames(
      const sensor_config::SensorConfig& sensorConfig);
  // Names guaranteed to exist on every hardware version: base + asicCommand +
  // per-PmUnit intersection of versionedSensors entries. Use for reference
  // validation.
  std::unordered_set<std::string> getAllUniversalSensorNames(
      const sensor_config::SensorConfig& sensorConfig);

  // Cross service validation
  bool isValidSensorName(
      const sensor_config::SensorConfig& sensorConfig,
      const std::string& sensorName);
};
} // namespace facebook::fboss::platform::sensor_service
