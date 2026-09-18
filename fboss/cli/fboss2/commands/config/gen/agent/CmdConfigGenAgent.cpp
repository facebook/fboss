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
#include <string>

#include <folly/FileUtil.h>

#include "fboss/agent/FbossError.h"
#include "fboss/cli/fboss2/CmdHandler.cpp"
#include "fboss/cli/fboss2/commands/config/gen/agent/AgentConfigComparisonUtils.h"
#include "fboss/cli/fboss2/commands/config/gen/agent/AgentConfigGenUtils.h"

namespace facebook::fboss {
namespace {

std::string readFile(const std::filesystem::path& path) {
  std::string contents;
  if (!folly::readFile(path.c_str(), contents)) {
    throw FbossError("Unable to read reference Agent config ", path.string());
  }
  return contents;
}

std::string formatComparisonResult(
    const std::filesystem::path& generatedPath,
    const std::filesystem::path& referencePath,
    const configgen::AgentConfigComparisonResult& comparison) {
  std::string output =
      "Generated Agent configuration at " + generatedPath.string() +
      "\nReference: " + referencePath.string() + "\nResult: " +
      std::string(
          configgen::getAgentConfigComparisonStatusName(comparison.status));
  if (!comparison.ignoredPaths.empty()) {
    output += "\nIgnored paths:";
    for (const auto& path : comparison.ignoredPaths) {
      output += "\n  " + path;
    }
  }
  if (!comparison.semanticDifferences.empty()) {
    output += "\nSemantic differences:";
    for (const auto& difference : comparison.semanticDifferences) {
      output += "\n  " + difference;
    }
  }
  if (!comparison.differingPaths.empty()) {
    output += "\nDiffering paths:";
    for (const auto& path : comparison.differingPaths) {
      output += "\n  " + path;
    }
  }
  return output;
}

} // namespace

CmdConfigGenAgent::RetType CmdConfigGenAgent::queryClient(
    const HostInfo& /* hostInfo */) {
  auto options = CmdLocalOptions::getInstance();
  auto platform = options->getLocalOption(
      std::string(kConfigGenAgentCommand), kConfigGenAgentPlatform);
  auto profile = options->getLocalOption(
      std::string(kConfigGenAgentCommand), kConfigGenAgentProfile);
  auto fbossRoot = options->getLocalOption(
      std::string(kConfigGenAgentCommand), kConfigGenAgentFbossRoot);
  auto asicConfigFile = options->getLocalOption(
      std::string(kConfigGenAgentCommand), kConfigGenAgentAsicConfigFile);
  auto asicConfigType = options->getLocalOption(
      std::string(kConfigGenAgentCommand), kConfigGenAgentAsicConfigType);
  auto referenceConfigFile = options->getLocalOption(
      std::string(kConfigGenAgentCommand), kConfigGenAgentReferenceConfigFile);
  auto outputDirectory = options->getLocalOption(
      std::string(kConfigGenAgentCommand), kConfigGenAgentOutputDirectory);

  std::optional<std::filesystem::path> asicConfigFilePath;
  if (!asicConfigFile.empty()) {
    asicConfigFilePath = asicConfigFile;
  }
  std::optional<cfg::AsicConfigType> parsedAsicConfigType;
  if (!asicConfigType.empty()) {
    parsedAsicConfigType = configgen::parseAsicConfigType(asicConfigType);
  }
  std::optional<std::filesystem::path> outputDirectoryPath;
  if (!outputDirectory.empty()) {
    outputDirectoryPath = outputDirectory;
  }
  const auto outputPath = configgen::generateAgentConfig(
      platform,
      profile,
      fbossRoot,
      outputDirectoryPath,
      asicConfigFilePath,
      parsedAsicConfigType);
  if (referenceConfigFile.empty()) {
    return "Generated Agent configuration at " + outputPath.string();
  }

  const auto comparison = configgen::compareAgentConfigContents(
      readFile(outputPath), readFile(referenceConfigFile));
  const auto result =
      formatComparisonResult(outputPath, referenceConfigFile, comparison);
  if (comparison.status ==
      configgen::AgentConfigComparisonStatus::CONTENT_MISMATCH) {
    throw FbossError(result);
  }
  return result;
}

void CmdConfigGenAgent::printOutput(const RetType& output) {
  std::cout << output << std::endl;
}

template void CmdHandler<CmdConfigGenAgent, CmdConfigGenAgentTraits>::run();

} // namespace facebook::fboss
