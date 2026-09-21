/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/cli/fboss2/commands/config/gen/agent/CmdConfigGenAgent.h"

#include <filesystem>
#include <iostream>
#include <optional>

#include "fboss/cli/fboss2/CmdHandler.cpp"
#include "fboss/cli/fboss2/commands/config/gen/agent/AgentConfigGenUtils.h"

namespace facebook::fboss {

CmdConfigGenAgent::RetType CmdConfigGenAgent::queryClient(
    const HostInfo& /* hostInfo */) {
  auto options = CmdLocalOptions::getInstance();
  auto platform = options->getLocalOption(
      std::string(kConfigGenAgentCommand), kConfigGenAgentPlatform);
  auto profile = options->getLocalOption(
      std::string(kConfigGenAgentCommand), kConfigGenAgentProfile);
  auto fbossRoot = options->getLocalOption(
      std::string(kConfigGenAgentCommand), kConfigGenAgentFbossRoot);
  auto outputDirectory = options->getLocalOption(
      std::string(kConfigGenAgentCommand), kConfigGenAgentOutputDirectory);

  std::optional<std::filesystem::path> outputDirectoryPath;
  if (!outputDirectory.empty()) {
    outputDirectoryPath = outputDirectory;
  }
  return configgen::generateAgentConfig(
             platform, profile, fbossRoot, outputDirectoryPath)
      .string();
}

void CmdConfigGenAgent::printOutput(const RetType& outputPath) {
  std::cout << "Generated Agent configuration at " << outputPath << std::endl;
}

template void CmdHandler<CmdConfigGenAgent, CmdConfigGenAgentTraits>::run();

} // namespace facebook::fboss
