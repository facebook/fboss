// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include <folly/Range.h>
#include <optional>
#include <string>
#include <vector>

#include "fboss/platform/reboot_cause_finder/if/gen-cpp2/reboot_cause_config_types.h"

namespace facebook::fboss::platform::reboot_cause_finder {

// Causes derived from files the kernel, systemd and kdump write, rather than
// from a per-platform hardware provider. Paths are parameters so tests can
// point them at a temp dir; production call sites pass the real constants.
namespace detail {

// Reads and decodes one hardware provider's sysfs file. Uses no member state,
// so it is a free function and can be tested directly. The returned attempt's
// status distinguishes a clean read that found nothing from a source that
// could not be read or could not be parsed.
reboot_cause_config::RebootCauseProviderAttempt readProvider(
    const reboot_cause_config::RebootCauseProviderConfig& config);

// Boot start in epoch seconds. std::nullopt when it cannot be established or
// is in the future, which disables both readers below.
std::optional<int64_t> readBootTimeSec(const std::string& procStatPath);

// Causes inside [btimeSec - windowSec, btimeSec). At most one each; the match
// nearest btimeSec wins. Empty when the source is absent.
// The returned attempt's status is READ_FAILED when a source is present but
// unreadable, and OK when it is simply absent -- most switches have never
// panicked and no image uses every log path.
reboot_cause_config::RebootCauseProviderAttempt readKernelPanic(
    const std::vector<std::string>& crashDirs,
    int64_t btimeSec,
    int64_t windowSec);

// Directories searched for a crash dump; a dump can be in either.
const std::vector<std::string>& kernelPanicCrashDirs();

reboot_cause_config::RebootCauseProviderAttempt readManualReboot(
    const std::vector<std::string>& logPaths,
    int64_t btimeSec,
    int64_t windowSec);

// std::nullopt when the name is not a timestamp this code understands.
std::optional<std::time_t> parseCrashDirName(const std::string& name);

// Resolves a syslog stamp's missing year against btime. std::nullopt when no
// plausible year puts the line at or before btime.
std::optional<std::time_t> parseSyslogTimestamp(
    const std::string& line,
    int64_t btimeSec);

// Log files searched for the systemd-logind reboot line, in no
// particular order.
const std::vector<std::string>& manualRebootLogPaths();

// Whether a syslog line is systemd-logind announcing a reboot. False for a
// line that merely contains the phrase, such as sshd's verbatim record of a
// remote command that grepped for it.
bool isManualRebootLine(folly::StringPiece line);

// The cause nearest to boot start across every attempt, paired with the
// provider that reported it. std::nullopt when no attempt reported anything.
std::optional<reboot_cause_config::DeterminedCause> selectNearestToBoot(
    const std::vector<reboot_cause_config::RebootCauseProviderAttempt>&
        attempts);

} // namespace detail

class RebootCauseFinderImpl {
 public:
  // Everything the finder reads or writes outside the config. Defaults are
  // the real locations; tests point them at a temp dir so the whole of
  // determineRebootCause() can be exercised without touching the host.
  struct Paths {
    std::string historyDir;
    std::string procStat;
    std::string bootId;
    std::vector<std::string> crashDirs;
    std::vector<std::string> logPaths;
  };
  static Paths defaultPaths();

  explicit RebootCauseFinderImpl(
      const reboot_cause_config::RebootCauseConfig& config,
      Paths paths = defaultPaths());

  // Read all providers, determine the reboot cause, persist the record, and
  // optionally clear providers (when --clear_reboot_causes is set).
  // Invoked once when the one-shot binary runs.
  void determineRebootCause();

 private:
  const reboot_cause_config::RebootCauseConfig config_;
  const Paths paths_;

  void clearProvider(
      const reboot_cause_config::RebootCauseProviderConfig& config);

  // Persist the record as a pretty-printed JSON file under the history dir.
  // The filename carries the boot id, so a successful write is also what arms
  // the once-per-boot guard. Returns false if the record did not reach disk.
  [[nodiscard]] bool persistResult(
      const reboot_cause_config::RebootCauseRecord& record);
};

} // namespace facebook::fboss::platform::reboot_cause_finder
