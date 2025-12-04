// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/platform/platform_manager/ScubaLogger.h"

#include <folly/logging/xlog.h>

namespace facebook::fboss::platform::platform_manager {
namespace ScubaLogger {

void log(
    const std::unordered_map<std::string, std::string>& normals,
    const std::unordered_map<std::string, int64_t>& ints,
    const std::string& category) {
  // No-op: Scuba logging is not available in OSS builds
  XLOG(DBG3) << "Scuba logging is not available in OSS builds. ";
}

} // namespace ScubaLogger
} // namespace facebook::fboss::platform::platform_manager
