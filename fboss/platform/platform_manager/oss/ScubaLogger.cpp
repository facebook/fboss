// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/platform/platform_manager/ScubaLogger.h"
#include "fboss/platform/platform_manager/DataStore.h"
#include "fboss/platform/platform_manager/gen-cpp2/platform_manager_config_types.h"

#include <folly/logging/xlog.h>

namespace facebook::fboss::platform::platform_manager {

ScubaLogger::ScubaLogger(
    const std::string& platformName,
    const DataStore& dataStore)
    : platformName_(platformName), dataStore_(dataStore) {
  // No-op: Scuba logging is not available in OSS builds
}

void ScubaLogger::addPlatformFields(
    std::unordered_map<std::string, std::string>& normals) {
  // No-op: Scuba logging is not available in OSS builds
}

void ScubaLogger::log(
    const std::string& event,
    const std::unordered_map<std::string, std::string>& normals,
    const std::unordered_map<std::string, int64_t>& ints,
    const std::string& category) {
  // No-op: Scuba logging is not available in OSS builds
  XLOG(DBG3) << "Scuba logging is not available in OSS builds. Event: "
             << event;
}

void ScubaLogger::logCrash(
    const std::exception& ex,
    const std::unordered_map<std::string, std::string>& additionalFields) {
  // No-op: Scuba logging is not available in OSS builds
  XLOG(DBG3) << "Scuba logging is not available in OSS builds. Crash: "
             << ex.what();
}

} // namespace facebook::fboss::platform::platform_manager
