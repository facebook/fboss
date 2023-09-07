// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/platform/platform_manager/gen-cpp2/platform_manager_config_types.h"

namespace facebook::fboss::platform::platform_manager {

class PlatformValidator {
 public:
  bool isValid(const PlatformConfig& platformConfig);
  bool isValidSlotTypeConfig(const SlotTypeConfig& slotTypeConfig);
  bool isValidI2cDeviceConfig(const I2cDeviceConfig& i2cDeviceConfig);
};

} // namespace facebook::fboss::platform::platform_manager
