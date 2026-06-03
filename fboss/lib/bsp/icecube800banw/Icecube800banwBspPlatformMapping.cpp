// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/lib/bsp/icecube800banw/Icecube800banwBspPlatformMapping.h"

#include <thrift/lib/cpp2/protocol/Serializer.h>

namespace facebook::fboss {

Icecube800banwBspPlatformMapping::Icecube800banwBspPlatformMapping()
    : BspPlatformMapping("icecube800banw") {}

Icecube800banwBspPlatformMapping::Icecube800banwBspPlatformMapping(
    const std::string& platformMappingStr)
    : BspPlatformMapping(
          apache::thrift::SimpleJSONSerializer::deserialize<
              BspPlatformMappingThrift>(platformMappingStr)) {}

} // namespace facebook::fboss
