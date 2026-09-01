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

#include <optional>
#include <string>
#include <utility>

#include <folly/FileUtil.h>
#include <folly/json/json.h>
#include <thrift/lib/cpp/util/EnumUtils.h>

#include "fboss/agent/FbossError.h"
#include "fboss/cli/fboss2/commands/config/gen/PlatformConfigPathUtils.h"
#include "fboss/cli/fboss2/utils/ConfigFileUtils.h"
#include "fboss/lib/platforms/PlatformMappingUtils.h"

namespace facebook::fboss::configgen {
namespace fs = std::filesystem;
namespace {

constexpr std::string_view kAgentConfigFileName = "agent.conf";
constexpr std::string_view kDefaultProfileName = "default";
constexpr std::string_view kPortAssignmentFileName =
    "port_id_to_port_assignment.json";

struct GeneratedAsicConfigFile {
  fs::path path;
  cfg::AsicConfigType configType;
};

std::string readFile(const fs::path& path) {
  std::string contents;
  if (!folly::readFile(path.c_str(), contents)) {
    throw FbossError("Unable to read config-generation input ", path.string());
  }
  return contents;
}

std::optional<std::string> getConfigTypeName(
    const folly::dynamic& variant,
    const fs::path& metadataPath) {
  if (!variant.isObject()) {
    throw FbossError(
        "ASIC config variant in ", metadataPath.string(), " is not an object");
  }
  if (!variant.count("asic_config_params")) {
    return std::nullopt;
  }

  const auto& params = variant["asic_config_params"];
  if (!params.isObject()) {
    throw FbossError(
        "asic_config_params in ", metadataPath.string(), " is not an object");
  }
  if (!params.count("config_type")) {
    return std::nullopt;
  }
  if (!params["config_type"].isString()) {
    throw FbossError(
        "config_type in ", metadataPath.string(), " is not a string");
  }
  return params["config_type"].asString();
}

std::string_view getGeneratedFileExtension(cfg::AsicConfigType configType) {
  switch (configType) {
    case cfg::AsicConfigType::KEY_VALUE_CONFIG:
    case cfg::AsicConfigType::JSON_CONFIG:
      return ".json";
    case cfg::AsicConfigType::YAML_CONFIG:
      return ".yml";
    case cfg::AsicConfigType::NONE:
      throw FbossError("ASIC config type NONE does not have a generated file");
  }
  throw FbossError("Unknown ASIC config type");
}

GeneratedAsicConfigFile resolveGeneratedAsicConfig(
    const fs::path& fbossRoot,
    std::string_view platform,
    std::string_view profile) {
  const auto platformDirectory =
      findPlatformConfigDirectory(fbossRoot, platform);
  const auto asicConfigDirectory =
      findPlatformConfigComponentDirectory(platformDirectory, "asic_config");
  const auto metadataPath = asicConfigDirectory / "asic_config.json";
  if (!fs::is_regular_file(metadataPath)) {
    throw FbossError(
        "ASIC config metadata does not exist: ", metadataPath.string());
  }

  const auto metadata = folly::parseJson(readFile(metadataPath));
  const auto platformName = std::string(platform);
  const auto profileName =
      profile == kDefaultProfileName ? std::string{} : std::string(profile);
  if (!metadata.isObject() || !metadata.count("platform_name") ||
      !metadata["platform_name"].isString() ||
      metadata["platform_name"].asString() != platformName) {
    throw FbossError(
        "ASIC config metadata ",
        metadataPath.string(),
        " does not describe platform '",
        platform,
        "'");
  }
  if (!metadata.count("variants") || !metadata["variants"].isObject() ||
      !metadata["variants"].count(profileName)) {
    throw FbossError(
        "ASIC config profile '",
        profile,
        "' does not exist for platform '",
        platform,
        "'");
  }

  std::optional<std::string> configTypeName;
  if (metadata.count("defaults")) {
    configTypeName = getConfigTypeName(metadata["defaults"], metadataPath);
  }
  if (const auto variantConfigType =
          getConfigTypeName(metadata["variants"][profileName], metadataPath)) {
    configTypeName = variantConfigType;
  }
  if (!configTypeName) {
    throw FbossError(
        "ASIC config metadata ",
        metadataPath.string(),
        " does not define config_type for profile '",
        profile,
        "'");
  }
  cfg::AsicConfigType configType;
  if (!apache::thrift::util::tryParseEnum(*configTypeName, &configType)) {
    throw FbossError(
        "Unsupported ASIC config type '",
        *configTypeName,
        "' in ",
        metadataPath.string());
  }

  auto fileName = platformName;
  if (!profileName.empty()) {
    fileName += "_" + profileName;
  }
  fileName += getGeneratedFileExtension(configType);

  const auto generatedPath = asicConfigDirectory / "generated" / fileName;
  if (!fs::is_regular_file(generatedPath)) {
    throw FbossError(
        "Generated ASIC config does not exist: ", generatedPath.string());
  }
  return {.path = generatedPath, .configType = configType};
}

std::map<std::string, std::string> loadKeyValueConfig(
    const std::string& contents,
    const fs::path& path) {
  const auto json = folly::parseJson(contents);
  if (!json.isObject()) {
    throw FbossError(
        "Key-value ASIC config is not a JSON object: ", path.string());
  }

  std::map<std::string, std::string> config;
  for (const auto& [key, value] : json.items()) {
    if (!key.isString() || !value.isString()) {
      throw FbossError(
          "Key-value ASIC config must contain only string values: ",
          path.string());
    }
    config.emplace(key.asString(), value.asString());
  }
  return config;
}

cfg::ChipConfig loadAsicConfig(const GeneratedAsicConfigFile& generatedFile) {
  const auto contents = readFile(generatedFile.path);
  cfg::AsicConfigEntry common;
  switch (generatedFile.configType) {
    case cfg::AsicConfigType::KEY_VALUE_CONFIG:
      common.set_config(loadKeyValueConfig(contents, generatedFile.path));
      break;
    case cfg::AsicConfigType::JSON_CONFIG:
      folly::parseJson(contents);
      common.set_jsonConfig(contents);
      break;
    case cfg::AsicConfigType::YAML_CONFIG:
      common.set_yamlConfig(contents);
      break;
    case cfg::AsicConfigType::NONE:
      throw FbossError("ASIC config type NONE cannot be loaded");
  }

  cfg::AsicConfig asicConfig;
  asicConfig.common() = std::move(common);

  cfg::ChipConfig chipConfig;
  chipConfig.set_asicConfig(std::move(asicConfig));
  return chipConfig;
}

} // namespace

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

cfg::PlatformConfig generatePlatformConfig(
    cfg::ChipConfig chipConfig,
    std::map<int32_t, cfg::PortAssignment> portAssignments) {
  cfg::PlatformConfig platformConfig;
  platformConfig.chip() = std::move(chipConfig);
  platformConfig.portIdToPortAssignment() = std::move(portAssignments);
  return platformConfig;
}

fs::path findGeneratedAsicConfig(
    const fs::path& fbossRoot,
    std::string_view platform,
    std::string_view profile) {
  return resolveGeneratedAsicConfig(fbossRoot, platform, profile).path;
}

fs::path findPortIdToPortAssignmentConfig(
    const fs::path& fbossRoot,
    std::string_view platform) {
  const auto platformDirectory =
      findPlatformConfigDirectory(fbossRoot, platform);
  const auto colocatedPath = platformDirectory.path / "platform_mapping" /
      "generated" / kPortAssignmentFileName;
  if (fs::is_regular_file(colocatedPath)) {
    return colocatedPath;
  }

  const auto legacyPath = fbossRoot / "lib" / "platform_mapping_v2" /
      "generated_platform_mappings" / platformDirectory.systemVendor /
      std::string(platform) / kPortAssignmentFileName;
  if (fs::is_regular_file(legacyPath)) {
    return legacyPath;
  }

  throw FbossError(
      "Generated port assignments do not exist for platform '",
      platform,
      "'; checked ",
      colocatedPath.string(),
      " and ",
      legacyPath.string());
}

cfg::PlatformConfig generatePlatformConfigFromArtifacts(
    const fs::path& fbossRoot,
    std::string_view platform,
    std::string_view profile) {
  return generatePlatformConfig(
      loadAsicConfig(resolveGeneratedAsicConfig(fbossRoot, platform, profile)),
      readPortIdToPortAssignment(
          findPortIdToPortAssignmentConfig(fbossRoot, platform).string()));
}

fs::path generateAgentConfig(
    std::string_view platform,
    std::string_view profile,
    const fs::path& fbossRoot,
    const std::optional<fs::path>& outputDirectory) {
  auto config = assembleAgentConfig(
      {},
      {},
      generatePlatformConfigFromArtifacts(fbossRoot, platform, profile));
  auto directory = utils::prepareOutputDirectory(outputDirectory);
  auto outputPath = directory / kAgentConfigFileName;
  utils::writeFileWithoutOverwrite(
      outputPath, utils::serializeToPrettyJson(config) + "\n");
  return outputPath;
}

} // namespace facebook::fboss::configgen
