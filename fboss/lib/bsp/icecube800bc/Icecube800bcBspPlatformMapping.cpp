// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/lib/bsp/icecube800bc/Icecube800bcBspPlatformMapping.h"

#include <thrift/lib/cpp2/protocol/Serializer.h>

namespace facebook::fboss {

Icecube800bcBspPlatformMapping::Icecube800bcBspPlatformMapping()
    : BspPlatformMapping("icecube") {}

Icecube800bcBspPlatformMapping::Icecube800bcBspPlatformMapping(
    const std::string& platformMappingStr)
    : BspPlatformMapping(
          apache::thrift::SimpleJSONSerializer::deserialize<
              BspPlatformMappingThrift>(platformMappingStr)) {}

} // namespace facebook::fboss
