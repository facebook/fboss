/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/platforms/common/wedge800cact/Wedge800CACTPlatformMapping.h"

namespace {
constexpr auto kJsonPlatformMappingStr = R"()";
} // namespace

namespace facebook::fboss {
Wedge800CACTPlatformMapping::Wedge800CACTPlatformMapping()
    : PlatformMapping(kJsonPlatformMappingStr) {}

Wedge800CACTPlatformMapping::Wedge800CACTPlatformMapping(
    const std::string& platformMappingStr)
    : PlatformMapping(platformMappingStr) {}

} // namespace facebook::fboss
