// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/platform/platform_manager/gen-cpp2/platform_manager_config_types.h"

namespace facebook::fboss::platform::platform_manager {

class PkgUtils {
 public:
  void run(const PlatformConfig& config);
};

} // namespace facebook::fboss::platform::platform_manager
