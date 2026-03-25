// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/platform/sensor_service/utilities/ConfigDiffer.h"

#include <iostream>

#include <fmt/core.h>
#include <folly/logging/xlog.h>

#include <thrift/lib/cpp2/protocol/Serializer.h>

namespace facebook::fboss::platform::sensor_service {

constexpr const char* kArrow = " \u2192 ";

void ConfigDiffer::addDifference(
    const std::string& pmUnitName,
    const std::string& versionInfo,
    DiffType type,
    const std::string& sensorName,
    const std::string& fieldName,
    const std::optional<std::string>& oldValue,
    const std::optional<std::string>& newValue) {
  diffs_.push_back(
      ConfigDifference{
          .type = type,
          .pmUnitName = pmUnitName,
          .versionInfo = versionInfo,
          .sensorName = sensorName,
          .fieldName = fieldName,
          .oldValue = oldValue,
          .newValue = newValue,
      });
}

void ConfigDiffer::comparePmSensors(
    const std::vector<sensor_config::PmSensor>& sensors1,
    const std::vector<sensor_config::PmSensor>& sensors2,
    const std::string& pmUnitName,
    const std::string& versionInfo) {
  std::set<std::string> set1;
  std::set<std::string> set2;

  for (const auto& sensor : sensors1) {
    set1.insert(*sensor.name());
  }

  for (const auto& sensor : sensors2) {
    set2.insert(*sensor.name());
  }

  for (const auto& name : set2) {
    if (set1.find(name) == set1.end()) {
      addDifference(pmUnitName, versionInfo, DiffType::ADDED, name, "[ADDED]");
    }
  }
}

void ConfigDiffer::compareVersionedSensors(
    const sensor_config::SensorConfig& config) {
  XLOGF(INFO, "INFO: Comparing versioned sensors for all PmUnits");

  const auto& pmUnitSensorsList = config.pmUnitSensorsList().value();
  for (const auto& pmUnitSensors : pmUnitSensorsList) {
    compareVersionedSensors(config, *pmUnitSensors.pmUnitName());
  }
}

void ConfigDiffer::compareVersionedSensors(
    const sensor_config::SensorConfig& config,
    const std::string& pmUnitName) {
  XLOGF(INFO, "INFO: Comparing versioned sensors for PmUnit: {}", pmUnitName);

  const auto& pmUnitSensorsList = config.pmUnitSensorsList().value();

  std::optional<sensor_config::PmUnitSensors> defaultConfig = std::nullopt;
  for (const auto& pmUnitSensors : pmUnitSensorsList) {
    if (*pmUnitSensors.pmUnitName() == pmUnitName) {
      defaultConfig = pmUnitSensors;
      break;
    }
  }

  if (!defaultConfig || defaultConfig->pmUnitName()->empty()) {
    XLOGF(ERR, "ERR: No default config found for PmUnit '{}'", pmUnitName);
    return;
  }

  if (defaultConfig->sensors()->empty()) {
    XLOGF(
        ERR,
        "ERR: No default sensors found or exist for PmUnit '{}'",
        pmUnitName);
  }

  // Moved the following code inside the function body, after checking
  // defaultConfig
  const auto& versionedSensors = defaultConfig->versionedSensors().value();
  if (versionedSensors.empty()) {
    XLOGF(
        INFO,
        "INFO: No versioned sensors found or exist for PmUnit '{}'",
        pmUnitName);
    return;
  }

  for (const auto& versionedSensor : versionedSensors) {
    auto versionInfo = fmt::format(
        "default{}v{}.{}.{}",
        kArrow,
        *versionedSensor.productProductionState(),
        *versionedSensor.productVersion(),
        *versionedSensor.productSubVersion());

    comparePmSensors(
        *defaultConfig->sensors(),
        *versionedSensor.sensors(),
        pmUnitName,
        versionInfo);
  }
}

void ConfigDiffer::printDifferences() {
  if (diffs_.empty()) {
    std::cout << "No differences found.\n";
    return;
  }

  std::map<
      std::pair<std::string, std::string>,
      std::vector<const ConfigDifference*>>
      grouped;
  for (const auto& diff : diffs_) {
    grouped[{diff.pmUnitName, diff.versionInfo}].push_back(&diff);
  }

  for (const auto& [key, section_diffs] : grouped) {
    const auto& [pmUnitName, versionInfo] = key;

    std::cout << "\n[" << pmUnitName << " (" << versionInfo << ")]\n";

    std::map<std::string, std::vector<const ConfigDifference*>> bySensor;
    for (const auto* diff : section_diffs) {
      bySensor[diff->sensorName].push_back(diff);
    }

    for (const auto& [sensorName, sensor_diffs] : bySensor) {
      std::cout << "  " << sensorName << ":";

      bool first = true;
      for (const auto* diff : sensor_diffs) {
        if (!first) {
          std::cout << ",";
        }
        first = false;

        std::cout << " " << diff->fieldName;
        if (diff->oldValue || diff->newValue) {
          std::cout << " (";
          if (diff->oldValue) {
            std::cout << *diff->oldValue;
          }
          if (diff->oldValue && diff->newValue) {
            std::cout << kArrow;
          }
          if (diff->newValue) {
            std::cout << *diff->newValue;
          }
          std::cout << ")";
        }
      }

      std::cout << "\n";
    }
  }

  std::cout << "\n";
}

const std::vector<ConfigDifference>& ConfigDiffer::getDiffs() const {
  return diffs_;
}

} // namespace facebook::fboss::platform::sensor_service
