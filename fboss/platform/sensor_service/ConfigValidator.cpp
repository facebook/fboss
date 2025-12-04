// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/platform/sensor_service/ConfigValidator.h"

#include <fmt/format.h>
#include <folly/logging/xlog.h>
#include <re2/re2.h>
#include <thrift/lib/cpp2/FieldRef.h>

namespace facebook::fboss::platform::sensor_service {
using namespace sensor_config;
namespace {
const re2::RE2 kSensorSymlinkRegex{"(?P<Path>/run/devmap/sensors/.+)(/.+)+"};
}; // namespace

bool ConfigValidator::isValid(const SensorConfig& sensorConfig) {
  XLOG(INFO) << "Validating sensor_service config";
  if (!isValidPmUnitSensorsList(*sensorConfig.pmUnitSensorsList())) {
    return false;
  }
  if (!isValidPowerConfig(sensorConfig)) {
    return false;
  }
  if (!isValidTemperatureConfig(sensorConfig)) {
    return false;
  }
  if (!isValidAsicCommand(sensorConfig)) {
    return false;
  }
  return true;
}

bool ConfigValidator::isValidPmSensor(const sensor_config::PmSensor& pmSensor) {
  if (!pmSensor.name().has_value() || pmSensor.name()->empty()) {
    XLOG(ERR) << "PmSensor name must be non-empty";
    return false;
  }

  // Check if sensor name is all uppercase with underscores and digits
  const auto& name = *pmSensor.name();
  for (char c : name) {
    if (!std::isupper(c) && c != '_' && !std::isdigit(c)) {
      XLOG(ERR) << fmt::format(
          "PmSensor name '{}' must contain only uppercase letters, digits, and "
          " underscores ",
          name);
      return false;
    }
  }

  if (!pmSensor.sysfsPath().has_value() || pmSensor.sysfsPath()->empty()) {
    XLOG(ERR) << "PmSensor sysfsPath must be non-empty";
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
    if (!isValidPmSensor(pmSensor)) {
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

bool ConfigValidator::isValidPowerConfig(
    const sensor_config::SensorConfig& sensorConfig) {
  re2::RE2 psuPattern("((PSU|PEM)([1-9][0-9]*)|HSC)");

  XLOG(DBG1) << "Validating Power Config";

  const auto& powerConfig = *sensorConfig.powerConfig();
  auto sensorNames = getAllSensorNames(sensorConfig);
  auto universalSensorNames = getAllUniversalSensorNames(sensorConfig);
  std::unordered_set<std::string> perSlotPowerConfigNames;

  // perSlotPowerConfigs is mandatory and must not be empty
  if (powerConfig.perSlotPowerConfigs()->empty()) {
    XLOG(ERR) << "perSlotPowerConfigs must be defined and non-empty";
    return false;
  }

  for (const auto& perSlotConfig : *powerConfig.perSlotPowerConfigs()) {
    // Check for duplicate per-slot power config names
    if (perSlotPowerConfigNames.find(*perSlotConfig.name()) !=
        perSlotPowerConfigNames.end()) {
      XLOG(ERR) << fmt::format(
          "perSlotPowerConfig name {} is a duplicate", *perSlotConfig.name());
      return false;
    }
    perSlotPowerConfigNames.insert(*perSlotConfig.name());

    // Check if per-slot power config name conflicts with existing sensor names
    if (sensorNames.find(*perSlotConfig.name()) != sensorNames.end()) {
      XLOG(ERR) << fmt::format(
          "perSlotPowerConfig name {} conflicts with existing sensor name",
          *perSlotConfig.name());
      return false;
    }

    // Validate name pattern (PSU/PEM/HSC)
    if (!RE2::FullMatch(*perSlotConfig.name(), psuPattern)) {
      XLOG(ERR) << fmt::format(
          "perSlotPowerConfig name {} should be PSU[number], PEM[number], or HSC",
          *perSlotConfig.name());
      return false;
    }

    // Validate sensor references
    if (perSlotConfig.powerSensorName().has_value()) {
      if (universalSensorNames.count(*perSlotConfig.powerSensorName()) == 0) {
        XLOG(ERR) << fmt::format(
            "perSlotPowerConfig powerSensorName {} is not defined in"
            " SensorConfig",
            *perSlotConfig.powerSensorName());
        return false;
      }
    } else if (
        perSlotConfig.voltageSensorName().has_value() &&
        perSlotConfig.currentSensorName().has_value()) {
      if (universalSensorNames.count(*perSlotConfig.voltageSensorName()) == 0) {
        XLOG(ERR) << fmt::format(
            "perSlotPowerConfig voltageSensorName {} is not defined in"
            " SensorConfig",
            *perSlotConfig.voltageSensorName());
        return false;
      }

      if (universalSensorNames.count(*perSlotConfig.currentSensorName()) == 0) {
        XLOG(ERR) << fmt::format(
            "perSlotPowerConfig currentSensorName {} is not defined in"
            " SensorConfig",
            *perSlotConfig.currentSensorName());
        return false;
      }
    } else {
      XLOG(ERR) << fmt::format(
          "perSlotPowerConfig {}: Either powerSensorName, or voltageSensorName"
          " and currentSensorName should be defined",
          *perSlotConfig.name());
      return false;
    }
  }

  // Validate otherPowerSensorNames
  for (const auto& sensorName : *powerConfig.otherPowerSensorNames()) {
    if (universalSensorNames.count(sensorName) == 0) {
      XLOG(ERR) << fmt::format(
          "otherPowerSensorName {} is not defined in SensorConfig", sensorName);
      return false;
    }
  }

  // Validate inputVoltageSensors
  for (const auto& sensorName : *powerConfig.inputVoltageSensors()) {
    if (universalSensorNames.count(sensorName) == 0) {
      XLOG(ERR) << fmt::format(
          "inputVoltageSensor {} is not defined in SensorConfig", sensorName);
      return false;
    }
  }

  return true;
}

bool ConfigValidator::isValidTemperatureConfig(
    const sensor_config::SensorConfig& sensorConfig) {
  re2::RE2 asicPattern("ASIC([1-9][0-9]*)?");

  XLOG(DBG1) << "Validating Temperature Config";

  // Validate that temperatureConfigs is not empty
  if (sensorConfig.temperatureConfigs()->empty()) {
    XLOG(ERR) << "temperatureConfigs must not be empty";
    return false;
  }

  auto sensorNames = getAllSensorNames(sensorConfig);
  auto universalSensorNames = getAllUniversalSensorNames(sensorConfig);
  std::unordered_set<std::string> temperatureConfigNames;

  for (const auto& tempConfig : *sensorConfig.temperatureConfigs()) {
    // Check for duplicate temperature config names
    if (temperatureConfigNames.find(*tempConfig.name()) !=
        temperatureConfigNames.end()) {
      XLOG(ERR) << fmt::format(
          "temperatureConfig name `{}` is a duplicate", *tempConfig.name());
      return false;
    }
    temperatureConfigNames.insert(*tempConfig.name());

    // Check if temperature config name conflicts with existing sensor names
    if (sensorNames.find(*tempConfig.name()) != sensorNames.end()) {
      XLOG(ERR) << fmt::format(
          "temperatureConfig name `{}` conflicts with existing sensor name",
          *tempConfig.name());
      return false;
    }

    // Validate temperature config name pattern (ASIC or ASIC[number])
    if (!RE2::FullMatch(*tempConfig.name(), asicPattern)) {
      XLOG(ERR) << fmt::format(
          "temperatureConfig name `{}` should be ASIC or ASIC[number]",
          *tempConfig.name());
      return false;
    }

    // Validate that temperatureSensorNames list is non-empty
    if (tempConfig.temperatureSensorNames()->empty()) {
      XLOG(ERR) << fmt::format(
          "temperatureConfig `{}` must have a non-empty temperatureSensorNames"
          " list ",
          *tempConfig.name());
      return false;
    }

    // Validate that all temperature sensor names exist
    for (const auto& sensorName : *tempConfig.temperatureSensorNames()) {
      if (universalSensorNames.count(sensorName) == 0) {
        XLOG(ERR) << fmt::format(
            "temperatureConfig temperatureSensorName `{}` is not defined in"
            " SensorConfig",
            sensorName);
        return false;
      }
    }
  }

  return true;
}

bool ConfigValidator::isValidAsicCommand(
    const sensor_config::SensorConfig& sensorConfig) {
  if (!sensorConfig.asicCommand().has_value()) {
    // AsicCommand is optional, so it's valid if not present
    return true;
  }

  XLOG(DBG1) << "Validating AsicCommand config";

  const auto& asicCommand = *sensorConfig.asicCommand();

  // Check if sensorName is non-empty
  if (asicCommand.sensorName()->empty()) {
    XLOG(ERR) << "AsicCommand sensorName must be non-empty";
    return false;
  }

  // Check if cmd is non-empty
  if (asicCommand.cmd()->empty()) {
    XLOG(ERR) << "AsicCommand cmd must be non-empty";
    return false;
  }

  // Check if sensor name conflicts with existing sensor names (excluding the
  // asicCmd struct)
  auto sensorConfigCopy = sensorConfig;
  sensorConfigCopy.asicCommand().reset();
  auto sensorNames = getAllSensorNames(sensorConfigCopy);
  if (sensorNames.count(*asicCommand.sensorName()) != 0) {
    XLOG(ERR) << fmt::format(
        "AsicCommand sensorName {} conflicts with existing sensor name",
        *asicCommand.sensorName());
    return false;
  }

  return true;
}

std::unordered_set<std::string> ConfigValidator::getAllSensorNames(
    const sensor_config::SensorConfig& sensorConfig) {
  std::unordered_set<std::string> sensorNames;
  for (const auto& pmUnitSensors : *sensorConfig.pmUnitSensorsList()) {
    // Add base sensors
    for (const auto& pmSensor : *pmUnitSensors.sensors()) {
      sensorNames.emplace(*pmSensor.name());
    }
    // Add versioned sensors
    for (const auto& versionedPmSensor : *pmUnitSensors.versionedSensors()) {
      for (const auto& pmSensor : *versionedPmSensor.sensors()) {
        sensorNames.emplace(*pmSensor.name());
      }
    }
  }
  if (const auto& asicCmd = sensorConfig.asicCommand()) {
    sensorNames.emplace(*asicCmd->sensorName());
  }
  return sensorNames;
}

std::unordered_set<std::string> ConfigValidator::getAllUniversalSensorNames(
    const sensor_config::SensorConfig& sensorConfig) {
  std::unordered_set<std::string> sensorNames;
  for (const auto& pmUnitSensors : *sensorConfig.pmUnitSensorsList()) {
    for (const auto& pmSensor : *pmUnitSensors.sensors()) {
      sensorNames.emplace(*pmSensor.name());
    }
  }
  if (const auto& asicCmd = sensorConfig.asicCommand()) {
    sensorNames.emplace(*asicCmd->sensorName());
  }
  return sensorNames;
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
  if (const auto& asicCmd = sensorConfig.asicCommand()) {
    if (sensorName == asicCmd->sensorName()) {
      return true;
    }
  }
  XLOG(ERR) << fmt::format(
      "Sensor `{}` is not defined in SensorConfig", sensorName);
  return false;
}

} // namespace facebook::fboss::platform::sensor_service
