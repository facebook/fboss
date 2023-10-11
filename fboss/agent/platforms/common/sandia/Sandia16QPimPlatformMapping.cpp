/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/platforms/common/sandia/Sandia16QPimPlatformMapping.h"

namespace {
constexpr auto kJsonPlatformMappingStr = R"()";
} // namespace

namespace facebook {
namespace fboss {
Sandia16QPimPlatformMapping::Sandia16QPimPlatformMapping()
    : MultiPimPlatformMapping(kJsonPlatformMappingStr) {}

Sandia16QPimPlatformMapping::Sandia16QPimPlatformMapping(
    const std::string& platformMappingStr)
    : MultiPimPlatformMapping(platformMappingStr) {}

} // namespace fboss
} // namespace facebook
