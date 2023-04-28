/*
 *  Copyright (c) 2023-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/platforms/common/morgan/Morgan800ccPlatformMapping.h"

namespace {
constexpr auto kJsonPlatformMappingStr = R"(
{
)";
} // namespace

namespace facebook {
namespace fboss {
Morgan800ccPlatformMapping::Morgan800ccPlatformMapping()
    : PlatformMapping(kJsonPlatformMappingStr) {}

Morgan800ccPlatformMapping::Morgan800ccPlatformMapping(
    const std::string& platformMappingStr)
    : PlatformMapping(platformMappingStr) {}

} // namespace fboss
} // namespace facebook