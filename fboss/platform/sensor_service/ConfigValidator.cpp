// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/platform/sensor_service/ConfigValidator.h"

#include <fmt/format.h>
#include <folly/logging/xlog.h>
#include <re2/re2.h>

namespace facebook::fboss::platform::sensor_service {
using namespace sensor_config;
namespace {
const re2::RE2 kSensorSymlinkRegex{"(?P<Path>/run/devmap/sensors/.+)(/.+)+"};
}; // namespace

bool ConfigValidator::isValid(const SensorConfig& sensorConfig) {
  if (!isValidPmUnitSensorsList(*sensorConfig.pmUnitSensorsList())) {
    return false;
  }
  return true;
}

bool ConfigValidator::isValidPmUnitSensorsList(
    const std::vector<sensor_config::PmUnitSensors>& pmUnitSensorsList) {
  std::unordered_set<std::pair<std::string, std::string>> usedSlotPaths;
  for (const auto& pmUnitSensors : pmUnitSensorsList) {
    if (usedSlotPaths.contains(
            {*pmUnitSensors.slotPath(), *pmUnitSensors.pmUnitName()})) {
      XLOG(ERR) << fmt::format(
          "(SlotPath {}, PmUnitName {}) is a duplicate",
          *pmUnitSensors.slotPath(),
          *pmUnitSensors.pmUnitName());
      return false;
    }
    usedSlotPaths.insert(
        {*pmUnitSensors.slotPath(), *pmUnitSensors.pmUnitName()});
    if (!isValidPmSensors(*pmUnitSensors.sensors())) {
      return false;
    }
    for (const auto& versionedPmSensor : *pmUnitSensors.versionedSensors()) {
      if (!isValidPmSensors(*versionedPmSensor.sensors())) {
        return false;
      }
    }
  }
  return true;
}

bool ConfigValidator::isValidPmSensors(const std::vector<PmSensor>& pmSensors) {
  std::unordered_set<std::string> usedSensorNames;
  for (const auto& pmSensor : pmSensors) {
    if (pmSensor.name()->empty()) {
      XLOG(ERR) << "PmSensor name must be non-empty";
      return false;
    }
    if (usedSensorNames.contains(*pmSensor.name())) {
      XLOG(ERR) << fmt::format(
          "SensorName {} is a duplicate", *pmSensor.name());
      return false;
    }
    usedSensorNames.emplace(*pmSensor.name());
    if (!pmSensor.sysfsPath()->starts_with("/run/devmap/sensors/")) {
      XLOG(ERR) << "PmSensor sysfsPath must start with /run/devmap/sensors/";
      return false;
    }
  }
  return true;
}

bool ConfigValidator::isValidSensorName(
    const sensor_config::SensorConfig& sensorConfig,
    const std::string& sensorName) {
  for (const auto& pmUnitSensors : *sensorConfig.pmUnitSensorsList()) {
    for (const auto& pmSensor : *pmUnitSensors.sensors()) {
      if (sensorName == *pmSensor.name()) {
        return true;
      }
    }
  }
  XLOG(ERR) << fmt::format(
      "Sensor `{}` is not defined in SensorConfig", sensorName);
  return false;
}

} // namespace facebook::fboss::platform::sensor_service
