// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/lib/bsp/morgan800cc/Morgan800ccBspPlatformMapping.h"
#include <thrift/lib/cpp2/protocol/Serializer.h>
#include "fboss/lib/bsp/BspPlatformMapping.h"
#include "fboss/lib/bsp/gen-cpp2/bsp_platform_mapping_types.h"

using namespace facebook::fboss;

namespace {

constexpr auto kJsonBspPlatformMappingStr = R"(
{}
)";

static BspPlatformMappingThrift buildMorgan800ccPlatformMapping(
    const std::string& platformMappingStr) {
  return apache::thrift::SimpleJSONSerializer::deserialize<
      BspPlatformMappingThrift>(platformMappingStr);
}

} // namespace

namespace facebook {
namespace fboss {

Morgan800ccBspPlatformMapping::Morgan800ccBspPlatformMapping()
    : BspPlatformMapping(
          buildMorgan800ccPlatformMapping(kJsonBspPlatformMappingStr)) {}

Morgan800ccBspPlatformMapping::Morgan800ccBspPlatformMapping(
    const std::string& platformMappingStr)
    : BspPlatformMapping(buildMorgan800ccPlatformMapping(platformMappingStr)) {}

} // namespace fboss
} // namespace facebook
