// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include <string>

#include "fboss/platform/platform_manager/I2cAddr.h"
#include "fboss/platform/platform_manager/gen-cpp2/platform_manager_config_types.h"

namespace facebook::fboss::platform::platform_manager {

void configureFanCpld(
    uint16_t busNum,
    const I2cAddr& addr,
    const FanCpldConfig& fanCpldConfig);

} // namespace facebook::fboss::platform::platform_manager
