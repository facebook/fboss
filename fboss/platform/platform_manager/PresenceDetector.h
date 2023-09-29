// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include <stdexcept>
#include "fboss/platform/platform_manager/gen-cpp2/platform_manager_config_types.h"

namespace facebook::fboss::platform::platform_manager {

class PresenceDetector {
 public:
  // Checks for the presence of a PmUnit based on the passed `presenceDetection`
  bool isPresent(const PresenceDetection& presenceDetection) {
    // We will always return true for now. Once we figure out the implementation
    // details, this will be changed.
    throw std::runtime_error("Presence Detection not implemented yet");
  }
};

} // namespace facebook::fboss::platform::platform_manager
