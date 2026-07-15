/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/platforms/common/m5120csc/M5120CSCPlatformMapping.h"

namespace {
// placeholder for now, will be filled in later
constexpr auto kJsonPlatformMappingStr = R"()";
} // namespace

namespace facebook::fboss {
M5120CSCPlatformMapping::M5120CSCPlatformMapping()
    : PlatformMapping(kJsonPlatformMappingStr) {}

M5120CSCPlatformMapping::M5120CSCPlatformMapping(
    const std::string& platformMappingStr)
    : PlatformMapping(platformMappingStr) {}

} // namespace facebook::fboss
