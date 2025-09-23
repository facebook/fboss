/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/platforms/common/tahansb800bc/Tahansb800bcPlatformMapping.h"

namespace {
constexpr auto kJsonPlatformMappingStr = R"(
// to be added the generated platform mapping configuration 
// in the later PR for platform maping
)";
} // namespace

namespace facebook::fboss {
Tahansb800bcPlatformMapping::Tahansb800bcPlatformMapping()
    : PlatformMapping(kJsonPlatformMappingStr) {}

Tahansb800bcPlatformMapping::Tahansb800bcPlatformMapping(
    const std::string& platformMappingStr)
    : PlatformMapping(platformMappingStr) {}

} // namespace facebook::fboss
