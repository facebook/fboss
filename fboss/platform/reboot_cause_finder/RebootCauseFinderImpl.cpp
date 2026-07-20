// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/platform/reboot_cause_finder/RebootCauseFinderImpl.h"

#include <algorithm>
#include <chrono>
#include <ctime>
#include <filesystem>
#include <optional>
#include <string>

#include <fmt/format.h>
#include <folly/FileUtil.h>
#include <folly/dynamic.h>
#include <folly/json.h>
#include <folly/logging/xlog.h>
#include <gflags/gflags.h>
#include <thrift/lib/cpp2/protocol/Serializer.h>

DEFINE_bool(
    clear_reboot_causes,
    false,
    "Clear reboot causes from all providers after reading");

namespace facebook::fboss::platform::reboot_cause_finder {

namespace {

constexpr auto kHistoryDir = "/var/facebook/fboss/reboot_history";

int64_t toEpochMs(std::time_t t) {
  return static_cast<int64_t>(t) * 1000;
}

// Render an instant in the host's local (Pacific, fleet-wide) time with the
// zone abbreviation, e.g. "2026-07-02 23:16:55 PDT".
std::string pacificString(std::time_t t) {
  std::tm tm{};
  localtime_r(&t, &tm);
  char buf[64];
  std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S %Z", &tm);
  return buf;
}

// Sortable local-time stamp used in the history filename.
std::string filenameStamp(std::time_t t) {
  std::tm tm{};
  localtime_r(&t, &tm);
  char buf[32];
  std::strftime(buf, sizeof(buf), "%Y-%m-%d-%H-%M-%S", &tm);
  return buf;
}

// Parse a provider's local-time date string ("MM-DD-YYYY HH:MM:SS[.mmm]") into
// an epoch time_t; std::nullopt if it doesn't match.
std::optional<std::time_t> parseProviderDate(const std::string& s) {
  std::tm tm{};
  if (strptime(s.c_str(), "%m-%d-%Y %H:%M:%S", &tm) == nullptr) {
    return std::nullopt;
  }
  tm.tm_isdst = -1; // let mktime resolve PST vs PDT
  return std::mktime(&tm);
}

} // namespace

RebootCauseFinderImpl::RebootCauseFinderImpl(
    const reboot_cause_config::RebootCauseConfig& config)
    : config_(config) {}

std::vector<reboot_cause_config::RebootCause>
RebootCauseFinderImpl::readProvider(
    const reboot_cause_config::RebootCauseProviderConfig& providerConfig) {
  std::vector<reboot_cause_config::RebootCause> causes;

  std::string contents;
  if (!folly::readFile(providerConfig.sysfsReadPath()->c_str(), contents)) {
    XLOG(ERR) << fmt::format(
        "Failed to read reboot causes from provider '{}' at path '{}'",
        *providerConfig.name(),
        *providerConfig.sysfsReadPath());
    return causes;
  }

  try {
    auto json = folly::parseJson(contents);
    for (const auto& causeJson : json["causes"]) {
      reboot_cause_config::RebootCause cause;
      cause.providerName() = *providerConfig.name();
      cause.description() = causeJson["description"].asString();

      auto dateStr = causeJson["date"].asString();
      if (auto t = parseProviderDate(dateStr)) {
        cause.occurredAtMs() = toEpochMs(*t);
        cause.occurredAtPacific() = pacificString(*t);
      } else {
        cause.occurredAtMs() = 0;
        cause.occurredAtPacific() = dateStr;
      }

      if (causeJson.count("rawValue")) {
        cause.rawValue() = causeJson["rawValue"].asString();
      }
      causes.push_back(std::move(cause));
    }
  } catch (const std::exception& ex) {
    XLOG(ERR) << fmt::format(
        "Failed to parse reboot causes from provider '{}': {}",
        *providerConfig.name(),
        ex.what());
  }

  return causes;
}

void RebootCauseFinderImpl::clearProvider(
    const reboot_cause_config::RebootCauseProviderConfig& providerConfig) {
  if (!folly::writeFile(
          std::string("1"), providerConfig.sysfsClearPath()->c_str())) {
    XLOG(ERR) << fmt::format(
        "Failed to clear reboot causes at '{}'",
        *providerConfig.sysfsClearPath());
  }
}

reboot_cause_config::RebootCause RebootCauseFinderImpl::determineCause(
    const std::optional<reboot_cause_config::RebootCause>& primaryCause) {
  if (primaryCause.has_value()) {
    return *primaryCause;
  }

  // No provider reported a cause.
  auto now =
      std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
  reboot_cause_config::RebootCause unknown;
  unknown.providerName() = "None";
  unknown.description() = "Unknown";
  unknown.occurredAtMs() = toEpochMs(now);
  unknown.occurredAtPacific() = pacificString(now);
  return unknown;
}

void RebootCauseFinderImpl::determineRebootCause() {
  XLOG(INFO) << "Reading reboot causes from all providers";

  // Read providers in priority order (lower value = higher priority).
  auto providerConfigs = *config_.rebootCauseProviderConfigs();
  std::sort(
      providerConfigs.begin(),
      providerConfigs.end(),
      [](const auto& a, const auto& b) {
        return *a.priority() < *b.priority();
      });

  std::vector<reboot_cause_config::RebootCause> allCauses;
  std::optional<reboot_cause_config::RebootCause> primaryCause;
  for (const auto& providerConfig : providerConfigs) {
    XLOG(INFO) << fmt::format("Reading provider '{}'", *providerConfig.name());
    auto causes = readProvider(providerConfig);
    // Providers are read in priority order, so the first provider that reports
    // a cause is the highest-priority one; its first cause is the candidate.
    if (!primaryCause.has_value() && !causes.empty()) {
      primaryCause = causes.front();
    }
    allCauses.insert(
        allCauses.end(),
        std::make_move_iterator(causes.begin()),
        std::make_move_iterator(causes.end()));
  }

  reboot_cause_config::RebootCauseRecord record;
  auto now = std::chrono::system_clock::now();
  record.detectedAtMs() = std::chrono::duration_cast<std::chrono::milliseconds>(
                              now.time_since_epoch())
                              .count();
  record.detectedAtPacific() =
      pacificString(std::chrono::system_clock::to_time_t(now));
  record.determinedCause() = determineCause(primaryCause);
  record.allCauses() = allCauses;

  persistResult(record);

  if (FLAGS_clear_reboot_causes) {
    for (const auto& providerConfig : providerConfigs) {
      XLOG(INFO) << fmt::format(
          "Clearing provider '{}'", *providerConfig.name());
      clearProvider(providerConfig);
    }
  }

  XLOG(INFO) << fmt::format(
      "Determined reboot cause: '{}' [Provider: {}]",
      *record.determinedCause()->description(),
      *record.determinedCause()->providerName());
}

void RebootCauseFinderImpl::persistResult(
    const reboot_cause_config::RebootCauseRecord& record) {
  std::error_code ec;
  std::filesystem::create_directories(kHistoryDir, ec);
  if (ec) {
    XLOG(ERR) << fmt::format(
        "Failed to create history dir '{}': {}", kHistoryDir, ec.message());
    return;
  }

  auto filePath = fmt::format(
      "{}/reboot-cause-{}.json",
      kHistoryDir,
      filenameStamp(
          std::chrono::system_clock::to_time_t(
              std::chrono::system_clock::now())));

  auto json = folly::parseJson(
      apache::thrift::SimpleJSONSerializer::serialize<std::string>(record));
  if (!folly::writeFile(folly::toPrettyJson(json), filePath.c_str())) {
    XLOG(ERR) << fmt::format("Failed to write history file '{}'", filePath);
  }
}

} // namespace facebook::fboss::platform::reboot_cause_finder
