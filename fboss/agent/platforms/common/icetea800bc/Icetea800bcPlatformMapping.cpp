/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/platforms/common/icetea800bc/Icetea800bcPlatformMapping.h"

namespace {
constexpr auto kJsonPlatformMappingStr = R"(
//to be updated with subsequent platform mapping configuration generation and agent code changes.
)";
} // namespace

namespace facebook::fboss {
Icetea800bcPlatformMapping::Icetea800bcPlatformMapping()
    : PlatformMapping(kJsonPlatformMappingStr) {}

Icetea800bcPlatformMapping::Icetea800bcPlatformMapping(
    const std::string& platformMappingStr)
    : PlatformMapping(platformMappingStr) {}

} // namespace facebook::fboss
