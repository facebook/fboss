// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include <string>
#include <vector>

#include "fboss/platform/sensor_service/PmUnitInfoFetcher.h"
#include "fboss/platform/sensor_service/if/gen-cpp2/sensor_config_types.h"

namespace facebook::fboss::platform::sensor_service {
using namespace facebook::fboss::platform::sensor_config;

class Utils {
 public:
  static uint64_t nowInSecs();

  // Calculate expression that may contain invalid symbol (for exprtk), e.g.
  // "@", but compatiable with lmsensor style expression and to be replaced by
  // parameter "symbol"; or valid variable (for exprtk) in the expression as
  // indicated by parameter "symbol". Those will be replaced by parameter
  // "input" value to calculate the expression. For example: "expression" is
  // "@*0.1", "input" is 100f, invalid symbol is "@" which will get replaced by
  // valid symbol "x", thus the expression will be x*0.1 = 100f*0.1 = 10f
  static float computeExpression(
      const std::string& expression,
      float input,
      const std::string& symbol = "x");

  std::optional<VersionedPmSensor> resolveVersionedSensors(
      const PmUnitInfoFetcher& fetcher,
      const std::string& slotPath,
      std::vector<VersionedPmSensor> versionedSensors);

  SensorConfig getConfig();
};
} // namespace facebook::fboss::platform::sensor_service
