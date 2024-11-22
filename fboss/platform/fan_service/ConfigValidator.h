// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/platform/fan_service/if/gen-cpp2/fan_service_config_types.h"

namespace facebook::fboss::platform::fan_service {

class ConfigValidator {
 public:
  bool isValid(const FanServiceConfig& config);
  bool isValidFanConfig(const Fan& fanConfig);
  bool isValidOpticConfig(const Optic& opticConfig);
  bool isValidSensorConfig(const Sensor& sensorConfig);
  bool isValidZoneConfig(
      const Zone& zoneConfig,
      const std::vector<Fan>& fanConfigs,
      const std::vector<Sensor>& sensorConfigs,
      const std::vector<Optic>& opticConfigs);
};

} // namespace facebook::fboss::platform::fan_service
