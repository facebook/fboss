// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/lib/bsp/meru800bfa/Meru800bfaBspPlatformMapping.h"

#include <thrift/lib/cpp2/protocol/Serializer.h>

namespace facebook::fboss {

Meru800bfaBspPlatformMapping::Meru800bfaBspPlatformMapping()
    : BspPlatformMapping("meru800bfa") {}

Meru800bfaBspPlatformMapping::Meru800bfaBspPlatformMapping(
    const std::string& platformMappingStr)
    : BspPlatformMapping(
          apache::thrift::SimpleJSONSerializer::deserialize<
              BspPlatformMappingThrift>(platformMappingStr)) {}

} // namespace facebook::fboss
