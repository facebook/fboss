/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/cli/fboss2/commands/config/gen/agent/AgentConfigGenUtils.h"

#include "fboss/cli/fboss2/utils/ConfigFileUtils.h"

namespace facebook::fboss::configgen {
namespace fs = std::filesystem;

constexpr std::string_view kAgentConfigFileName = "agent.conf";

cfg::AgentConfig assembleAgentConfig(
    std::map<std::string, std::string> defaultCommandLineArgs,
    cfg::SwitchConfig sw,
    cfg::PlatformConfig platform) {
  cfg::AgentConfig config;
  config.defaultCommandLineArgs() = std::move(defaultCommandLineArgs);
  config.sw() = std::move(sw);
  config.platform() = std::move(platform);
  return config;
}

fs::path generateAgentConfig(
    std::string_view /* platform */,
    std::string_view /* profile */,
    const std::optional<fs::path>& outputDirectory) {
  auto directory = utils::prepareOutputDirectory(outputDirectory);
  auto outputPath = directory / kAgentConfigFileName;
  auto config = assembleAgentConfig({}, {}, {});
  utils::writeFileWithoutOverwrite(
      outputPath, utils::serializeToPrettyJson(config) + "\n");
  return outputPath;
}

} // namespace facebook::fboss::configgen
