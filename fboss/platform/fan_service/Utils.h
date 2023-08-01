// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/platform/fan_service/if/gen-cpp2/fan_service_config_types.h"

namespace facebook::fboss::platform::fan_service {

class Utils {
 public:
  bool isValidConfig(const FanServiceConfig& config);
};

} // namespace facebook::fboss::platform::fan_service
