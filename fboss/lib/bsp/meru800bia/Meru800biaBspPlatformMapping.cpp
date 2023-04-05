// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/lib/bsp/meru800bia/Meru800biaBspPlatformMapping.h"
#include "fboss/lib/bsp/BspPlatformMapping.h"
#include "fboss/lib/bsp/gen-cpp2/bsp_platform_mapping_types.h"

using namespace facebook::fboss;

namespace {
static BspPlatformMappingThrift buildMeru800biaPlatformMapping() {
  BspPlatformMappingThrift mapping;
  // TODO
  return mapping;
}

} // namespace

namespace facebook {
namespace fboss {

// TODO: Use pre generated bsp platform mapping from cfgr
Meru800biaBspPlatformMapping::Meru800biaBspPlatformMapping()
    : BspPlatformMapping(buildMeru800biaPlatformMapping()) {}

} // namespace fboss
} // namespace facebook
