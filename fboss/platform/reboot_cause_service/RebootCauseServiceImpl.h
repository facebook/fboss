// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include <optional>

#include "fboss/platform/reboot_cause_service/if/gen-cpp2/reboot_cause_config_types.h"
#include "fboss/platform/reboot_cause_service/if/gen-cpp2/reboot_cause_service_types.h"

namespace facebook::fboss::platform::reboot_cause_service {

class RebootCauseServiceImpl {
 public:
  explicit RebootCauseServiceImpl(
      const reboot_cause_config::RebootCauseConfig& config);

  // Read all providers, run investigation, optionally clear providers.
  // Called at startup when --determine_reboot_cause is set.
  void determineRebootCause();

  // Return the result of the last investigation.
  reboot_cause_service::RebootCauseResult getLastRebootCause() const;

 private:
  reboot_cause_config::RebootCauseConfig config_;
  std::optional<reboot_cause_service::RebootCauseResult> lastResult_;

  reboot_cause_service::RebootCauseProvider readProvider(
      const std::string& name,
      const reboot_cause_config::RebootCauseProviderConfig& config);

  void clearProvider(
      const reboot_cause_config::RebootCauseProviderConfig& config);

  reboot_cause_service::RebootCauseResult investigate(
      const std::vector<reboot_cause_service::RebootCauseProvider>& providers);
};

} // namespace facebook::fboss::platform::reboot_cause_service
