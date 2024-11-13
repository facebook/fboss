// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/platform/sensor_service/ConfigValidator.h"

#include <fmt/format.h>
#include <folly/logging/xlog.h>
#include <re2/re2.h>

namespace facebook::fboss::platform::sensor_service {
using namespace sensor_config;
namespace {
// For more info, see below
// https://github.com/facebook/fboss/blob/main/fboss/platform/platform_manager/platform_manager_config.thrift#L73
const re2::RE2 kSlotPathRe = "/|(/([A-Z]+_)+SLOT@\\d+)+";
} // namespace

bool ConfigValidator::isValid(const SensorConfig& sensorConfig) {
  for (std::unordered_set<std::pair<std::string, std::string>> usedSlotPaths;
       const auto& pmUnitSensors : *sensorConfig.pmUnitSensorsList()) {
    if (!isValidPmUnitSensors(pmUnitSensors, usedSlotPaths)) {
      return false;
    }
  }
  return true;
}

bool ConfigValidator::isValidPmUnitSensors(
    const PmUnitSensors& pmUnitSensors,
    std::unordered_set<std::pair<std::string, std::string>>& usedSlotPaths) {
  if (pmUnitSensors.slotPath()->empty()) {
    XLOG(ERR) << "SlotPath in PmUnitSensor must be non-empty";
    return false;
  }
  if (!isValidSlotPath(*pmUnitSensors.slotPath())) {
    return false;
  }
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
  for (std::unordered_set<std::string> usedSensorNames;
       const auto& pmSensor : *pmUnitSensors.sensors()) {
    if (!isValidPmSensor(pmSensor, usedSensorNames)) {
      return false;
    }
  }
  return true;
}

bool ConfigValidator::isValidPmSensor(
    const PmSensor& pmSensor,
    std::unordered_set<std::string>& usedSensorNames) {
  if (pmSensor.name()->empty()) {
    XLOG(ERR) << "PmSensor name must be non-empty";
    return false;
  }
  if (usedSensorNames.contains(*pmSensor.name())) {
    XLOG(ERR) << fmt::format("SensorName {} is a duplicate", *pmSensor.name());
    return false;
  }
  usedSensorNames.emplace(*pmSensor.name());
  if (!pmSensor.sysfsPath()->starts_with("/run/devmap/sensors/")) {
    XLOG(ERR) << "PmSensor sysfsPath must start with /run/devmap/sensors/";
    return false;
  }
  return true;
}

bool ConfigValidator::isValidSlotPath(const std::string& slotPath) {
  if (!re2::RE2::FullMatch(slotPath, kSlotPathRe)) {
    XLOG(ERR) << fmt::format("SlotPath {} is invalid", slotPath);
    return false;
  }
  return true;
}

} // namespace facebook::fboss::platform::sensor_service
