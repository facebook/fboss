// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/platform/config_lib/ConfigValidator.h"

#include <fmt/format.h>
#include <folly/logging/xlog.h>
#include <re2/re2.h>

#include "fboss/platform/platform_manager/ConfigValidator.h"
#include "fboss/platform/platform_manager/Utils.h"

namespace {
const re2::RE2 kSensorSymlinkRegex{"(?P<Path>/run/devmap/sensors/.+)(/.+)+"};
}; // namespace

namespace facebook::fboss::platform::cross_service {
ConfigValidator::ConfigValidator(const platform_manager::PlatformConfig& config)
    : pmConfig_(config) {}

bool ConfigValidator::isValid(const sensor_config::SensorConfig& sensorConfig) {
  if (!isPmValidPmUnitSensorList(*sensorConfig.pmUnitSensorsList())) {
    return false;
  }
  return true;
}

bool ConfigValidator::isPmValidPmUnitSensorList(
    const std::vector<sensor_config::PmUnitSensors>& pmUnitSensorsList) {
  for (const auto& pmUnitSensors : pmUnitSensorsList) {
    if (!platform_manager::ConfigValidator().isValidSlotPath(
            pmConfig_, *pmUnitSensors.slotPath())) {
      return false;
    }
    if (!platform_manager::ConfigValidator().isValidPmUnitName(
            pmConfig_,
            *pmUnitSensors.slotPath(),
            *pmUnitSensors.pmUnitName())) {
      return false;
    }
    if (!isPmValidPmSensors(
            *pmUnitSensors.slotPath(), *pmUnitSensors.sensors())) {
      return false;
    }
    if (!pmUnitSensors.versionedSensors()->empty() &&
        !isPmValidVersionedPmSensors(
            *pmUnitSensors.slotPath(),
            *pmUnitSensors.pmUnitName(),
            *pmUnitSensors.versionedSensors())) {
      return false;
    }
  }
  return true;
}

bool ConfigValidator::isPmValidPmSensors(
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
    if (!pmConfig_.symbolicLinkToDevicePath()->contains(symlink)) {
      XLOG(ERR) << fmt::format(
          "{} is not defined in PlatformConfig::symbolicLinkToDevicePath",
          symlink);
      return false;
    }
    auto [slotPath, deviceName] = platform_manager::Utils().parseDevicePath(
        pmConfig_.symbolicLinkToDevicePath()->at(symlink));
    if (pmUnitSensorsSlotPath != slotPath) {
      XLOG(ERR) << fmt::format(
          "PmUnitSensors SlotPath {} doesn't match with SlotPath {} of {}",
          pmUnitSensorsSlotPath,
          slotPath,
          symlink);
      return false;
    }
    if (!platform_manager::ConfigValidator().isValidDeviceName(
            pmConfig_, pmUnitSensorsSlotPath, deviceName)) {
      return false;
    }
  }
  return true;
}

bool ConfigValidator::isPmValidVersionedPmSensors(
    const std::string& slotPath,
    const std::string& pmUnitName,
    const std::vector<sensor_config::VersionedPmSensor>& versionedPmSensors) {
  const auto& pmUnitConfig = pmConfig_.pmUnitConfigs()->at(pmUnitName);
  const auto& slotTypeConfig =
      pmConfig_.slotTypeConfigs()->at(*pmUnitConfig.pluggedInSlotType());
  if (!slotTypeConfig.idpromConfig() && !versionedPmSensors.empty()) {
    XLOG(ERR) << fmt::format(
        "Unexpected VersionedSensors definition for PmUnit {} at {} "
        "where IDPROM is not present.",
        pmUnitName,
        slotPath);
    return false;
  }
  for (const auto& versionedPmSensor : versionedPmSensors) {
    if (!isPmValidPmSensors(slotPath, *versionedPmSensor.sensors())) {
      return false;
    }
  }
  return true;
}
} // namespace facebook::fboss::platform::cross_service
