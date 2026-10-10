/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/cli/fboss2/commands/config/gen/FeatureDefaultCommandArgs.h"

#include <algorithm>
#include <optional>
#include <string>

#include <folly/FileUtil.h>
#include <folly/json/json.h>
#include <thrift/lib/cpp/util/EnumUtils.h>
#include <thrift/lib/cpp2/protocol/Serializer.h>

#include "fboss/agent/FbossError.h"

namespace facebook::fboss::configgen {
namespace fs = std::filesystem;
namespace {

fs::path getConfigPath(ServiceType serviceType) {
  constexpr std::string_view kBasePath =
      "configs/platforms/generic/forwarding_stack";
  constexpr std::string_view kJsonFile =
      "feature_default_command_args_config.json";
  switch (serviceType) {
    case ServiceType::AGENT:
      return fs::path(kBasePath) / "agent" / kJsonFile;
  }
  throw FbossError("Unsupported service type");
}

// SimpleJSON intentionally ignores unknown fields. Comparing the input with
// its Thrift-normalized form makes misspelled fields visible without
// duplicating the IDL as a handwritten schema. Keep this check in production:
// callers can load configs from an arbitrary fbossRoot, while the config GTest
// only protects files checked into this repository.
void validateNoUnknownFields(
    const folly::dynamic& input,
    const folly::dynamic& normalized,
    const std::string& path) {
  if (input.isObject() && normalized.isObject()) {
    for (const auto& [name, value] : input.items()) {
      const auto normalizedIt = normalized.find(name);
      if (normalizedIt == normalized.items().end()) {
        throw FbossError("Unknown field '", name.asString(), "' at ", path);
      }
      validateNoUnknownFields(
          value, normalizedIt->second, path + "." + name.asString());
    }
  } else if (input.isArray() && normalized.isArray()) {
    const auto commonSize = std::min(input.size(), normalized.size());
    for (size_t index = 0; index < commonSize; ++index) {
      validateNoUnknownFields(
          input[index],
          normalized[index],
          path + "[" + std::to_string(index) + "]");
    }
  }
}

void validateNoUnknownFields(
    std::string_view contents,
    const FeatureDefaultCommandArgsConfig& config) {
  const auto input = folly::parseJson(contents);
  const auto normalized = folly::parseJson(
      apache::thrift::SimpleJSONSerializer::serialize<std::string>(config));
  validateNoUnknownFields(input, normalized, "$");
}

// The remaining validation helpers enforce semantic constraints that the
// Thrift type system cannot express, such as nonempty condition sets,
// recognized ASIC names, and conflict-free argument composition.
void validateConditionValues(
    const FeatureConditionValues& values,
    std::string_view context) {
  if (values.included()->empty() && values.excluded()->empty()) {
    throw FbossError(context, " must contain at least one value");
  }
  const auto validateNonEmpty = [context](const auto& entries) {
    for (const auto& entry : entries) {
      if (entry.empty()) {
        throw FbossError(context, " must not contain an empty value");
      }
    }
  };
  validateNonEmpty(*values.included());
  validateNonEmpty(*values.excluded());
  for (const auto& value : *values.included()) {
    if (values.excluded()->contains(value)) {
      throw FbossError(
          context, " contains '", value, "' in both included and excluded");
    }
  }
}

template <typename Enum>
void validateEnumNames(
    const FeatureConditionValues& values,
    std::string_view typeName) {
  const auto validate = [typeName](const auto& names) {
    for (const auto& name : names) {
      Enum value;
      if (!apache::thrift::util::tryParseEnum(name, &value)) {
        throw FbossError(
            "Unknown ", typeName, " in feature conditions: ", name);
      }
    }
  };
  validate(*values.included());
  validate(*values.excluded());
}

void validateConfig(const FeatureDefaultCommandArgsConfig& config) {
  for (const auto& profileAndArgs : *config.profileDefaultArgs()) {
    if (profileAndArgs.first.empty()) {
      throw FbossError("Profile names must not be empty");
    }
    for (const auto& argument : profileAndArgs.second) {
      if (argument.first.empty()) {
        throw FbossError("Command-line argument names must not be empty");
      }
    }
  }
  for (const auto& [featureName, feature] : *config.features()) {
    if (featureName.empty()) {
      throw FbossError("Feature names must not be empty");
    }
    if (feature.args()->empty()) {
      throw FbossError("Feature '", featureName, "' has no arguments");
    }
    for (const auto& argument : *feature.args()) {
      if (argument.first.empty()) {
        throw FbossError(
            "Feature '", featureName, "' has an empty argument name");
      }
    }
    if (!feature.autoEnableWhen()) {
      continue;
    }
    const auto& conditions = *feature.autoEnableWhen();
    if (!conditions.configProfiles() && !conditions.asicTypes() &&
        !conditions.platforms()) {
      throw FbossError(
          "Feature '",
          featureName,
          "' has autoEnableWhen without any conditions");
    }
    if (conditions.configProfiles()) {
      validateConditionValues(
          *conditions.configProfiles(), "configProfiles condition");
    }
    if (conditions.asicTypes()) {
      validateConditionValues(*conditions.asicTypes(), "asicTypes condition");
      validateEnumNames<cfg::AsicType>(*conditions.asicTypes(), "ASIC type");
    }
    if (conditions.platforms()) {
      validateConditionValues(*conditions.platforms(), "platforms condition");
      validateEnumNames<PlatformType>(*conditions.platforms(), "platform type");
    }
  }
}

bool matches(
    const std::optional<FeatureConditionValues>& condition,
    std::string_view value) {
  if (!condition) {
    return true;
  }
  const auto stringValue = std::string(value);
  if (condition->excluded()->contains(stringValue)) {
    return false;
  }
  return condition->included()->empty() ||
      condition->included()->contains(stringValue);
}

template <typename Enum>
bool matchesEnum(
    const std::optional<FeatureConditionValues>& condition,
    Enum value) {
  if (!condition) {
    return true;
  }
  const auto matchesValue = [value](const auto& names) {
    return std::any_of(names.begin(), names.end(), [value](const auto& name) {
      Enum candidate;
      return apache::thrift::util::tryParseEnum(name, &candidate) &&
          candidate == value;
    });
  };
  if (matchesValue(*condition->excluded())) {
    return false;
  }
  return condition->included()->empty() || matchesValue(*condition->included());
}

bool matches(
    const std::optional<FeatureConditionValues>& condition,
    std::optional<cfg::AsicType> asicType) {
  if (!condition) {
    return true;
  }
  return asicType && matchesEnum(condition, *asicType);
}

void mergeArgs(
    std::map<std::string, std::string>& resolved,
    const std::map<std::string, std::string>& additions,
    std::string_view source) {
  for (const auto& [name, value] : additions) {
    const auto [it, inserted] = resolved.emplace(name, value);
    if (!inserted && it->second != value) {
      throw FbossError(
          "Command-line argument '",
          name,
          "' has conflicting values '",
          it->second,
          "' and '",
          value,
          "' from ",
          source);
    }
  }
}

} // namespace

FeatureDefaultCommandArgsConfig parseFeatureDefaultCommandArgsConfig(
    std::string_view contents) {
  auto config = apache::thrift::SimpleJSONSerializer::deserialize<
      FeatureDefaultCommandArgsConfig>(contents);
  validateNoUnknownFields(contents, config);
  validateConfig(config);
  return config;
}

FeatureDefaultCommandArgsConfig loadFeatureDefaultCommandArgsConfig(
    const fs::path& fbossRoot,
    ServiceType serviceType) {
  const auto path = fbossRoot / getConfigPath(serviceType);
  std::string contents;
  if (!folly::readFile(path.c_str(), contents)) {
    throw FbossError(
        "Unable to read feature default command arguments from ",
        path.string());
  }

  try {
    return parseFeatureDefaultCommandArgsConfig(contents);
  } catch (const FbossError&) {
    throw;
  } catch (const std::exception& error) {
    throw FbossError(
        "Unable to parse feature default command arguments from ",
        path.string(),
        ": ",
        error.what());
  }
}

std::map<std::string, std::string> resolveFeatureDefaultCommandArgs(
    const FeatureDefaultCommandArgsConfig& config,
    std::string_view profile,
    std::optional<cfg::AsicType> asicType,
    PlatformType platformType) {
  // Callers may construct the Thrift object directly instead of using the JSON
  // parser, so semantic validation also belongs at this public entry point.
  validateConfig(config);
  std::map<std::string, std::string> resolved;
  const auto profileName = std::string(profile);
  if (const auto profileIt = config.profileDefaultArgs()->find(profileName);
      profileIt != config.profileDefaultArgs()->end()) {
    mergeArgs(resolved, profileIt->second, "profile defaults");
  }

  for (const auto& [featureName, feature] : *config.features()) {
    if (!feature.autoEnableWhen()) {
      continue;
    }
    const auto& conditions = *feature.autoEnableWhen();
    if (matches(conditions.configProfiles().to_optional(), profile) &&
        matches(conditions.asicTypes().to_optional(), asicType) &&
        matchesEnum(conditions.platforms().to_optional(), platformType)) {
      mergeArgs(resolved, *feature.args(), featureName);
    }
  }
  return resolved;
}

std::map<std::string, std::string> generateFeatureDefaultCommandArgs(
    const fs::path& fbossRoot,
    ServiceType serviceType,
    std::string_view profile,
    std::optional<cfg::AsicType> asicType,
    PlatformType platformType) {
  return resolveFeatureDefaultCommandArgs(
      loadFeatureDefaultCommandArgsConfig(fbossRoot, serviceType),
      profile,
      asicType,
      platformType);
}

} // namespace facebook::fboss::configgen
