// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/platform/sensor_service/ConfigValidator.h"

#include <fmt/format.h>
#include <folly/logging/xlog.h>
#include <re2/re2.h>

#include "fboss/platform/platform_manager/Utils.h"

namespace facebook::fboss::platform::sensor_service {
using namespace sensor_config;
namespace {
const re2::RE2 kSensorSymlinkRegex{"(?P<Path>/run/devmap/sensors/.+)(/.+)+"};
}; // namespace

ConfigValidator::ConfigValidator(
    const std::shared_ptr<platform_manager::ConfigValidator>& pmConfigValidator)
    : pmConfigValidator_(pmConfigValidator) {}

bool ConfigValidator::isValid(
    const SensorConfig& sensorConfig,
    const std::optional<platform_manager::PlatformConfig>& platformConfig) {
  if (!isValidPmUnitSensorsList(*sensorConfig.pmUnitSensorsList())) {
    return false;
  }
  // This is Darwin if platformConfig=std::nullopt
  // Until it onboards PM, we can't cross-validate against PM.
  if (!platformConfig) {
    return true;
  }
  // Cross-validation agains platform_manager::ConfigValidator.
  if (!isPmValidPmUnitSensorList(
          *platformConfig, *sensorConfig.pmUnitSensorsList())) {
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

bool ConfigValidator::isPmValidPmUnitSensorList(
    const platform_manager::PlatformConfig& platformConfig,
    const std::vector<sensor_config::PmUnitSensors>& pmUnitSensorsList) {
  for (const auto& pmUnitSensors : pmUnitSensorsList) {
    if (!pmConfigValidator_->isValidSlotPath(
            platformConfig, *pmUnitSensors.slotPath())) {
      return false;
    }
    if (!pmConfigValidator_->isValidPmUnitName(
            platformConfig,
            *pmUnitSensors.slotPath(),
            *pmUnitSensors.pmUnitName())) {
      return false;
    }
    if (!isPmValidPmSensors(
            platformConfig,
            *pmUnitSensors.slotPath(),
            *pmUnitSensors.sensors())) {
      return false;
    }
    if (!pmUnitSensors.versionedSensors()->empty() &&
        !isPmValidVersionedPmSensors(
            platformConfig,
            *pmUnitSensors.slotPath(),
            *pmUnitSensors.pmUnitName(),
            *pmUnitSensors.versionedSensors())) {
      return false;
    }
  }
  return true;
}

bool ConfigValidator::isPmValidPmSensors(
    const platform_manager::PlatformConfig& platformConfig,
    const std::string& pmUnitSensorsSlotPath,
    const std::vector<sensor_config::PmSensor>& pmSensors) {
  for (const auto& pmSensor : pmSensors) {
    std::string symlink;
    if (!re2::RE2::FullMatch(
            *pmSensor.sysfsPath(), kSensorSymlinkRegex, &symlink)) {
      XLOG(ERR) << fmt::format(
          "Fail to extract sensor symlink from {}", *pmSensor.sysfsPath());
      return false;
    }
    if (!platformConfig.symbolicLinkToDevicePath()->contains(symlink)) {
      XLOG(ERR) << fmt::format(
          "{} is not defined in PlatformConfig::symbolicLinkToDevicePath",
          symlink);
      return false;
    }
    auto [slotPath, deviceName] = platform_manager::Utils().parseDevicePath(
        platformConfig.symbolicLinkToDevicePath()->at(symlink));
    if (pmUnitSensorsSlotPath != slotPath) {
      XLOG(ERR) << fmt::format(
          "PmUnitSensors SlotPath {} doesn't match with SlotPath {} of {}",
          pmUnitSensorsSlotPath,
          slotPath,
          symlink);
      return false;
    }
    if (!pmConfigValidator_->isValidDeviceName(
            platformConfig, pmUnitSensorsSlotPath, deviceName)) {
      return false;
    }
  }
  return true;
}

bool ConfigValidator::isPmValidVersionedPmSensors(
    const platform_manager::PlatformConfig& platformConfig,
    const std::string& slotPath,
    const std::string& pmUnitName,
    const std::vector<VersionedPmSensor>& versionedPmSensors) {
  const auto& pmUnitConfig = platformConfig.pmUnitConfigs()->at(pmUnitName);
  const auto& slotTypeConfig =
      platformConfig.slotTypeConfigs()->at(*pmUnitConfig.pluggedInSlotType());
  if (!slotTypeConfig.idpromConfig() && !versionedPmSensors.empty()) {
    XLOG(ERR) << fmt::format(
        "Unexpected VersionedSensors definition for PmUnit {} at {} "
        "where IDPROM is not present.",
        pmUnitName,
        slotPath);
    return false;
  }
  for (const auto& versionedPmSensor : versionedPmSensors) {
    if (!isPmValidPmSensors(
            platformConfig, slotPath, *versionedPmSensor.sensors())) {
      return false;
    }
  }
  return true;
}
} // namespace facebook::fboss::platform::sensor_service
