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
#include <map>
#include <optional>
#include <string>
#include <string_view>

#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/configs/platforms/generic/forwarding_stacks/gen-cpp2/feature_default_command_args_types.h"

namespace facebook::fboss::configgen {

// Selects the service-specific config below generic/forwarding_stacks. Add a
// value when another service starts consuming default command-line arguments.
enum class ServiceType {
  AGENT,
};

// Parses JSON into the shared Thrift model and rejects unknown fields, invalid
// field types, and semantic errors. This entry point is also used by the
// checked-in config validation test.
FeatureDefaultCommandArgsConfig parseFeatureDefaultCommandArgsConfig(
    std::string_view contents);

// Loads and parses the config associated with serviceType from fbossRoot.
FeatureDefaultCommandArgsConfig loadFeatureDefaultCommandArgsConfig(
    const std::filesystem::path& fbossRoot,
    ServiceType serviceType);

// Combines defaults for profile with every automatically enabled feature whose
// configured profile, ASIC, and platform conditions all match. A missing ASIC
// does not match an ASIC condition. Conflicting argument values are rejected.
std::map<std::string, std::string> resolveFeatureDefaultCommandArgs(
    const FeatureDefaultCommandArgsConfig& config,
    std::string_view profile,
    std::optional<cfg::AsicType> asicType,
    std::string_view platform);

// Convenience entry point that loads the service config and resolves its
// command-line arguments for one config-generation request.
std::map<std::string, std::string> generateFeatureDefaultCommandArgs(
    const std::filesystem::path& fbossRoot,
    ServiceType serviceType,
    std::string_view profile,
    std::optional<cfg::AsicType> asicType,
    std::string_view platform);

} // namespace facebook::fboss::configgen
