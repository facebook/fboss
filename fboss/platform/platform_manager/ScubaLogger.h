// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include <string>
#include <unordered_map>

namespace facebook::fboss::platform::platform_manager {

/**
 * ScubaLogger provides utilities for logging data to Scuba.
 * In OSS builds, this is a no-op stub.
 * In internal builds, this logs data via ScribeClient.
 */
namespace ScubaLogger {

// Default Scuba category for Platform Manager logs
constexpr const char* kDefaultCategory = "perfpipe_fboss_platform_manager";

/**
 * Log an event to Scuba with normal (string) and int fields.
 *
 * Note: The following fields are automatically added:
 *   - "hostname" (string): The hostname from getLocalHost()
 *   - "time" (int): Unix timestamp in seconds
 *
 * @param normals Map of string field names to string values
 * @param ints Map of string field names to int64_t values
 * @param category The Scuba category to log to
 */
void log(
    const std::unordered_map<std::string, std::string>& normals,
    const std::unordered_map<std::string, int64_t>& ints = {},
    const std::string& category = kDefaultCategory);

} // namespace ScubaLogger

} // namespace facebook::fboss::platform::platform_manager
