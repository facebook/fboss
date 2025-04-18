// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/platform/config_lib/CrossConfigValidator.h"

#include <fmt/format.h>
#include <folly/logging/xlog.h>
#include <re2/re2.h>

#include "fboss/platform/platform_manager/ConfigValidator.h"
#include "fboss/platform/sensor_service/ConfigValidator.h"

namespace {
const re2::RE2 kRuntimePathRegex{
    "(?P<Path>/run/devmap/(sensors|gpiochips|fpgas)/[^/]+)($|(/.+)+)"};
}; // namespace

namespace facebook::fboss::platform {
CrossConfigValidator::CrossConfigValidator(
    const platform_manager::PlatformConfig& config)
    : pmConfig_(config) {}

bool CrossConfigValidator::isValidSensorConfig(
    const sensor_config::SensorConfig& sensorConfig) {
  for (const auto& pmUnitSensors : *sensorConfig.pmUnitSensorsList()) {
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
    if (!isValidPmSensors(
            *pmUnitSensors.slotPath(),
            *pmUnitSensors.pmUnitName(),
            *pmUnitSensors.sensors())) {
      return false;
    }
    if (!pmUnitSensors.versionedSensors()->empty() &&
        !isValidVersionedPmSensors(
            *pmUnitSensors.slotPath(),
            *pmUnitSensors.pmUnitName(),
            *pmUnitSensors.versionedSensors())) {
      return false;
    }
  }
  return true;
}

bool CrossConfigValidator::isValidFanServiceConfig(
    const fan_service::FanServiceConfig& fanConfig,
    const std::optional<sensor_config::SensorConfig>& sensorConfig) {
  for (const auto& fan : *fanConfig.fans()) {
    if (!isValidRuntimePath(*fan.rpmSysfsPath()) ||
        !isValidRuntimePath(*fan.pwmSysfsPath())) {
      return false;
    }
    if (fan.presenceSysfsPath() &&
        !isValidRuntimePath(*fan.presenceSysfsPath())) {
      return false;
    }
    if (fan.presenceGpio() &&
        !isValidRuntimePath(*fan.presenceGpio()->path())) {
      return false;
    }
  }
  if (!fanConfig.sensors()->empty() && !sensorConfig) {
    XLOG(ERR) << fmt::format(
        "Failed validate SensorNames in FanServiceConfig. "
        "Undefined SensorConfig in {}",
        *pmConfig_.platformName());
    return false;
  }
  for (const auto& sensor : *fanConfig.sensors()) {
    if (!sensor_service::ConfigValidator().isValidSensorName(
            *sensorConfig, *sensor.sensorName())) {
      return false;
    }
  }
  return true;
}

bool CrossConfigValidator::isValidPmSensors(
    const std::string& slotPath,
    const std::string& pmUnitName,
    const std::vector<sensor_config::PmSensor>& pmSensors) {
  for (const auto& pmSensor : pmSensors) {
    if (!isValidRuntimePath(*pmSensor.sysfsPath())) {
      return false;
    }
  }
  return true;
}

bool CrossConfigValidator::isValidVersionedPmSensors(
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
    if (!isValidPmSensors(slotPath, pmUnitName, *versionedPmSensor.sensors())) {
      return false;
    }
  }
  return true;
}

bool CrossConfigValidator::isValidRuntimePath(const std::string& path) {
  std::string symlink;
  if (!re2::RE2::FullMatch(path, kRuntimePathRegex, &symlink)) {
    XLOG(ERR) << fmt::format("Fail to extract runtime path from {}", path);
    return false;
  }
  if (!pmConfig_.symbolicLinkToDevicePath()->contains(symlink)) {
    XLOG(ERR) << fmt::format(
        "{} is not defined in PlatformConfig::symbolicLinkToDevicePath",
        symlink);
    return false;
  }
  return true;
}
} // namespace facebook::fboss::platform
