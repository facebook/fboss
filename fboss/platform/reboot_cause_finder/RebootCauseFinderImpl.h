// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include <optional>
#include <string>
#include <vector>

#include "fboss/platform/reboot_cause_finder/if/gen-cpp2/reboot_cause_config_types.h"

namespace facebook::fboss::platform::reboot_cause_finder {

// Exposed for unit tests; see tests/RebootCauseFinderImplTest.cpp.
namespace detail {

// Reads and decodes one hardware provider's sysfs file. Uses no member state,
// so it is a free function and can be tested directly. The returned attempt's
// status distinguishes a clean read that found nothing from a source that
// could not be read or could not be parsed.
reboot_cause_config::RebootCauseProviderAttempt readProvider(
    const reboot_cause_config::RebootCauseProviderConfig& config);

} // namespace detail

class RebootCauseFinderImpl {
 public:
  explicit RebootCauseFinderImpl(
      const reboot_cause_config::RebootCauseConfig& config);

  // Read all providers, determine the reboot cause, persist the record, and
  // optionally clear providers (when --clear_reboot_causes is set).
  // Invoked once when the one-shot binary runs.
  void determineRebootCause();

 private:
  const reboot_cause_config::RebootCauseConfig config_;

  void clearProvider(
      const reboot_cause_config::RebootCauseProviderConfig& config);

  // Persist the record as a pretty-printed JSON file under the history dir.
  // The filename carries the boot id, so a successful write is also what arms
  // the once-per-boot guard. Returns false if the record did not reach disk.
  [[nodiscard]] bool persistResult(
      const reboot_cause_config::RebootCauseRecord& record);
};

} // namespace facebook::fboss::platform::reboot_cause_finder
