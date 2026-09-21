// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/lib/bsp/wedge800bact/Wedge800BACTBspPlatformMapping.h"

#include <thrift/lib/cpp2/protocol/Serializer.h>

namespace facebook::fboss {

Wedge800BACTBspPlatformMapping::Wedge800BACTBspPlatformMapping()
    : BspPlatformMapping("wedge800bact") {}

Wedge800BACTBspPlatformMapping::Wedge800BACTBspPlatformMapping(
    const std::string& platformMappingStr)
    : BspPlatformMapping(
          apache::thrift::SimpleJSONSerializer::deserialize<
              BspPlatformMappingThrift>(platformMappingStr)) {}

} // namespace facebook::fboss
