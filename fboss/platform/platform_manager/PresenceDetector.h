// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/platform/platform_manager/gen-cpp2/platform_manager_config_types.h"

namespace facebook::fboss::platform::platform_manager {

class PresenceDetector {
 public:
  // Checks for the presence of a FRU based on the passed `presenceDetection`
  bool isPresent(const PresenceDetection& presenceDetection) {
    throw std::runtime_error("Not implemented");
  }
};

} // namespace facebook::fboss::platform::platform_manager
