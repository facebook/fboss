// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/platform/platform_manager/DevicePathResolver.h"

namespace facebook::fboss::platform::platform_manager {
DevicePathResolver::DevicePathResolver(const PlatformConfig& config)
    : platformConfig_(config) {}

} // namespace facebook::fboss::platform::platform_manager
