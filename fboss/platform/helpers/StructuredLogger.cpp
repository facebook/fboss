// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/platform/helpers/StructuredLogger.h"

#include <fmt/format.h>
#include <folly/logging/xlog.h>

#include "fboss/platform/helpers/PlatformNameLib.h"

namespace facebook::fboss::platform::helpers {

namespace {
// Escape special characters in tag values to prevent regex issues
std::string escapeTagValue(const std::string& value) {
  std::string escaped;
  escaped.reserve(value.size());
  for (char c : value) {
    // Replace characters that could break the <key:value> format
    if (c == '<' || c == '>' || c == ':') {
      escaped += '_';
    } else if (c == '\n' || c == '\r') {
      escaped += ' ';
    } else {
      escaped += c;
    }
  }
  return escaped;
}
} // namespace

StructuredLogger::StructuredLogger(const std::string& serviceName)
    : serviceName_(serviceName),
      platformName_(PlatformNameLib().getPlatformName().value_or("")) {}

std::string StructuredLogger::formatTags(
    const std::string& event,
    const std::unordered_map<std::string, std::string>& tags) const {
  std::string result;

  result += fmt::format("<event:{}>", escapeTagValue(event));
  result += fmt::format(" <service:{}>", escapeTagValue(serviceName_));
  if (!platformName_.empty()) {
    result += fmt::format(" <platform:{}>", escapeTagValue(platformName_));
  }

  // Add persistent tags
  for (const auto& [key, value] : persistentTags_) {
    result +=
        fmt::format(" <{}:{}>", escapeTagValue(key), escapeTagValue(value));
  }

  // Add provided tags
  for (const auto& [key, value] : tags) {
    result +=
        fmt::format(" <{}:{}>", escapeTagValue(key), escapeTagValue(value));
  }

  return result;
}

void StructuredLogger::logEvent(
    const std::string& event,
    const std::unordered_map<std::string, std::string>& tags) {
  std::string formattedTags = formatTags(event, tags);
  XLOG(INFO) << "FBOSS_EVENT " << formattedTags;
}

void StructuredLogger::logAlert(
    const std::string& event,
    const std::string& errorMsg,
    const std::unordered_map<std::string, std::string>& tags) {
  std::string formattedTags = formatTags(event, tags);
  XLOG(ERR) << "FBOSS_ALERT " << escapeTagValue(errorMsg) << " "
            << formattedTags;
}

void StructuredLogger::setTags(
    const std::unordered_map<std::string, std::string>& tags) {
  for (const auto& [key, value] : tags) {
    persistentTags_[key] = value;
  }
}

void StructuredLogger::addFwTags(
    const std::unordered_map<std::string, std::string>& firmwareVersions) {
  for (const auto& [deviceName, firmwareVersion] : firmwareVersions) {
    persistentTags_[fmt::format("firmware_version_{}", deviceName)] =
        firmwareVersion;
  }
}

} // namespace facebook::fboss::platform::helpers
