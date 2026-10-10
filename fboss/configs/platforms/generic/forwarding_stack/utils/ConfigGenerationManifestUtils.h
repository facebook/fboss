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
#include <string_view>

#include "fboss/configs/platforms/generic/forwarding_stack/gen-cpp2/config_generation_types.h"

namespace facebook::fboss::configgen {

ConfigGenerationManifest parseConfigGenerationManifest(
    std::string_view contents);

ConfigGenerationManifest loadConfigGenerationManifest(
    const std::filesystem::path& fbossRoot);

} // namespace facebook::fboss::configgen
