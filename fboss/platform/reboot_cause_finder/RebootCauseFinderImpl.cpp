// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/platform/reboot_cause_finder/RebootCauseFinderImpl.h"

#include <algorithm>
#include <chrono>
#include <ctime>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

#include <fmt/format.h>
#include <folly/Conv.h>
#include <folly/FileUtil.h>
#include <folly/String.h>
#include <folly/dynamic.h>
#include <folly/json.h>
#include <folly/logging/xlog.h>
#include <gflags/gflags.h>
#include <thrift/lib/cpp/util/EnumUtils.h>
#include <thrift/lib/cpp2/protocol/Serializer.h>

DEFINE_bool(
    clear_reboot_causes,
    false,
    "Clear reboot causes from all providers after reading");

namespace facebook::fboss::platform::reboot_cause_finder {

namespace {

constexpr auto kHistoryDir = "/var/facebook/fboss/reboot_history";
constexpr auto kProcBootIdPath = "/proc/sys/kernel/random/boot_id";

// These three are properties of the kernel, systemd and kdump, identical on
// every platform, so they are constants rather than per-platform config.
constexpr auto kProcStatPath = "/proc/stat";

// The kernel regenerates this UUID on every boot, including kexec. It is not
// virtualized per namespace, and fboss platform services run as RootDirectory
// sandboxes rather than nspawn containers, so the value read here is the
// host's on both classic and NetOS Native.
// Empty when unreadable. The guard cannot be established without it, so the
// caller does nothing rather than risk a destructive clear.
std::string readBootId() {
  std::string contents;
  if (!folly::readFile(kProcBootIdPath, contents)) {
    return {};
  }
  return folly::trimWhitespace(contents).str();
}

// The boot id is the filename suffix of every record, so the existence of a
// matching file is itself the once-per-boot guard. Deliberately not a separate
// marker file: "the cause was recorded" and "this boot was already handled"
// must be the same fact, or the two can disagree and a boot's cause is lost.
// Matching on the suffix rather than picking the newest file also avoids
// trusting filenameStamp(), whose clock may not be NTP-synced this early.
std::string recordFilenameSuffix(const std::string& bootId) {
  return fmt::format("-{}.json", bootId);
}

// std::nullopt means the history dir could not be read, which is not the same
// as holding no record: the first says we cannot tell whether this boot was
// handled, the second says it was not. Collapsing them would let an unreadable
// dir look like a fresh boot and re-clear providers on every run.
std::optional<bool> recordExistsForBoot(const std::string& bootId) {
  std::error_code ec;
  std::filesystem::directory_iterator it(kHistoryDir, ec);
  if (ec == std::errc::no_such_file_or_directory) {
    return false;
  }
  if (ec) {
    XLOG(ERR) << fmt::format(
        "Failed to read history dir '{}': {}", kHistoryDir, ec.message());
    return std::nullopt;
  }

  // Stepped by hand rather than with a range-for: operator++ throws on error,
  // and a mid-scan failure must land in the same "cannot tell" branch as a
  // failure to open the dir, not propagate out of determineRebootCause().
  const std::filesystem::directory_iterator end;
  const auto suffix = recordFilenameSuffix(bootId);
  while (it != end) {
    if (it->path().filename().string().ends_with(suffix)) {
      return true;
    }
    it.increment(ec);
    if (ec) {
      XLOG(ERR) << fmt::format(
          "Failed while scanning history dir '{}': {}",
          kHistoryDir,
          ec.message());
      return std::nullopt;
    }
  }
  return false;
}

// Clearing a provider is destructive and unrecoverable, so it only runs once
// this boot's cause is safely on disk.
bool shouldClearProviders(bool recorded) {
  if (!FLAGS_clear_reboot_causes) {
    return false;
  }
  if (!recorded) {
    XLOG(ERR)
        << "Skipping provider clear because this boot's cause was not "
           "recorded; clearing now would discard it with nothing to show.";
    return false;
  }
  return true;
}

reboot_cause_config::RebootCauseProviderAttempt makeAttempt(
    const std::string& name,
    reboot_cause_config::RebootCauseProviderStatus status,
    const std::string& detail,
    std::vector<reboot_cause_config::RebootCause> causes = {}) {
  reboot_cause_config::RebootCauseProviderAttempt attempt;
  attempt.name() = name;
  attempt.status() = status;
  attempt.detail() = detail;
  attempt.causes() = std::move(causes);
  return attempt;
}

std::string summariseAttempts(
    const std::vector<reboot_cause_config::RebootCauseProviderAttempt>&
        attempts) {
  std::vector<std::string> parts;
  parts.reserve(attempts.size());
  for (const auto& a : attempts) {
    parts.push_back(
        fmt::format(
            "{}={}",
            *a.name(),
            apache::thrift::util::enumNameSafe(*a.status())));
  }
  return folly::join(", ", parts);
}

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

// Exposed for unit tests; see tests/RebootCauseFinderImplTest.cpp.
namespace detail {

std::optional<int64_t> readBootTimeSec(const std::string& procStatPath) {
  std::string contents;
  if (!folly::readFile(procStatPath.c_str(), contents)) {
    XLOG(ERR) << fmt::format("Failed to read '{}'", procStatPath);
    return std::nullopt;
  }

  std::vector<folly::StringPiece> lines;
  folly::split('\n', contents, lines);

  int64_t btime = 0;
  bool found = false;
  for (const auto& line : lines) {
    if (folly::StringPiece(line).startsWith("btime ") &&
        folly::tryTo<int64_t>(folly::trimWhitespace(line.subpiece(6)))
            .hasValue()) {
      btime = *folly::tryTo<int64_t>(folly::trimWhitespace(line.subpiece(6)));
      found = true;
      break;
    }
  }
  if (!found) {
    XLOG(ERR) << fmt::format("No usable btime line in '{}'", procStatPath);
    return std::nullopt;
  }

  // btime is wall clock minus uptime, so a badly skewed clock can put it in
  // the future. The window would then be nonsense.
  const auto now = static_cast<int64_t>(
      std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()));
  if (btime > now) {
    XLOG(ERR) << fmt::format(
        "btime {} is in the future (now {}); skipping log-derived causes",
        btime,
        now);
    return std::nullopt;
  }
  return btime;
}

reboot_cause_config::RebootCauseProviderAttempt readProvider(
    const reboot_cause_config::RebootCauseProviderConfig& providerConfig) {
  auto status = reboot_cause_config::RebootCauseProviderStatus::OK;
  std::vector<reboot_cause_config::RebootCause> causes;

  std::string contents;
  if (!folly::readFile(providerConfig.sysfsReadPath()->c_str(), contents)) {
    XLOG(ERR) << fmt::format(
        "Failed to read reboot causes from provider '{}' at path '{}'",
        *providerConfig.name(),
        *providerConfig.sysfsReadPath());
    status = reboot_cause_config::RebootCauseProviderStatus::READ_FAILED;
    return makeAttempt(
        *providerConfig.name(),
        status,
        *providerConfig.sysfsReadPath(),
        std::move(causes));
  }

  try {
    auto json = folly::parseJson(contents);
    for (const auto& causeJson : json["causes"]) {
      reboot_cause_config::RebootCause cause;
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
    status = reboot_cause_config::RebootCauseProviderStatus::PARSE_FAILED;
  }

  return makeAttempt(
      *providerConfig.name(),
      status,
      *providerConfig.sysfsReadPath(),
      std::move(causes));
}

} // namespace detail

RebootCauseFinderImpl::RebootCauseFinderImpl(
    const reboot_cause_config::RebootCauseConfig& config)
    : config_(config) {}

void RebootCauseFinderImpl::clearProvider(
    const reboot_cause_config::RebootCauseProviderConfig& providerConfig) {
  if (!folly::writeFile(
          std::string("1"), providerConfig.sysfsClearPath()->c_str())) {
    XLOG(ERR) << fmt::format(
        "Failed to clear reboot causes at '{}'",
        *providerConfig.sysfsClearPath());
  }
}

void RebootCauseFinderImpl::determineRebootCause() {
  // Anything that re-runs this binary within a boot (a manual invocation, a
  // platform_stack push, a restart of whatever unit owns it) would otherwise
  // record a reboot that never happened and, worse, clear the providers that
  // still hold the real boot's causes. Only the first run of a boot is
  // meaningful.
  const auto bootId = readBootId();
  if (bootId.empty()) {
    XLOG(ERR) << fmt::format(
        "Failed to read boot id from '{}'. Doing nothing: without it a fresh "
        "boot is indistinguishable from a re-run of this binary within the "
        "same boot, and clearing the providers would risk discarding a real "
        "boot's causes.",
        kProcBootIdPath);
    return;
  }
  const auto alreadyHandled = recordExistsForBoot(bootId);
  if (!alreadyHandled.has_value()) {
    XLOG(ERR) << "Doing nothing: cannot tell whether this boot was already "
                 "handled, so clearing the providers would risk discarding a "
                 "real boot's causes.";
    return;
  }
  if (*alreadyHandled) {
    XLOG(INFO) << fmt::format(
        "Reboot causes were already determined during boot {}. Nothing to do.",
        bootId);
    return;
  }

  XLOG(INFO) << "Reading reboot causes from all providers";

  // Read providers in priority order (lower value = higher priority).
  auto providerConfigs = *config_.rebootCauseProviderConfigs();
  std::sort(
      providerConfigs.begin(),
      providerConfigs.end(),
      [](const auto& a, const auto& b) {
        return *a.priority() < *b.priority();
      });

  // A panic names the failure and a manual reboot names the actor; the CPLD
  // reports only the mechanism (some generic warm-reset bit) for both. So
  // these outrank every hardware provider and are collected first. They are
  // not config providers: their paths and patterns come from systemd and
  // kdump and are the same on every platform.
  std::vector<reboot_cause_config::RebootCauseProviderAttempt> attempts;
  int64_t bootTimeMs = 0;
  std::optional<reboot_cause_config::DeterminedCause> determined;
  if (const auto btime = detail::readBootTimeSec(kProcStatPath)) {
    bootTimeMs = toEpochMs(static_cast<std::time_t>(*btime));
  }

  for (const auto& providerConfig : providerConfigs) {
    XLOG(INFO) << fmt::format("Reading provider '{}'", *providerConfig.name());
    auto attempt = detail::readProvider(providerConfig);
    // Providers are sorted by priority, so the first one to report anything
    // is the highest-priority hardware answer. It only applies when neither
    // implicit provider found a cause.
    if (!determined.has_value() && !attempt.causes()->empty()) {
      reboot_cause_config::DeterminedCause d;
      d.providerName() = *providerConfig.name();
      d.cause() = attempt.causes()->front();
      determined = d;
    }
    attempts.push_back(std::move(attempt));
  }

  reboot_cause_config::RebootCauseRecord record;
  auto now = std::chrono::system_clock::now();
  record.detectedAtMs() = std::chrono::duration_cast<std::chrono::milliseconds>(
                              now.time_since_epoch())
                              .count();
  record.detectedAtPacific() =
      pacificString(std::chrono::system_clock::to_time_t(now));
  // Left unset when nothing was found. A placeholder cause would have to
  // carry a placeholder timestamp, and that timestamp necessarily postdates
  // the boot it claims to explain.
  if (determined.has_value()) {
    record.determinedCause() = *determined;
  }
  record.bootId() = bootId;
  record.bootTimeMs() = bootTimeMs;
  record.providersAttempted() = attempts;

  // Writing the record is what arms the guard, so it has to succeed before
  // anything destructive happens. If it failed, the providers stay latched and
  // the next run retries this boot from scratch.
  const bool recorded = persistResult(record);

  if (shouldClearProviders(recorded)) {
    for (const auto& providerConfig : providerConfigs) {
      XLOG(INFO) << fmt::format(
          "Clearing provider '{}'", *providerConfig.name());
      clearProvider(providerConfig);
    }
  }

  if (record.determinedCause().has_value()) {
    XLOG(INFO) << fmt::format(
        "Determined reboot cause: '{}' [Provider: {}]",
        *record.determinedCause()->cause()->description(),
        *record.determinedCause()->providerName());
  } else {
    XLOG(INFO) << fmt::format(
        "No reboot cause determined. Providers attempted: {}",
        summariseAttempts(attempts));
  }
}

bool RebootCauseFinderImpl::persistResult(
    const reboot_cause_config::RebootCauseRecord& record) {
  std::error_code ec;
  std::filesystem::create_directories(kHistoryDir, ec);
  if (ec) {
    XLOG(ERR) << fmt::format(
        "Failed to create history dir '{}': {}", kHistoryDir, ec.message());
    return false;
  }

  // The boot id suffix is what recordExistsForBoot() matches on; the stamp is
  // only there to keep the directory readable.
  auto filePath = fmt::format(
      "{}/reboot-cause-{}{}",
      kHistoryDir,
      filenameStamp(
          std::chrono::system_clock::to_time_t(
              std::chrono::system_clock::now())),
      recordFilenameSuffix(*record.bootId()));

  auto json = folly::parseJson(
      apache::thrift::SimpleJSONSerializer::serialize<std::string>(record));
  if (!folly::writeFile(folly::toPrettyJson(json), filePath.c_str())) {
    XLOG(ERR) << fmt::format("Failed to write history file '{}'", filePath);
    return false;
  }
  return true;
}

} // namespace facebook::fboss::platform::reboot_cause_finder
