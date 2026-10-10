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

#include <optional>
#include <set>
#include <string>
#include <utility>

#include <folly/FileUtil.h>
#include <folly/json/json.h>
#include <thrift/lib/cpp/util/EnumUtils.h>

#include "fboss/agent/FbossError.h"
#include "fboss/configs/platforms/generic/forwarding_stack/utils/ThriftConfigUtils.h"

namespace facebook::fboss::configgen {
namespace fs = std::filesystem;
namespace {

constexpr std::string_view kManifestPath =
    "configs/platforms/generic/forwarding_stack/"
    "config_generation_manifest.json";

template <typename Enum>
int64_t parseEnumName(const folly::dynamic& value, std::string_view field) {
  if (!value.isString()) {
    throw FbossError(field, " must use a Thrift enum name");
  }
  return thriftEnumToInt(parseThriftEnumName<Enum>(value.asString(), field));
}

// SimpleJSON encodes enums as integers. Keep the checked-in manifest readable
// by accepting only enum names at its boundary, then convert those names before
// deserializing into the strongly typed Thrift model.
std::string normalizeManifestEnumNames(std::string_view contents) {
  auto manifest = folly::parseJson(contents);
  if (!manifest.isObject()) {
    return folly::toJson(manifest);
  }
  const auto targetsIt = manifest.find("targets");
  if (targetsIt == manifest.items().end() || !targetsIt->second.isArray()) {
    return folly::toJson(manifest);
  }

  for (auto& target : targetsIt->second) {
    if (!target.isObject()) {
      continue;
    }
    if (const auto platformIt = target.find("platform");
        platformIt != target.items().end()) {
      platformIt->second =
          parseEnumName<PlatformType>(platformIt->second, "platform");
    }
    const auto servicesIt = target.find("serviceToOptions");
    if (servicesIt == target.items().end() || !servicesIt->second.isObject()) {
      continue;
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
  }
  return folly::toJson(manifest);
}

void validateManifest(const ConfigGenerationManifest& manifest) {
  if (manifest.targets()->empty()) {
    throw FbossError("Config generation manifest has no targets");
  }

  std::set<PlatformType> platforms;
  for (const auto& target : *manifest.targets()) {
    const auto platform = *target.platform();
    validateThriftEnum(platform, "Target platform");
    if (!platforms.insert(platform).second) {
      throw FbossError(
          "Duplicate config generation target for ",
          apache::thrift::util::enumNameSafe(platform));
    }
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
}

} // namespace

ConfigGenerationManifest parseConfigGenerationManifest(
    std::string_view contents) {
  auto manifest = parseStrictSimpleJson<ConfigGenerationManifest>(
      normalizeManifestEnumNames(contents));
  validateManifest(manifest);
  return manifest;
}

ConfigGenerationManifest loadConfigGenerationManifest(
    const fs::path& fbossRoot) {
  const auto path = fbossRoot / kManifestPath;
  std::string contents;
  if (!folly::readFile(path.c_str(), contents)) {
    throw FbossError(
        "Unable to read config generation manifest from ", path.string());
  }

  try {
    return parseConfigGenerationManifest(contents);
  } catch (const std::exception& error) {
    throw FbossError(
        "Unable to parse config generation manifest from ",
        path.string(),
        ": ",
        error.what());
  }
}

} // namespace facebook::fboss::configgen
