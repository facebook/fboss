// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include <string>
#include <unordered_map>

namespace facebook::fboss::platform::helpers {

/**
 * StructuredLogger provides utilities for logging structured events that can
 * be parsed by net_event_log.
 *
 * Output format:
 *   FBOSS_EVENT event_name <key1:value1> <key2:value2> ...
 *   FBOSS_ALERT event_name <key1:value1> <key2:value2> ...
 *
 * Tags are to be extracted as key-value pairs
 */
class StructuredLogger {
 public:
  explicit StructuredLogger(const std::string& serviceName);

  /**
   * Log an event at INFO level with FBOSS_EVENT marker.
   */
  void logEvent(
      const std::string& event,
      const std::unordered_map<std::string, std::string>& tags = {});

  /**
   * Log an alert at ERR level with FBOSS_ALERT marker.
   */
  void logAlert(
      const std::string& event,
      const std::string& errorMsg,
      const std::unordered_map<std::string, std::string>& tags = {});

  /**
   * Set persistent tags that will be included in all future logs.
   */
  void setTags(const std::unordered_map<std::string, std::string>& tags);

  /**
   * Add firmware version tags from the device name -> version map.
   * Each entry becomes a tag with key "firmware_version_{deviceName}".
   */
  void addFwTags(
      const std::unordered_map<std::string, std::string>& firmwareVersions);

 private:
  std::string formatTags(
      const std::string& event,
      const std::unordered_map<std::string, std::string>& tags) const;

  std::string serviceName_;
  std::string platformName_;
  std::unordered_map<std::string, std::string> persistentTags_;
};

} // namespace facebook::fboss::platform::helpers
