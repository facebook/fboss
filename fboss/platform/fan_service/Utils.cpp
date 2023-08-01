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
  }
  return true;
}

} // namespace facebook::fboss::platform::fan_service
