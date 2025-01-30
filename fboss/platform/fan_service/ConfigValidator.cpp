// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/platform/fan_service/ConfigValidator.h"

#include <folly/logging/xlog.h>
#include <range/v3/range/conversion.hpp>
#include <range/v3/view/transform.hpp>

#include "fboss/platform/fan_service/if/gen-cpp2/fan_service_config_constants.h"
#include "fboss/platform/fan_service/if/gen-cpp2/fan_service_config_types.h"

namespace {
namespace constants =
    facebook::fboss::platform::fan_service::fan_service_config_constants;

std::unordered_set<std::string> accessMethodTypes = {
    constants::ACCESS_TYPE_SYSFS(),
    constants::ACCESS_TYPE_UTIL(),
    constants::ACCESS_TYPE_THRIFT(),
    constants::ACCESS_TYPE_QSFP()};

std::unordered_set<std::string> zoneTypes = {
    constants::ZONE_TYPE_MAX(),
    constants::ZONE_TYPE_MIN(),
    constants::ZONE_TYPE_AVG()};

std::unordered_set<std::string> opticTypes = {
    constants::OPTIC_TYPE_100_GENERIC(),
    constants::OPTIC_TYPE_200_GENERIC(),
    constants::OPTIC_TYPE_400_GENERIC(),
    constants::OPTIC_TYPE_800_GENERIC()};

std::unordered_set<std::string> opticAggregationTypes = {
    constants::OPTIC_AGGREGATION_TYPE_MAX(),
    constants::OPTIC_AGGREGATION_TYPE_PID()};

std::unordered_set<std::string> sensorPwmCalcTypes = {
    constants::SENSOR_PWM_CALC_TYPE_FOUR_LINEAR_TABLE(),
    constants::SENSOR_PWM_CALC_TYPE_PID()};

} // namespace

namespace facebook::fboss::platform::fan_service {
bool ConfigValidator::isValid(const FanServiceConfig& config) {
  if (config.controlInterval()) {
    if (*config.controlInterval()->pwmUpdateInterval() <= 0) {
      XLOG(ERR) << "Invalid pwm update interval: "
                << *config.controlInterval()->pwmUpdateInterval();
      return false;
    }
    if (*config.controlInterval()->sensorReadInterval() <= 0) {
      XLOG(ERR) << "Invalid sensor read interval: "
                << *config.controlInterval()->sensorReadInterval();
      return false;
    }
  }
  for (const auto& sensor : *config.sensors()) {
    if (!accessMethodTypes.count(*sensor.access()->accessType())) {
      XLOG(ERR) << "Invalid access method: " << *sensor.access()->accessType();
      return false;
    }
    if (!sensorPwmCalcTypes.count(*sensor.pwmCalcType())) {
      XLOG(ERR) << "Invalid PWM calculation type: " << *sensor.pwmCalcType();
      return false;
    }
  }

  if (config.watchdog()) {
    if (!config.watchdog()->sysfsPath()->c_str()) {
      XLOG(ERR) << "Watchdog sysfs path must have a non-empty value";
      return false;
    }
  }

  if (*config.pwmTransitionValue() <= 0) {
    XLOG(ERR) << "Transitional PWM value must be greater than 0";
    return false;
  }

  for (const auto& fan : *config.fans()) {
    if (!isValidFanConfig(fan)) {
      return false;
    }
  }

  for (const auto& zone : *config.zones()) {
    if (!isValidZoneConfig(
            zone, *config.fans(), *config.sensors(), *config.optics())) {
      return false;
    }
  }

  for (const auto& optic : *config.optics()) {
    if (!isValidOpticConfig(optic)) {
      return false;
    }
  }

  XLOG(INFO) << "The config is valid";
  return true;
}

bool ConfigValidator::isValidFanConfig(const Fan& fan) {
  if (fan.rpmSysfsPath()->empty()) {
    XLOG(ERR) << "rpmSysfsPath cannot be empty";
    return false;
  }
  if (fan.pwmSysfsPath()->empty()) {
    XLOG(ERR) << "pwmSysfsPath cannot be empty";
    return false;
  }
  if (fan.presenceSysfsPath() && fan.presenceGpio()) {
    XLOG(ERR) << "Both presenceSysfsPath and presenceGpio cannot be set";
    return false;
  }
  if (!fan.presenceSysfsPath() && !fan.presenceGpio()) {
    XLOG(ERR) << "Either presenceSysfsPath or presenceGpio must be set";
    return false;
  }
  return true;
}

bool ConfigValidator::isValidOpticConfig(const Optic& optic) {
  if (optic.opticName()->empty()) {
    XLOG(ERR) << "opticName cannot be empty";
    return false;
  }

  if (!opticAggregationTypes.count(*optic.aggregationType())) {
    XLOG(ERR) << "Invalid optic aggregation type: " << *optic.aggregationType();
    return false;
  }

  if (*optic.aggregationType() == constants::OPTIC_AGGREGATION_TYPE_PID()) {
    if (optic.pidSettings()->empty()) {
      XLOG(ERR) << "PID settings cannot be empty for optic aggregation type: "
                << *optic.aggregationType();
      return false;
    }
    for (const auto& [opticType, pidSetting] : *optic.pidSettings()) {
      if (!opticTypes.count(opticType)) {
        XLOG(ERR) << "Invalid optic type: " << opticType;
        return false;
      }
    }
  }

  if (*optic.aggregationType() == constants::OPTIC_AGGREGATION_TYPE_MAX()) {
    if (optic.tempToPwmMaps()->empty()) {
      XLOG(ERR)
          << "tempToPwmMaps settings cannot be empty for optic aggregation type: "
          << *optic.aggregationType();
      return false;
    }
    for (const auto& [opticType, tempToPwmMap] : *optic.tempToPwmMaps()) {
      if (!opticTypes.count(opticType)) {
        XLOG(ERR) << "Invalid optic type: " << opticType;
        return false;
      }
    }
  }
  return true;
}

bool ConfigValidator::isValidZoneConfig(
    const Zone& zone,
    const std::vector<Fan>& fans,
    const std::vector<Sensor>& sensors,
    const std::vector<Optic>& optics) {
  if (zone.zoneName()->empty()) {
    XLOG(ERR) << "zoneName cannot be empty";
    return false;
  }
  if (!zoneTypes.count(*zone.zoneType())) {
    XLOG(ERR) << "Invalid zone type: " << *zone.zoneType();
    return false;
  }
  auto validFanNames = fans |
      ranges::views::transform([](const auto& fan) { return *fan.fanName(); }) |
      ranges::to<std::unordered_set>;
  auto validSensorNames = sensors |
      ranges::views::transform([](const auto& sensor) {
                            return *sensor.sensorName();
                          }) |
      ranges::to<std::unordered_set>;
  auto validOpticNames = optics |
      ranges::views::transform([](const auto& optic) {
                           return *optic.opticName();
                         }) |
      ranges::to<std::unordered_set>;
  for (const auto& fanName : *zone.fanNames()) {
    if (!validFanNames.count(fanName)) {
      XLOG(ERR) << fmt::format(
          "Invalid fan name: {} in Zone: {}", fanName, *zone.zoneName());
      return false;
    }
  }
  for (const auto& sensorName : *zone.sensorNames()) {
    if (!validSensorNames.count(sensorName) &&
        !validOpticNames.count(sensorName)) {
      XLOG(ERR) << fmt::format(
          "Invalid sensor name: {} in Zone: {}", sensorName, *zone.zoneName());
      return false;
    }
  }
  return true;
}
} // namespace facebook::fboss::platform::fan_service
