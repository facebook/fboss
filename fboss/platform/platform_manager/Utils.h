// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/platform/platform_manager/gen-cpp2/platform_manager_config_types.h"

namespace facebook::fboss::platform::platform_manager {

class Utils {
 public:
  PlatformConfig getConfig(const std::string& configFile);
};

} // namespace facebook::fboss::platform::platform_manager
