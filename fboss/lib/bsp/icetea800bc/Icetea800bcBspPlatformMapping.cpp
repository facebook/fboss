// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/lib/bsp/icetea800bc/Icetea800bcBspPlatformMapping.h"

#include <thrift/lib/cpp2/protocol/Serializer.h>

namespace facebook::fboss {

Icetea800bcBspPlatformMapping::Icetea800bcBspPlatformMapping()
    : BspPlatformMapping("icetea") {}

Icetea800bcBspPlatformMapping::Icetea800bcBspPlatformMapping(
    const std::string& platformMappingStr)
    : BspPlatformMapping(
          apache::thrift::SimpleJSONSerializer::deserialize<
              BspPlatformMappingThrift>(platformMappingStr)) {}

} // namespace facebook::fboss
