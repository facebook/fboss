/*
 *  Copyright (c) 2023-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/platforms/common/minipack3n/Minipack3NPlatformMapping.h"

namespace {
constexpr auto kJsonPlatformMappingStr = R"(
)";
} // namespace

namespace facebook::fboss {
Minipack3NPlatformMapping::Minipack3NPlatformMapping()
    : PlatformMapping(kJsonPlatformMappingStr) {}

Minipack3NPlatformMapping::Minipack3NPlatformMapping(
    const std::string& platformMappingStr)
    : PlatformMapping(platformMappingStr) {}

} // namespace facebook::fboss
