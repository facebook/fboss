// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include <string>
#include <vector>

#include "fboss/platform/sensor_service/if/gen-cpp2/sensor_config_types.h"

namespace facebook::fboss::platform::sensor_service {

enum class DiffType {
  ADDED,
};

struct ConfigDifference {
  DiffType type;
  std::string pmUnitName;
  std::string versionInfo;
  std::string sensorName;
  std::string fieldName;
  std::optional<std::string> oldValue;
  std::optional<std::string> newValue;
};

class ConfigDiffer {
 public:
  void comparePmSensors(
      const std::vector<sensor_config::PmSensor>& sensors1,
      const std::vector<sensor_config::PmSensor>& sensors2,
      const std::string& pmUnitName,
      const std::string& versionInfo);

  void compareVersionedSensors(const sensor_config::SensorConfig& config);

  void compareVersionedSensors(
      const sensor_config::SensorConfig& config,
      const std::string& pmUnitName);

  void printDifferences();

  const std::vector<ConfigDifference>& getDiffs() const;

 private:
  void addDifference(
      const std::string& pmUnitName,
      const std::string& versionInfo,
      DiffType type,
      const std::string& sensorName,
      const std::string& fieldName,
      const std::optional<std::string>& oldValue = std::nullopt,
      const std::optional<std::string>& newValue = std::nullopt);

  std::vector<ConfigDifference> diffs_;
};

} // namespace facebook::fboss::platform::sensor_service
