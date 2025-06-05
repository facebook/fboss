// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/platform/weutil/if/gen-cpp2/weutil_config_types.h"

namespace facebook::fboss::platform::weutil {

class ConfigValidator {
 public:
  bool isValid(
      const weutil_config::WeutilConfig& config,
      const std::string& platformName);
};

} // namespace facebook::fboss::platform::weutil
