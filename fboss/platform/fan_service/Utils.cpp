// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/platform/fan_service/Utils.h"

#include <folly/logging/xlog.h>

#include "fboss/platform/fan_service/if/gen-cpp2/fan_service_config_constants.h"
#include "fboss/platform/fan_service/if/gen-cpp2/fan_service_config_types.h"

namespace {
using constants =
    facebook::fboss::platform::fan_service::fan_service_config_constants;

std::unordered_set<std::string> rangeCheckActions = {
    constants::RANGE_CHECK_ACTION_SHUTDOWN()};

std::unordered_set<std::string> accessMethodTypes = {
    constants::ACCESS_TYPE_SYSFS(),
    constants::ACCESS_TYPE_UTIL(),
    constants::ACCESS_TYPE_THRIFT(),
    constants::ACCESS_TYPE_REST(),
    constants::ACCESS_TYPE_QSFP()};

std::unordered_set<std::string> zoneTypes = {
    constants::ZONE_TYPE_MAX(),
    constants::ZONE_TYPE_MIN(),
    constants::ZONE_TYPE_AVG()};

std::unordered_set<std::string> opticAggregationTypes = {
    constants::OPTIC_AGGREGATION_TYPE_MAX()};
} // namespace

namespace facebook::fboss::platform::fan_service {
bool Utils::isValidConfig(const FanServiceConfig& config) {
  for (const auto& sensor : *config.sensors()) {
    if (sensor.rangeCheck()) {
      if (!rangeCheckActions.count(
              *sensor.rangeCheck()->invalidRangeAction())) {
        XLOG(ERR) << "Invalid range check action: "
                  << *sensor.rangeCheck()->invalidRangeAction();
        return false;
      }
    }
    if (!accessMethodTypes.count(*sensor.access()->accessType())) {
      XLOG(ERR) << "Invalid access method: " << *sensor.access()->accessType();
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

  for (const auto& fan : *config.fans()) {
    if (!accessMethodTypes.count(*fan.rpmAccess()->accessType())) {
      XLOG(ERR) << "Invalid rpmAccess method: "
                << *fan.rpmAccess()->accessType();
      return false;
    }
    if (!accessMethodTypes.count(*fan.pwmAccess()->accessType())) {
      XLOG(ERR) << "Invalid pwmAccess method: "
                << *fan.pwmAccess()->accessType();
      return false;
    }
    if (!accessMethodTypes.count(*fan.presenceAccess()->accessType())) {
      XLOG(ERR) << "Invalid presenceAccess method: "
                << *fan.presenceAccess()->accessType();
      return false;
    }
    if (!accessMethodTypes.count(*fan.ledAccess()->accessType())) {
      XLOG(ERR) << "Invalid ledAccess method: "
                << *fan.ledAccess()->accessType();
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
    if (!opticAggregationTypes.count(*optic.aggregationType())) {
      XLOG(ERR) << "Invalid optic aggregation type: "
                << *optic.aggregationType();
      return false;
    }
  }

  XLOG(INFO) << "The config is valid";
  return true;
}

} // namespace facebook::fboss::platform::fan_service
