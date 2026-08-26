// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/lib/bsp/morgan800cc/Morgan800ccBspPlatformMapping.h"

#include <thrift/lib/cpp2/protocol/Serializer.h>

namespace facebook::fboss {

Morgan800ccBspPlatformMapping::Morgan800ccBspPlatformMapping()
    : BspPlatformMapping("morgan800cc") {}

Morgan800ccBspPlatformMapping::Morgan800ccBspPlatformMapping(
    const std::string& platformMappingStr)
    : BspPlatformMapping(
          apache::thrift::SimpleJSONSerializer::deserialize<
              BspPlatformMappingThrift>(platformMappingStr)) {}

} // namespace facebook::fboss
