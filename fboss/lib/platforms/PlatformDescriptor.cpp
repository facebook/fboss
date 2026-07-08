/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/lib/platforms/PlatformDescriptor.h"

#include <boost/algorithm/string.hpp>
#include <filesystem>
#include <utility>

#include <folly/FileUtil.h>
#include <thrift/lib/cpp2/FieldRef.h>
#include <thrift/lib/cpp2/protocol/Serializer.h>

#include "fboss/agent/FbossError.h"
#include "fboss/agent/gen-cpp2/platform_config_types.h"

DEFINE_string(
    platform_descriptor_config_path,
    "",
    "Path to a platform descriptor config root. Expected layout: "
    "<root>/<system_vendor>/<platform_name>/platform_descriptor.json and "
    "<root>/<system_vendor>/<platform_name>/platform_mapping.json.");

namespace fs = std::filesystem;

namespace facebook::fboss {
namespace {

constexpr auto kPlatformDescriptorFileName = "platform_descriptor.json";
constexpr auto kPlatformMappingFileName = "platform_mapping.json";

std::string normalize(std::string_view value) {
  return boost::algorithm::to_lower_copy(std::string(value));
}

bool startsWithNormalized(
    std::string_view candidate,
    const std::string& normalizedPrefix) {
  auto normalizedCandidate = normalize(candidate);
  return normalizedCandidate.rfind(normalizedPrefix, 0) == 0;
}

bool equalsNormalized(
    std::string_view candidate,
    const std::string& normalizedExpected) {
  return normalize(candidate) == normalizedExpected;
}

void validatePlatformDescriptor(
    const PlatformDescriptor& descriptor,
    const std::string& path) {
  if (!apache::thrift::is_non_optional_field_set_manually_or_by_serializer(
          descriptor.platformType())) {
    throw FbossError("Platform descriptor ", path, " is missing platformType");
  }
  if (!apache::thrift::is_non_optional_field_set_manually_or_by_serializer(
          descriptor.productNamePrefixes())) {
    throw FbossError(
        "Platform descriptor ", path, " is missing productNamePrefixes");
  }
  if (!apache::thrift::is_non_optional_field_set_manually_or_by_serializer(
          descriptor.modeNames())) {
    throw FbossError("Platform descriptor ", path, " is missing modeNames");
  }
  if (!apache::thrift::is_non_optional_field_set_manually_or_by_serializer(
          descriptor.asicType())) {
    throw FbossError("Platform descriptor ", path, " is missing asicType");
  }
}

void addPlatformDirsFromVendorDir(
    const fs::path& vendorDir,
    std::vector<fs::path>& platformDirs) {
  for (const auto& platformEntry : fs::directory_iterator(vendorDir)) {
    if (platformEntry.is_directory()) {
      platformDirs.push_back(platformEntry.path());
    }
  }
}

std::vector<fs::path> getPlatformDirs(
    const fs::path& descriptorDir,
    const std::string& path,
    std::string_view systemVendor) {
  std::vector<fs::path> platformDirs;
  auto normalizedVendor = normalize(systemVendor);

  // Descriptor config is organized as <root>/<system_vendor>/<platform_name>/.
  // When a vendor is specified, only scan that vendor directory.
  if (!normalizedVendor.empty()) {
    auto vendorDir = descriptorDir / normalizedVendor;
    if (fs::is_directory(vendorDir)) {
      addPlatformDirsFromVendorDir(vendorDir, platformDirs);
    }
  } else {
    // Without a vendor hint, scan all vendor directories under the root.
    for (const auto& vendorEntry : fs::directory_iterator(descriptorDir)) {
      if (vendorEntry.is_directory()) {
        addPlatformDirsFromVendorDir(vendorEntry.path(), platformDirs);
      }
    }
  }

  if (platformDirs.empty()) {
    if (!normalizedVendor.empty()) {
      return platformDirs;
    }
    throw FbossError(
        "Platform descriptor directory ",
        path,
        " does not contain any vendor/platform directories");
  }
  return platformDirs;
}

fs::path getRequiredPlatformFile(
    const fs::path& platformDir,
    std::string_view fileName) {
  auto file = platformDir / fileName;
  if (!fs::exists(file)) {
    throw FbossError(
        "Platform directory ", platformDir.string(), " is missing ", fileName);
  }
  return file;
}

} // namespace

PlatformDescriptorRegistry::PlatformDescriptorRegistry(
    std::vector<PlatformDescriptorEntry> descriptorEntries)
    : descriptorEntries_(std::move(descriptorEntries)) {}

const PlatformDescriptorRegistry& PlatformDescriptorRegistry::get() {
  static PlatformDescriptorRegistry instance(
      FLAGS_platform_descriptor_config_path.empty()
          ? std::vector<PlatformDescriptorEntry>{}
          : loadPlatformDescriptorEntriesFromDirectory(
                FLAGS_platform_descriptor_config_path));
  return instance;
}

const PlatformDescriptor* FOLLY_NULLABLE
PlatformDescriptorRegistry::getDescriptor(PlatformType type) const {
  for (const auto& entry : descriptorEntries_) {
    if (entry.descriptor.platformType().value() == type) {
      return &entry.descriptor;
    }
  }
  return nullptr;
}

std::optional<PlatformType> PlatformDescriptorRegistry::findPlatformType(
    std::string_view productName,
    std::string_view mode) const {
  if (!mode.empty()) {
    for (const auto& entry : descriptorEntries_) {
      for (const auto& modeName : entry.descriptor.modeNames().value()) {
        if (equalsNormalized(mode, normalize(modeName))) {
          return entry.descriptor.platformType().value();
        }
      }
    }
    return std::nullopt;
  }

  for (const auto& entry : descriptorEntries_) {
    for (const auto& productNamePrefix :
         entry.descriptor.productNamePrefixes().value()) {
      if (startsWithNormalized(productName, normalize(productNamePrefix))) {
        return entry.descriptor.platformType().value();
      }
    }
  }
  return std::nullopt;
}

std::optional<std::string> PlatformDescriptorRegistry::loadPlatformMapping(
    PlatformType type) const {
  for (const auto& entry : descriptorEntries_) {
    if (entry.descriptor.platformType().value() != type) {
      continue;
    }
    if (entry.platformMappingPath.empty()) {
      return std::nullopt;
    }

    std::string mappingJson;
    if (!folly::readFile(entry.platformMappingPath.c_str(), mappingJson)) {
      throw FbossError(
          "Unable to read platform mapping file ", entry.platformMappingPath);
    }

    try {
      cfg::PlatformMapping mapping;
      apache::thrift::SimpleJSONSerializer::deserialize<cfg::PlatformMapping>(
          mappingJson, mapping);
    } catch (const std::exception& ex) {
      throw FbossError(
          "Unable to parse platform mapping file ",
          entry.platformMappingPath,
          ": ",
          ex.what());
    }
    return mappingJson;
  }
  return std::nullopt;
}

PlatformDescriptor PlatformDescriptorRegistry::loadPlatformDescriptorFromFile(
    const std::string& path) {
  if (!fs::exists(path)) {
    throw FbossError("Unable to find platform descriptor file ", path);
  }

  std::string descriptorJson;
  if (!folly::readFile(path.c_str(), descriptorJson)) {
    throw FbossError("Unable to read platform descriptor file ", path);
  }

  PlatformDescriptor descriptor;
  try {
    apache::thrift::SimpleJSONSerializer::deserialize<PlatformDescriptor>(
        descriptorJson, descriptor);
  } catch (const std::exception& ex) {
    throw FbossError(
        "Unable to parse platform descriptor file ", path, ": ", ex.what());
  }
  validatePlatformDescriptor(descriptor, path);
  return descriptor;
}

PlatformDescriptorRegistry
PlatformDescriptorRegistry::loadPlatformDescriptorRegistryFromDirectory(
    const std::string& path,
    std::string_view systemVendor) {
  return PlatformDescriptorRegistry(
      loadPlatformDescriptorEntriesFromDirectory(path, systemVendor));
}

std::vector<PlatformDescriptorRegistry::PlatformDescriptorEntry>
PlatformDescriptorRegistry::loadPlatformDescriptorEntriesFromDirectory(
    const std::string& path,
    std::string_view systemVendor) {
  const fs::path descriptorDir(path);
  if (!fs::is_directory(descriptorDir)) {
    throw FbossError(
        "Platform descriptor config path ", path, " is not a directory");
  }

  auto platformDirs = getPlatformDirs(descriptorDir, path, systemVendor);
  std::vector<PlatformDescriptorEntry> descriptorEntries;
  descriptorEntries.reserve(platformDirs.size());
  for (const auto& platformDir : platformDirs) {
    auto descriptorFile =
        getRequiredPlatformFile(platformDir, kPlatformDescriptorFileName);
    auto mappingFile =
        getRequiredPlatformFile(platformDir, kPlatformMappingFileName);

    descriptorEntries.push_back(
        PlatformDescriptorEntry{
            loadPlatformDescriptorFromFile(descriptorFile.string()),
            mappingFile.string()});
  }
  return descriptorEntries;
}

} // namespace facebook::fboss
