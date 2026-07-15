// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include <optional>
#include <vector>

#include "fboss/platform/reboot_cause_finder/if/gen-cpp2/reboot_cause_config_types.h"

namespace facebook::fboss::platform::reboot_cause_finder {

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

  std::vector<reboot_cause_config::RebootCause> readProvider(
      const reboot_cause_config::RebootCauseProviderConfig& config);

  void clearProvider(
      const reboot_cause_config::RebootCauseProviderConfig& config);

  // Return the determined cause: the highest-priority provider's candidate
  // cause, or an "Unknown" cause when no provider reported anything.
  reboot_cause_config::RebootCause determineCause(
      const std::optional<reboot_cause_config::RebootCause>& primaryCause);

  // Persist the record as a pretty-printed JSON file under the history dir.
  void persistResult(const reboot_cause_config::RebootCauseRecord& record);
};

} // namespace facebook::fboss::platform::reboot_cause_finder
