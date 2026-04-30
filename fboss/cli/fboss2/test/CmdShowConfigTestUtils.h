// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once
#ifndef IS_OSS
#include <string>

#include "neteng/fboss/coop/if/gen-cpp2/CoopService.h"
#include "neteng/fboss/coop/if/gen-cpp2/coop_constants.h"

namespace facebook::fboss::show_config_utils {
std::string getAgentRunningConfig();
std::string getBgpRunningConfig();

std::map<std::string, std::string> getConfigVersion();
std::string getConfigVersionOutput();

std::vector<::fboss::coop::CoopConfigPatcher> createCoopConfigPatchers();
folly::dynamic getCoopPatcher();
} // namespace facebook::fboss::show_config_utils
#endif // IS_OSS
