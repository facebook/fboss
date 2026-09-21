// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/lib/bsp/wedge800cact/Wedge800CACTBspPlatformMapping.h"

#include <thrift/lib/cpp2/protocol/Serializer.h>

namespace facebook::fboss {

Wedge800CACTBspPlatformMapping::Wedge800CACTBspPlatformMapping()
    : BspPlatformMapping("wedge800cact") {}

Wedge800CACTBspPlatformMapping::Wedge800CACTBspPlatformMapping(
    const std::string& platformMappingStr)
    : BspPlatformMapping(
          apache::thrift::SimpleJSONSerializer::deserialize<
              BspPlatformMappingThrift>(platformMappingStr)) {}

} // namespace facebook::fboss
