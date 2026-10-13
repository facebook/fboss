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

struct ConfigGenerationPlan {
  std::string serviceName;
  std::vector<PendingConfig> configs;
  std::set<fs::path> outputPaths;
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

ConfigGenerationPlan createConfigGenerationPlan(
    const fs::path& fbossRoot,
    ServiceType service,
    const ConfigContentsGenerator& generateContents) {
  const auto manifest = loadConfigGenerationManifest(fbossRoot);
  ConfigGenerationPlan plan{.serviceName = getServiceName(service)};

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
          plan.serviceName / getOutputFileName(profile, variant);
      if (!plan.outputPaths.insert(outputPath).second) {
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
          .serviceName = plan.serviceName,
          .platformDirectory = platformDirectory,
          .outputPath = outputPath,
      };
      plan.configs.push_back(
          {.outputPath = std::move(outputPath),
           .contents = generateContents(request)});
    }
  }

  if (plan.configs.empty()) {
    throw FbossError(
        "Config generation manifest has no ", plan.serviceName, " targets");
  }
  return plan;
}

std::set<fs::path> findGeneratedConfigPaths(
    const fs::path& fbossRoot,
    std::string_view serviceName) {
  const auto platformsDirectory = fbossRoot / "configs" / "platforms";
  if (!fs::is_directory(platformsDirectory)) {
    throw FbossError(
        "Platform config directory does not exist: ",
        platformsDirectory.string());
  }

  std::set<fs::path> paths;
  for (const auto& vendorEntry : fs::directory_iterator(platformsDirectory)) {
    if (!vendorEntry.is_directory() ||
        vendorEntry.path().filename() == "generic") {
      continue;
    }
    for (const auto& platformEntry :
         fs::directory_iterator(vendorEntry.path())) {
      if (!platformEntry.is_directory()) {
        continue;
      }
      const auto serviceDirectory =
          platformEntry.path() / "forwarding_stack" / "generated" / serviceName;
      if (!fs::is_directory(serviceDirectory)) {
        continue;
      }
      for (const auto& outputEntry : fs::directory_iterator(serviceDirectory)) {
        if (!outputEntry.is_regular_file()) {
          throw FbossError(
              "Unexpected entry in generated config directory: ",
              outputEntry.path().string());
        }
        paths.insert(outputEntry.path());
      }
    }
  }
  return paths;
}

std::string readGeneratedConfig(const fs::path& path) {
  std::string contents;
  if (!folly::readFile(path.c_str(), contents)) {
    throw FbossError("Unable to read generated config ", path.string());
  }
  return contents;
}

} // namespace

std::vector<fs::path> generateConfigsFromManifest(
    const fs::path& fbossRoot,
    ServiceType service,
    const ConfigContentsGenerator& generateContents) {
  auto plan = createConfigGenerationPlan(fbossRoot, service, generateContents);
  const auto existingPaths =
      findGeneratedConfigPaths(fbossRoot, plan.serviceName);

  // Do not create or update outputs until every target has generated
  // successfully. This prevents a bad manifest target from leaving a partial
  // regeneration behind.
  for (const auto& pendingConfig : plan.configs) {
    fs::create_directories(pendingConfig.outputPath.parent_path());
  }
  for (const auto& pendingConfig : plan.configs) {
    folly::writeFileAtomic(
        pendingConfig.outputPath.c_str(), pendingConfig.contents, 0644);
  }
  for (const auto& existingPath : existingPaths) {
    if (!plan.outputPaths.contains(existingPath)) {
      fs::remove(existingPath);
    }
  }

  std::vector<fs::path> generatedPaths;
  generatedPaths.reserve(plan.configs.size());
  for (auto& pendingConfig : plan.configs) {
    generatedPaths.push_back(std::move(pendingConfig.outputPath));
  }
  return generatedPaths;
}

ConfigVerificationResult verifyConfigsFromManifest(
    const fs::path& fbossRoot,
    ServiceType service,
    const ConfigContentsGenerator& generateContents) {
  const auto plan =
      createConfigGenerationPlan(fbossRoot, service, generateContents);
  const auto existingPaths =
      findGeneratedConfigPaths(fbossRoot, plan.serviceName);
  ConfigVerificationResult result;

  for (const auto& expected : plan.configs) {
    if (!existingPaths.contains(expected.outputPath)) {
      result.missing.push_back(expected.outputPath);
    } else if (readGeneratedConfig(expected.outputPath) != expected.contents) {
      result.stale.push_back(expected.outputPath);
    }
  }
  for (const auto& existingPath : existingPaths) {
    if (!plan.outputPaths.contains(existingPath)) {
      result.obsolete.push_back(existingPath);
    }
  }
  return result;
}

} // namespace facebook::fboss::configgen
