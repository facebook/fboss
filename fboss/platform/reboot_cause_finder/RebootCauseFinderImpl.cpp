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

// Measured on minipack3n fboss332669634.ash7: a BMC power cycle at 19:51:57
// reached btime at 20:17:51, a gap of 1554s, nearly all of it POST plus a PXE
// attempt timing out before falling back to local disk. 1800 would have left
// four minutes of margin. Too large only risks admitting an older event, which
// nearest-to-btime selection already handles; too small silently misses the
// cause.
DEFINE_int32(
    max_downtime_sec,
    3600,
    "How far before boot start to look for a kernel panic or a manual reboot. "
    "Must cover shutdown, POST and bootloader, not the power-off duration: a "
    "long outage leaves no log line at all and is reported by the hardware "
    "providers instead.");

namespace facebook::fboss::platform::reboot_cause_finder {

namespace {

constexpr auto kHistoryDir = "/var/facebook/fboss/reboot_history";
constexpr auto kProcBootIdPath = "/proc/sys/kernel/random/boot_id";

// These three are properties of the kernel, systemd and kdump, identical on
// every platform, so they are constants rather than per-platform config.
constexpr auto kProcStatPath = "/proc/stat";
// Scanning /var/crash turns up the processed subdir itself. It is scanned in
// its own right, so it is not a dump name that failed to parse.
constexpr auto kProcessedDirName = "processed";

constexpr auto kKernelPanicProvider = "KernelPanic";

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

reboot_cause_config::RebootCause makeCause(
    const std::string& description,
    std::time_t occurredAt) {
  reboot_cause_config::RebootCause cause;
  cause.description() = description;
  cause.occurredAtMs() = toEpochMs(occurredAt);
  cause.occurredAtPacific() = pacificString(occurredAt);
  return cause;
}

// Only events inside [btime - window, btime) can have caused this boot.
// Anything at or after btime belongs to the boot we are running in, and
// anything older belongs to an earlier boot.
bool inBootWindow(std::time_t when, int64_t btimeSec, int64_t windowSec) {
  const auto t = static_cast<int64_t>(when);
  return t < btimeSec && t >= btimeSec - windowSec;
}

// Crash dirs are named "%Y-%m-%dT%H:%M:%S%Z" or "%Y-%m-%d-%H:%M:%S".
// Any %Z is ignored; see below.
std::optional<std::time_t> parseCrashDirName(const std::string& name) {
  std::tm tm{};
  if (strptime(name.c_str(), "%Y-%m-%dT%H:%M:%S", &tm) == nullptr) {
    tm = {};
    if (strptime(name.c_str(), "%Y-%m-%d-%H:%M:%S", &tm) == nullptr) {
      return std::nullopt;
    }
  }

  // The producer wrote this name with localtime() on this host, so mktime()
  // on the same host is the exact inverse -- the zone text carries no
  // information mktime does not already have. tm{} zero-inits tm_isdst to 0,
  // which would assert "not daylight", so it must be set to -1 explicitly.
  //
  // The one case this cannot resolve is the hour repeated at the DST
  // fall-back, where mktime picks one of the two candidates.
  tm.tm_isdst = -1;

  const auto when = std::mktime(&tm);
  if (when == -1) {
    return std::nullopt;
  }
  return when;
}

// the dump is written by the crash kernel or by post-boot processing, so it
// can fall after btime even though the panic preceded it.
// A dump can be in either directory, so both are searched.
const std::vector<std::string>& kernelPanicCrashDirs() {
  static const std::vector<std::string> kDirs = {
      "/var/crash",
      "/var/crash/processed",
  };
  return kDirs;
}

// Folds the nearest-to-btime match in one directory into best/bestName.
// Returns false only when the directory is present but could not be read; an
// absent directory is the normal case on a switch that has never panicked and
// must not be reported as a failure.
bool scanCrashDir(
    const std::string& crashDir,
    int64_t btimeSec,
    int64_t windowSec,
    std::optional<std::time_t>& best,
    std::string& bestName) {
  std::error_code ec;
  std::filesystem::directory_iterator it(crashDir, ec);
  if (ec == std::errc::no_such_file_or_directory) {
    return true;
  }
  if (ec) {
    XLOG(ERR) << fmt::format(
        "Failed to open crash dir '{}': {}", crashDir, ec.message());
    return false;
  }

  const std::filesystem::directory_iterator end;
  while (it != end) {
    const auto name = it->path().filename().string();
    const auto when =
        name == kProcessedDirName ? std::nullopt : parseCrashDirName(name);
    if (when) {
      if (inBootWindow(*when, btimeSec, windowSec) &&
          (!best || *when > *best)) {
        best = *when;
        bestName = name;
      }
    } else if (name != kProcessedDirName) {
      // A name we cannot read is a panic we cannot report, so say so rather
      // than skipping in silence.
      XLOG(ERR) << fmt::format(
          "Unrecognised crash dir name '{}' in '{}'", name, crashDir);
    }
    it.increment(ec);
    if (ec) {
      XLOG(ERR) << fmt::format(
          "Failed while scanning crash dir '{}': {}", crashDir, ec.message());
      return false;
    }
  }
  return true;
}

reboot_cause_config::RebootCauseProviderAttempt readKernelPanic(
    const std::vector<std::string>& crashDirs,
    int64_t btimeSec,
    int64_t windowSec) {
  auto status = reboot_cause_config::RebootCauseProviderStatus::OK;
  std::optional<std::time_t> best;
  std::string bestName;
  for (const auto& crashDir : crashDirs) {
    // Keep scanning the remaining dirs: a cause found elsewhere is still
    // worth reporting, but the attempt is no longer a clean read.
    if (!scanCrashDir(crashDir, btimeSec, windowSec, best, bestName)) {
      status = reboot_cause_config::RebootCauseProviderStatus::READ_FAILED;
    }
  }

  std::vector<reboot_cause_config::RebootCause> causes;
  if (best) {
    // The description is a stable name so consumers can group on it; the
    // crash dir varies per instance and belongs in rawValue.
    auto cause = makeCause("Kernel Panic", *best);
    cause.rawValue() = bestName;
    causes.push_back(std::move(cause));
  }
  return makeAttempt(
      kKernelPanicProvider,
      status,
      folly::join(", ", crashDirs),
      std::move(causes));
}

// Nearest to boot start wins, across every cause in the given attempts.
// Returns the provider alongside the cause, because a cause on its own does
// not say which provider found it.
std::optional<reboot_cause_config::DeterminedCause> selectNearestToBoot(
    const std::vector<reboot_cause_config::RebootCauseProviderAttempt>&
        attempts) {
  std::optional<reboot_cause_config::DeterminedCause> best;
  for (const auto& attempt : attempts) {
    for (const auto& cause : *attempt.causes()) {
      if (!best || *cause.occurredAtMs() > *best->cause()->occurredAtMs()) {
        reboot_cause_config::DeterminedCause determined;
        determined.providerName() = *attempt.name();
        determined.cause() = cause;
        best = determined;
      }
    }
  }
  return best;
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
  const auto btime = detail::readBootTimeSec(kProcStatPath);
  if (btime) {
    bootTimeMs = toEpochMs(static_cast<std::time_t>(*btime));
    const int64_t window = FLAGS_max_downtime_sec;

    std::vector<reboot_cause_config::RebootCauseProviderAttempt> software;
    software.push_back(
        detail::readKernelPanic(
            detail::kernelPanicCrashDirs(), *btime, window));

    determined = detail::selectNearestToBoot(software);
    attempts.insert(attempts.end(), software.begin(), software.end());
  } else {
    // Without boot start there is no window, so the provider cannot be
    // evaluated. Say so rather than letting its absence read as "looked and
    // found nothing".
    attempts.push_back(makeAttempt(
        kKernelPanicProvider,
        reboot_cause_config::RebootCauseProviderStatus::SKIPPED,
        "boot time unavailable"));
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
