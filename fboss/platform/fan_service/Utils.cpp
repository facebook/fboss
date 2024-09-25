// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/platform/fan_service/Utils.h"

#include <folly/logging/xlog.h>

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
    constants::SENSOR_PWM_CALC_TYPE_INCREMENT_PID(),
    constants::SENSOR_PWM_CALC_TYPE_PID()};

} // namespace

namespace facebook::fboss::platform::fan_service {
bool Utils::isValidConfig(const FanServiceConfig& config) {
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
    if (!accessMethodTypes.count(*config.watchdog()->access()->accessType())) {
      XLOG(ERR) << "Invalid access method for watchdog config: "
                << *config.watchdog()->access()->accessType();
      return false;
    }
  }

  if (*config.pwmTransitionValue() <= 0) {
    XLOG(ERR) << "Transitional PWM value must be greater than 0";
    return false;
  }

  for (const auto& fan : *config.fans()) {
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
  }

  for (const auto& zone : *config.zones()) {
    if (!zoneTypes.count(*zone.zoneType())) {
      XLOG(ERR) << "Invalid zone type: " << *zone.zoneType();
      return false;
    }
  }

  for (const auto& optic : *config.optics()) {
    for (const auto& [opticType, tempToPwmMap] : *optic.tempToPwmMaps()) {
      if (!opticTypes.count(opticType)) {
        XLOG(ERR) << "Invalid optic type: " << opticType;
        return false;
      }
    }
    if (!opticAggregationTypes.count(*optic.aggregationType())) {
      XLOG(ERR) << "Invalid optic aggregation type: "
                << *optic.aggregationType();
      return false;
    }
    if (*optic.aggregationType() == constants::OPTIC_AGGREGATION_TYPE_PID()) {
      if (optic.pidSettings()->empty()) {
        XLOG(ERR) << "PID settings cannot be empty for optic aggregation type: "
                  << *optic.aggregationType();
        return false;
      }
    } else if (
        *optic.aggregationType() == constants::OPTIC_AGGREGATION_TYPE_MAX()) {
      if (optic.tempToPwmMaps()->empty()) {
        XLOG(ERR)
            << "tempToPwmMaps settings cannot be empty for optic aggregation type: "
            << *optic.aggregationType();
        return false;
      }
    }
  }

  XLOG(INFO) << "The config is valid";
  return true;
}

} // namespace facebook::fboss::platform::fan_service
