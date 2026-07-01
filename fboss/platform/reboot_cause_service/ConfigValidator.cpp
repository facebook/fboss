// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/platform/reboot_cause_service/ConfigValidator.h"

#include <fmt/format.h>
#include <folly/logging/xlog.h>

#include <unordered_set>

namespace facebook::fboss::platform::reboot_cause_service {
using namespace reboot_cause_config;

bool ConfigValidator::isValid(const RebootCauseConfig& config) {
  XLOG(INFO) << "Validating reboot_cause_service config";

  std::unordered_set<std::string> seenNames;
  std::unordered_set<int16_t> seenPriorities;

  for (const auto& [name, providerConfig] :
       *config.rebootCauseProviderConfigs()) {
    if (name.empty()) {
      XLOG(ERR) << "RebootCauseProviderConfig name must be non-empty";
      return false;
    }
    if (seenNames.contains(name)) {
      XLOG(ERR) << fmt::format("Provider name '{}' is a duplicate", name);
      return false;
    }
    seenNames.insert(name);

    auto priority = *providerConfig.priority();
    if (seenPriorities.contains(priority)) {
      XLOG(ERR) << fmt::format("Provider priority {} is a duplicate", priority);
      return false;
    }
    seenPriorities.insert(priority);

    if (providerConfig.sysfsReadPath()->empty()) {
      XLOG(ERR) << fmt::format(
          "Provider '{}' sysfsReadPath must be non-empty", name);
      return false;
    }
    if (providerConfig.sysfsClearPath()->empty()) {
      XLOG(ERR) << fmt::format(
          "Provider '{}' sysfsClearPath must be non-empty", name);
      return false;
    }
  }

  return true;
}

} // namespace facebook::fboss::platform::reboot_cause_service
