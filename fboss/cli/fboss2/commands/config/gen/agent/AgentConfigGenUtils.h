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

#include <cstdint>
#include <filesystem>
#include <map>
#include <optional>
#include <string>
#include <string_view>

#include "fboss/agent/gen-cpp2/agent_config_types.h"

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

// Combines the ASIC configuration and deployment-specific port assignments
// into the platform section of an AgentConfig.
cfg::PlatformConfig generatePlatformConfig(
    cfg::ChipConfig chipConfig,
    std::map<int32_t, cfg::PortAssignment> portAssignments);

// Resolves the generated ASIC configuration selected by platform and profile.
// An empty profile or "default" selects the default variant. The file extension
// is derived from config_type in asic_config.json.
std::filesystem::path findGeneratedAsicConfig(
    const std::filesystem::path& fbossRoot,
    std::string_view platform,
    std::string_view profile);

// Resolves the generated port-assignment artifact for a platform. The
// colocated platform config layout is preferred, with the legacy centralized
// platform-mapping directory supported during migration.
std::filesystem::path findPortIdToPortAssignmentConfig(
    const std::filesystem::path& fbossRoot,
    std::string_view platform);

// Loads the selected ASIC configuration and port assignments into the platform
// section of an AgentConfig.
cfg::PlatformConfig generatePlatformConfigFromArtifacts(
    const std::filesystem::path& fbossRoot,
    std::string_view platform,
    std::string_view profile);

// Generates a new agent.conf and returns its path. The output is written to a
// unique temporary directory by default and never overwrites an existing file.
std::filesystem::path generateAgentConfig(
    std::string_view platform,
    std::string_view profile,
    const std::filesystem::path& fbossRoot,
    const std::optional<std::filesystem::path>& outputDirectory = std::nullopt);

} // namespace facebook::fboss::configgen
