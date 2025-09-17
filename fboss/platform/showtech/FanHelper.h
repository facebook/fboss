// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include <string>
#include <tuple>

#include "fboss/platform/fan_service/if/gen-cpp2/fan_service_config_types.h"

namespace facebook::fboss::platform {

bool readFanIsPresentOnDevice(const fan_service::Fan& fan);
int readFanRpm(const fan_service::Fan& fan);
int readFanPwmPercent(const fan_service::Fan& fan);
float readSysfsAsFloat(const std::string& path);

} // namespace facebook::fboss::platform
