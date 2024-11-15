// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/platform/sensor_service/ConfigValidator.h"

#include <fmt/format.h>
#include <folly/logging/xlog.h>
#include <re2/re2.h>

namespace facebook::fboss::platform::sensor_service {
using namespace sensor_config;

ConfigValidator::ConfigValidator(
    const std::shared_ptr<platform_manager::ConfigValidator>& pmConfigValidator)
    : pmConfigValidator_(pmConfigValidator) {}

bool ConfigValidator::isValid(
    const SensorConfig& sensorConfig,
    const std::optional<platform_manager::PlatformConfig>& platformConfig) {
  for (std::unordered_set<std::pair<std::string, std::string>> usedSlotPaths;
       const auto& pmUnitSensors : *sensorConfig.pmUnitSensorsList()) {
    if (!isValidPmUnitSensors(pmUnitSensors, usedSlotPaths)) {
      return false;
    }
  }
  // This is Darwin if platformConfig=std::nullopt
  // Until it onboards PM, we can't cross-validate against PM.
  if (!platformConfig) {
    return true;
  }
  // Cross-validation agains platform_manager::ConfigValidator.
  for (const auto& pmUnitSensors : *sensorConfig.pmUnitSensorsList()) {
    if (!pmConfigValidator_->isValidSlotPath(
            *platformConfig, *pmUnitSensors.slotPath())) {
      return false;
    }
  }
  return true;
}

bool ConfigValidator::isValidPmUnitSensors(
    const PmUnitSensors& pmUnitSensors,
    std::unordered_set<std::pair<std::string, std::string>>& usedSlotPaths) {
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
} // namespace facebook::fboss::platform::sensor_service
