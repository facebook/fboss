// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.
#pragma once

#include <memory>
#include "fboss/platform/weutil/WeutilDarwin.h"

namespace facebook::fboss::platform {
std::unique_ptr<WeutilInterface> get_plat_weutil(
    const std::string& eeprom = "",
    const std::string& configFile = "");
PlainWeutilConfig parseConfig(const std::string& configFile = "");
} // namespace facebook::fboss::platform
