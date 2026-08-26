// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/lib/bsp/tahansb800bc/Tahansb800bcBspPlatformMapping.h"

#include <thrift/lib/cpp2/protocol/Serializer.h>

namespace facebook::fboss {

Tahansb800bcBspPlatformMapping::Tahansb800bcBspPlatformMapping()
    : BspPlatformMapping("tahansb800bc") {}

Tahansb800bcBspPlatformMapping::Tahansb800bcBspPlatformMapping(
    const std::string& platformMappingStr)
    : BspPlatformMapping(
          apache::thrift::SimpleJSONSerializer::deserialize<
              BspPlatformMappingThrift>(platformMappingStr)) {}

} // namespace facebook::fboss
