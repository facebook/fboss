// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/lib/bsp/gen-cpp2/bsp_platform_mapping_types.h"

namespace facebook::fboss {

class XcvrLib;

BspPlatformMappingThrift buildFromXcvrLib(const std::string& platformName);
BspPlatformMappingThrift buildFromXcvrLib(const XcvrLib& xcvrLib);

} // namespace facebook::fboss
