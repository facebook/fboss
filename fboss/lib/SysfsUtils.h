// Copyright (c) Meta Platforms, Inc. and affiliates.

#pragma once

#include <filesystem>

// This file contains the Linux sysfs paths/files and relevant utilities
// that are widely used by FBOSS applications.

namespace facebook::fboss {

const std::filesystem::path sysI2cDevices = "/sys/bus/i2c/devices";
const std::filesystem::path sysI2cDrivers = "/sys/bus/i2c/drivers";

const std::filesystem::path sysLeds = "/sys/class/leds";

} // namespace facebook::fboss
