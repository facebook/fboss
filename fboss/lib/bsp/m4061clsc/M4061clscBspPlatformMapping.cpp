// Copyright (c) Meta Platforms, Inc. and affiliates.

#include "fboss/lib/bsp/m4061clsc/M4061clscBspPlatformMapping.h"
#include <thrift/lib/cpp2/protocol/Serializer.h>
#include "fboss/lib/bsp/BspPlatformMapping.h"
#include "fboss/lib/bsp/gen-cpp2/bsp_platform_mapping_types.h"

namespace facebook::fboss {

M4061clscBspPlatformMapping::M4061clscBspPlatformMapping()
    : BspPlatformMapping("m4061clsc") {}

M4061clscBspPlatformMapping::M4061clscBspPlatformMapping(
    const std::string& platformMappingStr)
    : BspPlatformMapping(
          apache::thrift::SimpleJSONSerializer::deserialize<
              BspPlatformMappingThrift>(platformMappingStr)) {}

} // namespace facebook::fboss
