// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/platform/reboot_cause_service/RebootCauseServiceImpl.h"

#include <chrono>
#include <filesystem>
#include <iomanip>
#include <sstream>

#include <fmt/format.h>
#include <folly/FileUtil.h>
#include <folly/dynamic.h>
#include <folly/json.h>
#include <folly/logging/xlog.h>
#include <thrift/lib/cpp2/protocol/Serializer.h>

#include "fboss/platform/helpers/StructuredLogger.h"
#include "fboss/platform/reboot_cause_service/Flags.h"

namespace facebook::fboss::platform::reboot_cause_service {

constexpr auto kHistoryDir = "/var/facebook/fboss/reboot_cause_service/history";
constexpr int16_t kLogFileVersion = 1;

namespace {

std::string nowAsString() {
  auto now = std::chrono::system_clock::now();
  auto t = std::chrono::system_clock::to_time_t(now);
  std::ostringstream oss;
  oss << std::put_time(std::localtime(&t), "%m-%d-%Y %H:%M:%S");
  return oss.str();
}

std::string nowAsFilename() {
  auto now = std::chrono::system_clock::now();
  auto t = std::chrono::system_clock::to_time_t(now);
  std::ostringstream oss;
  oss << std::put_time(std::localtime(&t), "%Y-%m-%d-%H-%M-%S");
  return oss.str();
}

// Sort providers by priority ascending (lower value = higher priority).
// Generic providers (priority 0) always sort first.
bool byPriority(
    const reboot_cause_service::RebootCauseProvider& a,
    const reboot_cause_service::RebootCauseProvider& b) {
  return *a.config()->priority() < *b.config()->priority();
}

} // namespace

RebootCauseServiceImpl::RebootCauseServiceImpl(
    const reboot_cause_config::RebootCauseConfig& config)
    : config_(config) {}

reboot_cause_service::RebootCauseProvider RebootCauseServiceImpl::readProvider(
    const std::string& name,
    const reboot_cause_config::RebootCauseProviderConfig& providerConfig) {
  reboot_cause_service::RebootCauseProvider provider;
  provider.name() = name;
  provider.config() = providerConfig;
  provider.causes() = {};

  std::string contents;
  if (!folly::readFile(providerConfig.sysfsReadPath()->c_str(), contents)) {
    helpers::StructuredLogger("reboot_cause_service")
        .logAlert(
            "provider_read_error",
            fmt::format(
                "Failed to read reboot causes from provider '{}' at path '{}'",
                name,
                *providerConfig.sysfsReadPath()));
    return provider;
  }

  try {
    auto json = folly::parseJson(contents);
    for (const auto& causeJson : json["causes"]) {
      reboot_cause_service::RebootCauseData cause;
      cause.description() = causeJson["description"].asString();
      cause.date() = causeJson["date"].asString();
      if (causeJson.count("externalProvider")) {
        cause.externalProvider() = causeJson["externalProvider"].asString();
      }
      provider.causes()->push_back(std::move(cause));
    }
  } catch (const std::exception& ex) {
    helpers::StructuredLogger("reboot_cause_service")
        .logAlert(
            "provider_parse_error",
            fmt::format(
                "Failed to parse reboot causes from provider '{}': {}",
                name,
                ex.what()));
  }

  return provider;
}

void RebootCauseServiceImpl::clearProvider(
    const reboot_cause_config::RebootCauseProviderConfig& providerConfig) {
  if (!folly::writeFile(
          std::string("1"), providerConfig.sysfsClearPath()->c_str())) {
    XLOG(ERR) << fmt::format(
        "Failed to clear reboot causes at '{}'",
        *providerConfig.sysfsClearPath());
  }
}

reboot_cause_service::RebootCauseResult RebootCauseServiceImpl::investigate(
    const std::vector<reboot_cause_service::RebootCauseProvider>& providers) {
  reboot_cause_service::RebootCauseResult result;
  result.date() = nowAsString();
  result.providers() = providers;

  for (const auto& provider : providers) {
    if (provider.causes()->empty()) {
      continue;
    }

    // Chronologically first cause from this provider is the determined cause.
    auto determinedCause = provider.causes()->front();

    // Follow externalProvider pointer if set.
    if (determinedCause.externalProvider().has_value()) {
      const auto extName = *determinedCause.externalProvider();
      bool found = false;
      for (const auto& extProvider : providers) {
        if (*extProvider.name() == extName && !extProvider.causes()->empty()) {
          determinedCause = extProvider.causes()->front();
          result.determinedCauseProvider() = extName;
          found = true;
          break;
        }
      }
      if (!found) {
        XLOG(WARN) << fmt::format(
            "externalProvider '{}' not found or has no causes, "
            "falling back to provider '{}'",
            extName,
            *provider.name());
        result.determinedCauseProvider() = *provider.name();
      }
    } else {
      result.determinedCauseProvider() = *provider.name();
    }

    result.determinedCause() = determinedCause;
    return result;
  }

  // No cause found — report unknown.
  reboot_cause_service::RebootCauseData unknown;
  unknown.description() = "Unknown";
  unknown.date() = nowAsString();
  result.determinedCause() = unknown;
  result.determinedCauseProvider() = "None";
  return result;
}

void RebootCauseServiceImpl::determineRebootCause() {
  XLOG(INFO) << "Reading reboot causes from all providers";

  std::vector<reboot_cause_service::RebootCauseProvider> providers;
  for (const auto& [name, providerConfig] :
       *config_.rebootCauseProviderConfigs()) {
    XLOG(INFO) << fmt::format("Reading provider '{}'", name);
    providers.push_back(readProvider(name, providerConfig));
  }

  std::sort(providers.begin(), providers.end(), byPriority);

  auto result = investigate(providers);
  persistResult(result);

  if (FLAGS_clear_reboot_causes) {
    for (const auto& [name, providerConfig] :
         *config_.rebootCauseProviderConfigs()) {
      XLOG(INFO) << fmt::format("Clearing provider '{}'", name);
      clearProvider(providerConfig);
    }
  }
  XLOG(INFO) << fmt::format(
      "Determined reboot cause: '{}' [Provider: {}]",
      *result.determinedCause()->description(),
      *result.determinedCauseProvider());
}

void RebootCauseServiceImpl::persistResult(
    reboot_cause_service::RebootCauseResult& result) {
  std::error_code ec;
  std::filesystem::create_directories(kHistoryDir, ec);
  if (ec) {
    XLOG(ERR) << fmt::format(
        "Failed to create history dir '{}': {}", kHistoryDir, ec.message());
    return;
  }

  auto filePath =
      fmt::format("{}/reboot-cause-{}.json", kHistoryDir, nowAsFilename());

  reboot_cause_service::RebootCauseResultLogFile logFile;
  logFile.filePath() = filePath;
  logFile.fileVersion() = kLogFileVersion;
  result.logFile() = logFile;

  // Inject a top-level "version" key so readers can check the schema version
  // before deserializing the rest of the file.
  auto obj = folly::parseJson(
      apache::thrift::SimpleJSONSerializer::serialize<std::string>(result));
  obj["version"] = static_cast<int64_t>(kLogFileVersion);

  if (!folly::writeFile(folly::toJson(obj), filePath.c_str())) {
    XLOG(ERR) << fmt::format("Failed to write history file '{}'", filePath);
  }
}

namespace {

// Deserialize a RebootCauseResult from a versioned history file.
// The file has a top-level "version" key alongside the result fields.
// SimpleJSONSerializer ignores the unknown "version" key when deserializing.
reboot_cause_service::RebootCauseResult deserializeHistoryFile(
    const std::string& path,
    const std::string& contents) {
  auto obj = folly::parseJson(contents);
  if (obj.count("version") && obj["version"].asInt() != kLogFileVersion) {
    XLOG(WARN) << fmt::format(
        "Unexpected file version {} in '{}', expected {}",
        obj["version"].asInt(),
        path,
        kLogFileVersion);
  }
  return apache::thrift::SimpleJSONSerializer::deserialize<
      reboot_cause_service::RebootCauseResult>(contents);
}

std::vector<std::filesystem::path> listHistoryFiles() {
  std::error_code ec;
  std::vector<std::filesystem::path> files;
  for (const auto& entry :
       std::filesystem::directory_iterator(kHistoryDir, ec)) {
    if (entry.path().extension() == ".json") {
      files.push_back(entry.path());
    }
  }
  if (ec) {
    XLOG(ERR) << fmt::format(
        "Failed to list history dir '{}': {}", kHistoryDir, ec.message());
    return files;
  }
  std::sort(files.begin(), files.end(), std::greater<>());
  return files;
}

} // namespace

reboot_cause_service::RebootCauseResult
RebootCauseServiceImpl::getLastRebootCause() const {
  auto files = listHistoryFiles();
  if (files.empty()) {
    throw std::runtime_error("No reboot cause history found.");
  }

  std::string contents;
  if (!folly::readFile(files.front().string().c_str(), contents)) {
    throw std::runtime_error(
        fmt::format("Failed to read '{}'", files.front().string()));
  }

  return deserializeHistoryFile(files.front().string(), contents);
}

std::vector<reboot_cause_service::RebootCauseResult>
RebootCauseServiceImpl::getRebootCauseHistory() const {
  auto files = listHistoryFiles();
  if (files.empty()) {
    return {};
  }

  std::vector<reboot_cause_service::RebootCauseResult> history;
  for (const auto& file : files) {
    std::string contents;
    if (!folly::readFile(file.string().c_str(), contents)) {
      XLOG(ERR) << fmt::format(
          "Failed to read history file '{}'", file.string());
      continue;
    }
    try {
      history.push_back(deserializeHistoryFile(file.string(), contents));
    } catch (const std::exception& ex) {
      XLOG(ERR) << fmt::format(
          "Failed to parse history file '{}': {}", file.string(), ex.what());
    }
  }

  return history;
}

} // namespace facebook::fboss::platform::reboot_cause_service
