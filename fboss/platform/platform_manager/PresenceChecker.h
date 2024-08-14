// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/platform/platform_manager/DevicePathResolver.h"

namespace facebook::fboss::platform::platform_manager {

class PresenceChecker {
 public:
  explicit PresenceChecker(const DevicePathResolver& devicePathResolver);

  bool isPresent(
      const PresenceDetection& presenceDetection,
      const std::string slotPath);

 private:
  const DevicePathResolver& devicePathResolver_;
};

} // namespace facebook::fboss::platform::platform_manager
