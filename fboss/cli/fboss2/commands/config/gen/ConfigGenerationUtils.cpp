/*
 *  Copyright (c) Meta Platforms, Inc. and affiliates.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/cli/fboss2/commands/config/gen/ConfigGenerationUtils.h"

#include <set>
#include <string_view>
#include <utility>

#include <folly/FileUtil.h>
#include <folly/String.h>
#include <thrift/lib/cpp/util/EnumUtils.h>

#include "fboss/agent/FbossError.h"
#include "fboss/cli/fboss2/commands/config/gen/PlatformConfigPathUtils.h"
#include "fboss/configs/platforms/generic/forwarding_stack/utils/ConfigGenerationManifestUtils.h"

namespace facebook::fboss::configgen {
namespace fs = std::filesystem;
namespace {

struct PendingConfig {
  fs::path outputPath;
  std::string contents;
};

std::string getPlatformName(PlatformType platform) {
  auto name = std::string(apache::thrift::util::enumNameSafe(platform));
  constexpr std::string_view kPlatformPrefix = "PLATFORM_";
  if (!name.starts_with(kPlatformPrefix)) {
    throw FbossError("Invalid PlatformType name: ", name);
  }
  name.erase(0, kPlatformPrefix.size());
  folly::toLowerAscii(name);
  return name;
}

std::string getProfileName(ConfigProfileType profile) {
  switch (profile) {
    case ConfigProfileType::DEFAULT:
      return "default";
    case ConfigProfileType::HW_TEST:
      return "hw_test";
  }
  throw FbossError(
      "Unsupported config generation profile: ",
      apache::thrift::util::enumNameSafe(profile));
}

std::string getServiceName(ServiceType service) {
  switch (service) {
    case ServiceType::AGENT:
      return "agent";
  }
  throw FbossError(
      "Unsupported config generation service: ",
      apache::thrift::util::enumNameSafe(service));
}

void validateVariant(const std::optional<std::string>& variant) {
  if (!variant) {
    return;
  }
  const fs::path component{*variant};
  if (component.empty() || component.has_parent_path() || component == "." ||
      component == "..") {
    throw FbossError("Invalid config generation variant: ", *variant);
  }
}

std::string getOutputFileName(
    std::string_view profile,
    const std::optional<std::string>& variant) {
  auto fileName = std::string(profile);
  if (variant) {
    fileName += "_" + *variant;
  }
  fileName += ".conf";
  return fileName;
}

} // namespace

std::vector<fs::path> generateConfigsFromManifest(
    const fs::path& fbossRoot,
    ServiceType service,
    const ConfigContentsGenerator& generateContents) {
  const auto manifest = loadConfigGenerationManifest(fbossRoot);
  const auto serviceName = getServiceName(service);
  std::vector<PendingConfig> pendingConfigs;
  std::set<fs::path> outputPaths;

  for (const auto& target : *manifest.targets()) {
    const auto optionsIt = target.serviceToOptions()->find(service);
    if (optionsIt == target.serviceToOptions()->end()) {
      continue;
    }

    const auto platformType = *target.platform();
    const auto platform = getPlatformName(platformType);
    const auto platformDirectory =
        findPlatformConfigDirectory(fbossRoot, platform).path;
    for (const auto& option : optionsIt->second) {
      const auto profileType = *option.profile();
      const auto profile = getProfileName(profileType);
      std::optional<std::string> variant;
      if (option.variant()) {
        variant = *option.variant();
      }
      validateVariant(variant);

      auto outputPath = platformDirectory / "forwarding_stack" / "generated" /
          serviceName / getOutputFileName(profile, variant);
      if (!outputPaths.insert(outputPath).second) {
        throw FbossError(
            "Config generation manifest contains duplicate output ",
            outputPath.string());
      }

      ConfigGenerationRequest request{
          .fbossRoot = fbossRoot,
          .platformType = platformType,
          .platform = platform,
          .profileType = profileType,
          .profile = profile,
          .variant = std::move(variant),
          .service = service,
          .serviceName = serviceName,
          .platformDirectory = platformDirectory,
          .outputPath = outputPath,
      };
      pendingConfigs.push_back(
          {.outputPath = std::move(outputPath),
           .contents = generateContents(request)});
    }
  }

  if (pendingConfigs.empty()) {
    throw FbossError(
        "Config generation manifest has no ", serviceName, " targets");
  }

  // Do not create or update outputs until every target has generated
  // successfully. This prevents a bad manifest target from leaving a partial
  // regeneration behind.
  for (const auto& pendingConfig : pendingConfigs) {
    fs::create_directories(pendingConfig.outputPath.parent_path());
  }
  for (const auto& pendingConfig : pendingConfigs) {
    folly::writeFileAtomic(
        pendingConfig.outputPath.c_str(), pendingConfig.contents, 0644);
  }

  std::vector<fs::path> generatedPaths;
  generatedPaths.reserve(pendingConfigs.size());
  for (auto& pendingConfig : pendingConfigs) {
    generatedPaths.push_back(std::move(pendingConfig.outputPath));
  }
  return generatedPaths;
}

} // namespace facebook::fboss::configgen
