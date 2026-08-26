// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/lib/bsp/tahan800bc/Tahan800bcBspPlatformMapping.h"

#include <thrift/lib/cpp2/protocol/Serializer.h>

namespace facebook::fboss {

Tahan800bcBspPlatformMapping::Tahan800bcBspPlatformMapping()
    : BspPlatformMapping("tahan800bc") {}

Tahan800bcBspPlatformMapping::Tahan800bcBspPlatformMapping(
    const std::string& platformMappingStr)
    : BspPlatformMapping(
          apache::thrift::SimpleJSONSerializer::deserialize<
              BspPlatformMappingThrift>(platformMappingStr)) {}

} // namespace facebook::fboss
