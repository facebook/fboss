// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/platform/reboot_cause_service/if/gen-cpp2/reboot_cause_config_types.h"

namespace facebook::fboss::platform::reboot_cause_service {

class ConfigValidator {
 public:
  bool isValid(const reboot_cause_config::RebootCauseConfig& config);
};

} // namespace facebook::fboss::platform::reboot_cause_service
