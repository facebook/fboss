// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/lib/bsp/minipack3bta/Minipack3BTABspPlatformMapping.h"

#include <thrift/lib/cpp2/protocol/Serializer.h>

namespace facebook::fboss {

Minipack3BTABspPlatformMapping::Minipack3BTABspPlatformMapping()
    : BspPlatformMapping("minipack3ba") {}

Minipack3BTABspPlatformMapping::Minipack3BTABspPlatformMapping(
    const std::string& platformMappingStr)
    : BspPlatformMapping(
          apache::thrift::SimpleJSONSerializer::deserialize<
              BspPlatformMappingThrift>(platformMappingStr)) {}

} // namespace facebook::fboss
