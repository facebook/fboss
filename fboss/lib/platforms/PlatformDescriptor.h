/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <folly/CppAttributes.h>

#include "fboss/lib/platforms/gen-cpp2/platform_descriptor_types.h"

DECLARE_string(platform_descriptor_config_path);

namespace facebook::fboss {

class PlatformDescriptorRegistry {
 public:
  // Returns a cached singleton loaded from
  // FLAGS_platform_descriptor_config_path. All vendor directories are scanned
  // once on first access.
  static const PlatformDescriptorRegistry& get();

  const PlatformDescriptor* FOLLY_NULLABLE
  getDescriptor(PlatformType type) const;
  std::optional<PlatformType> findPlatformType(
      std::string_view productName,
      std::string_view mode) const;
  std::optional<std::string> loadPlatformMapping(PlatformType type) const;

  static PlatformDescriptor loadPlatformDescriptorFromFile(
      const std::string& path);
  // Loads descriptors from:
  // <path>/<system_vendor>/<platform_name>/platform_descriptor.json
  // and records each sibling platform_mapping.json for lazy mapping load.
  // systemVendor optionally narrows the scan to a single vendor directory.
  static PlatformDescriptorRegistry loadPlatformDescriptorRegistryFromDirectory(
      const std::string& path,
      std::string_view systemVendor = "");

 private:
  struct PlatformDescriptorEntry {
    PlatformDescriptor descriptor;
    std::string platformMappingPath;
  };

  explicit PlatformDescriptorRegistry(
      std::vector<PlatformDescriptorEntry> descriptorEntries);

  const PlatformDescriptorEntry* FOLLY_NULLABLE
  getDescriptorEntry(PlatformType type) const;

  static std::vector<PlatformDescriptorEntry>
  loadPlatformDescriptorEntriesFromDirectory(
      const std::string& path,
      std::string_view systemVendor = "");

  std::vector<PlatformDescriptorEntry> descriptorEntries_;
};

} // namespace facebook::fboss
