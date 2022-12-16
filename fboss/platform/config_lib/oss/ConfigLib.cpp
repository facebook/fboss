// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/platform/config_lib/ConfigLib.h"

namespace facebook::fboss::platform::config_lib {

std::string getSensorServiceConfig(
    const std::optional<std::string>& /* platformName */) {
  throw std::runtime_error("Unimplemented function");
}

} // namespace facebook::fboss::platform::config_lib
