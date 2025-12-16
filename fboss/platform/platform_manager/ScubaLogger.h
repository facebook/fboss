// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include <string>
#include <unordered_map>

#include "fboss/platform/platform_manager/DataStore.h"
#include "fboss/platform/platform_manager/gen-cpp2/platform_manager_config_types.h"

namespace facebook::fboss::platform::platform_manager {

/**
 * ScubaLogger provides utilities for logging data to Scuba with automatic
 * platform context enrichment. This is a no-op stub in OSS builds.
 */
class ScubaLogger {
 public:
  static constexpr const char* kDefaultCategory =
      "perfpipe_fboss_platform_manager";

  explicit ScubaLogger(
      const std::string& platformName,
      const DataStore& dataStore_);

  // Log an event to Scuba with string and int fields.
  // Automatically adds: platform, hostname, time, and persistent fields.
  void log(
      const std::string& event,
      const std::unordered_map<std::string, std::string>& normals = {},
      const std::unordered_map<std::string, int64_t>& ints = {},
      const std::string& category = kDefaultCategory);

  // Log an unexpected crash event to Scuba.
  void logCrash(
      const std::exception& ex,
      const std::unordered_map<std::string, std::string>& additionalFields =
          {});

 private:
  void addPlatformFields(std::unordered_map<std::string, std::string>& normals);

  std::string platformName_;
  const DataStore& dataStore_;
  std::unordered_map<std::string, std::string> persistentFields_;
};

} // namespace facebook::fboss::platform::platform_manager
