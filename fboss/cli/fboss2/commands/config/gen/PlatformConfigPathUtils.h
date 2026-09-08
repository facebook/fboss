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

#include <filesystem>
#include <string>
#include <string_view>

namespace facebook::fboss::configgen {

struct PlatformConfigDirectory {
  std::string systemVendor;
  std::filesystem::path path;
};

// Locates a platform below configs/platforms/<system_vendor>. Returns both the
// vendor name and platform directory, and rejects missing or duplicate names.
PlatformConfigDirectory findPlatformConfigDirectory(
    const std::filesystem::path& fbossRoot,
    std::string_view platform);

// Resolves a direct component directory, such as asic_config or
// platform_mapping, below an already located platform directory.
std::filesystem::path findPlatformConfigComponentDirectory(
    const PlatformConfigDirectory& platformDirectory,
    std::string_view component);

} // namespace facebook::fboss::configgen
