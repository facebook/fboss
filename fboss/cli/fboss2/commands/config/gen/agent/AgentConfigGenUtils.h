/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#pragma once

#include "fboss/agent/gen-cpp2/agent_config_types.h"

#include <filesystem>
#include <map>
#include <optional>
#include <string>
#include <string_view>

namespace facebook::fboss::configgen {

/*
 * Agent config generation is kept separate from the fboss2 command adapter so
 * other binaries can reuse it. The design follows AgentConfig's top-level
 * ownership boundaries: platform, sw, and defaultCommandLineArgs. Generation
 * logic for each field should be added only as that capability is supported,
 * rather than introducing speculative configuration models up front.
 */

// Combines independently constructed AgentConfig sections. Fields not listed
// here retain their Thrift defaults.
cfg::AgentConfig assembleAgentConfig(
    std::map<std::string, std::string> defaultCommandLineArgs,
    cfg::SwitchConfig sw,
    cfg::PlatformConfig platform);

// Generates a new agent.conf without modifying an existing service config.
// Platform and profile are selection inputs for field-specific generation;
// they remain accepted even when the current builders do not consume them.
std::filesystem::path generateAgentConfig(
    std::string_view platform,
    std::string_view profile,
    const std::optional<std::filesystem::path>& outputDirectory = std::nullopt);

} // namespace facebook::fboss::configgen
