/*
 *  Copyright (c) 2026 Nexthop Systems Inc.
 *  SPDX-License-Identifier: BSD-3-Clause
 */

#include "fboss/agent/platforms/common/m4062nhp/M4062nhpPlatformMapping.h"

namespace {
constexpr auto kJsonPlatformMappingStr = R"({})";
} // namespace

namespace facebook::fboss {

M4062nhpPlatformMapping::M4062nhpPlatformMapping()
    : PlatformMapping(kJsonPlatformMappingStr) {}

M4062nhpPlatformMapping::M4062nhpPlatformMapping(
    const std::string& platformMappingStr)
    : PlatformMapping(platformMappingStr) {}

} // namespace facebook::fboss
