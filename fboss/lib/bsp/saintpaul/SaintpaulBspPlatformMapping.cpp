// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/lib/bsp/saintpaul/SaintpaulBspPlatformMapping.h"

#include <thrift/lib/cpp2/protocol/Serializer.h>

namespace facebook::fboss {

SaintpaulBspPlatformMapping::SaintpaulBspPlatformMapping()
    : BspPlatformMapping("saintpaul") {}

SaintpaulBspPlatformMapping::SaintpaulBspPlatformMapping(
    const std::string& platformMappingStr)
    : BspPlatformMapping(
          apache::thrift::SimpleJSONSerializer::deserialize<
              BspPlatformMappingThrift>(platformMappingStr)) {}

} // namespace facebook::fboss
