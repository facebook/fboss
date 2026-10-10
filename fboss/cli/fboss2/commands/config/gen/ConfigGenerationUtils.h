/*
 *  Copyright (c) Meta Platforms, Inc. and affiliates.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#pragma once

#include <filesystem>
#include <functional>
#include <optional>
#include <string>
#include <vector>

#include "fboss/configs/platforms/generic/forwarding_stack/gen-cpp2/config_generation_types.h"
#include "fboss/lib/if/gen-cpp2/fboss_common_types.h"

namespace facebook::fboss::configgen {

// Service-neutral inputs resolved from one manifest option. Service-specific
// generators consume this request to produce serialized config contents.
struct ConfigGenerationRequest {
  std::filesystem::path fbossRoot;
  PlatformType platformType;
  std::string platform;
  ConfigProfileType profileType;
  std::string profile;
  std::optional<std::string> variant;
  ServiceType service;
  std::string serviceName;
  std::filesystem::path platformDirectory;
  std::filesystem::path outputPath;
};

using ConfigContentsGenerator =
    std::function<std::string(const ConfigGenerationRequest&)>;

// Generates every target for one service from the checked-in manifest. All
// contents are generated before any output is written, then each canonical
// platform forwarding_stack/generated/<service> output is atomically
// refreshed. Paths are returned in manifest order.
std::vector<std::filesystem::path> generateConfigsFromManifest(
    const std::filesystem::path& fbossRoot,
    ServiceType service,
    const ConfigContentsGenerator& generateContents);

} // namespace facebook::fboss::configgen
