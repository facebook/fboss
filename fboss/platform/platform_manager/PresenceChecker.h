// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/platform/platform_manager/DevicePathResolver.h"
#include "fboss/platform/platform_manager/Utils.h"

namespace facebook::fboss::platform::platform_manager {

class PresenceChecker {
 public:
  explicit PresenceChecker(
      const DevicePathResolver& devicePathResolver,
      const std::shared_ptr<Utils> utils = std::make_shared<Utils>());

  bool isPresent(
      const PresenceDetection& presenceDetection,
      const std::string slotPath);

 private:
  const DevicePathResolver& devicePathResolver_;
  const std::shared_ptr<Utils> utils_;

  bool sysfsPresent(const SysfsFileHandle& handle, const std::string& slotPath);
  bool gpioPresent(const GpioLineHandle& handle, const std::string& slotPath);
};

} // namespace facebook::fboss::platform::platform_manager
