/*
 *  Copyright (c) Meta Platforms, Inc. and affiliates.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/configs/platforms/generic/forwarding_stack/utils/ConfigGenerationManifestUtils.h"

#include <algorithm>
#include <optional>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include <folly/FileUtil.h>
#include <folly/String.h>
#include <folly/json/json.h>
#include <thrift/lib/cpp/util/EnumUtils.h>

#include "fboss/agent/FbossError.h"
#include "fboss/configs/platforms/generic/forwarding_stack/utils/ThriftConfigUtils.h"

namespace facebook::fboss::configgen {
namespace fs = std::filesystem;
namespace {

constexpr std::string_view kConfigGenerationFile = "config_generation.json";

template <typename Enum>
int64_t parseEnumName(const folly::dynamic& value, std::string_view field) {
  if (!value.isString()) {
    throw FbossError(field, " must use a Thrift enum name");
  }
  return thriftEnumToInt(parseThriftEnumName<Enum>(value.asString(), field));
}

// SimpleJSON encodes enums as integers. Keep checked-in declarations readable
// by accepting only enum names at their boundary, then convert those names
// before deserializing into the strongly typed Thrift model.
std::string normalizeTargetEnumNames(std::string_view contents) {
  auto target = folly::parseJson(contents);
  if (!target.isObject()) {
    return folly::toJson(target);
  }
  if (const auto platformIt = target.find("platform");
      platformIt != target.items().end()) {
    platformIt->second =
        parseEnumName<PlatformType>(platformIt->second, "platform");
  }
  const auto servicesIt = target.find("serviceToOptions");
  if (servicesIt == target.items().end() || !servicesIt->second.isObject()) {
    return folly::toJson(target);
  }

  folly::dynamic normalizedServices = folly::dynamic::object();
  for (const auto& [serviceName, rawOptions] : servicesIt->second.items()) {
    const auto service = parseEnumName<ServiceType>(serviceName, "service");
    auto options = rawOptions;
    if (options.isArray()) {
      for (auto& option : options) {
        if (!option.isObject()) {
          continue;
        }
        if (const auto profileIt = option.find("profile");
            profileIt != option.items().end()) {
          profileIt->second =
              parseEnumName<ConfigProfileType>(profileIt->second, "profile");
        }
      }
    }
    normalizedServices[std::to_string(service)] = std::move(options);
  }
  servicesIt->second = std::move(normalizedServices);
  return folly::toJson(target);
}

void validateTarget(const ConfigGenerationTarget& target) {
  const auto platform = *target.platform();
  validateThriftEnum(platform, "Target platform");
  if (target.serviceToOptions()->empty()) {
    throw FbossError(
        "Config generation target ",
        apache::thrift::util::enumNameSafe(platform),
        " has no service options");
  }

  for (const auto& [service, options] : *target.serviceToOptions()) {
    validateThriftEnum(service, "Target service");
    if (options.empty()) {
      throw FbossError(
          "Config generation target ",
          apache::thrift::util::enumNameSafe(platform),
          " has no options for service ",
          apache::thrift::util::enumNameSafe(service));
    }

    std::set<std::pair<ConfigProfileType, std::optional<std::string>>>
        uniqueOptions;
    for (const auto& option : options) {
      const auto profile = *option.profile();
      validateThriftEnum(profile, "Config profile");
      const auto variant = option.variant().to_optional();
      if (variant && variant->empty()) {
        throw FbossError("Config option variant must not be empty");
      }
      const auto asicConfigVariant = option.asicConfigVariant().to_optional();
      if (asicConfigVariant && asicConfigVariant->empty()) {
        throw FbossError("Config option asicConfigVariant must not be empty");
      }
      if (!uniqueOptions.emplace(profile, variant).second) {
        throw FbossError(
            "Duplicate config option for ",
            apache::thrift::util::enumNameSafe(platform),
            ", ",
            apache::thrift::util::enumNameSafe(service),
            ", and ",
            apache::thrift::util::enumNameSafe(profile));
      }
    }
  }
}

void validateTargetOwner(
    const ConfigGenerationTarget& target,
    const fs::path& platformDirectory) {
  const auto platformName = getPlatformName(*target.platform());
  if (platformDirectory.filename() != platformName) {
    throw FbossError(
        "Config generation target ",
        apache::thrift::util::enumNameSafe(*target.platform()),
        " does not match platform directory ",
        platformDirectory.string());
  }
}

} // namespace

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

ConfigGenerationTarget parseConfigGenerationTarget(std::string_view contents) {
  auto target = parseStrictSimpleJson<ConfigGenerationTarget>(
      normalizeTargetEnumNames(contents));
  validateTarget(target);
  return target;
}

ConfigGenerationTarget loadConfigGenerationTarget(
    const fs::path& platformDirectory) {
  const auto path =
      platformDirectory / "forwarding_stack" / kConfigGenerationFile;
  std::string contents;
  if (!folly::readFile(path.c_str(), contents)) {
    throw FbossError(
        "Unable to read config generation declaration from ", path.string());
  }

  try {
    auto target = parseConfigGenerationTarget(contents);
    validateTargetOwner(target, platformDirectory);
    return target;
  } catch (const std::exception& error) {
    throw FbossError(
        "Unable to parse config generation declaration from ",
        path.string(),
        ": ",
        error.what());
  }
}

ConfigGenerationManifest loadConfigGenerationManifest(
    const fs::path& fbossRoot) {
  const auto platformsDirectory = fbossRoot / "configs" / "platforms";
  if (!fs::is_directory(platformsDirectory)) {
    throw FbossError(
        "Platform config directory does not exist: ",
        platformsDirectory.string());
  }

  std::vector<fs::path> platformDirectories;
  for (const auto& vendorEntry : fs::directory_iterator(platformsDirectory)) {
    if (!vendorEntry.is_directory() ||
        vendorEntry.path().filename() == "generic") {
      continue;
    }
    for (const auto& platformEntry :
         fs::directory_iterator(vendorEntry.path())) {
      if (platformEntry.is_directory() &&
          fs::is_regular_file(
              platformEntry.path() / "forwarding_stack" /
              kConfigGenerationFile)) {
        platformDirectories.push_back(platformEntry.path());
      }
    }
  }
  std::sort(platformDirectories.begin(), platformDirectories.end());

  ConfigGenerationManifest manifest;
  std::set<PlatformType> platforms;
  for (const auto& platformDirectory : platformDirectories) {
    auto target = loadConfigGenerationTarget(platformDirectory);
    if (!platforms.insert(*target.platform()).second) {
      throw FbossError(
          "Duplicate config generation target for ",
          apache::thrift::util::enumNameSafe(*target.platform()));
    }
    manifest.targets()->push_back(std::move(target));
  }
  if (manifest.targets()->empty()) {
    throw FbossError("No platform config generation declarations found");
  }
  return manifest;
}

} // namespace facebook::fboss::configgen
