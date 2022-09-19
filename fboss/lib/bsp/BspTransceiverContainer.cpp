// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/lib/bsp/BspTransceiverContainer.h"
#include "fboss/lib/bsp/BspPlatformMapping.h"
#include "fboss/lib/bsp/gen-cpp2/bsp_platform_mapping_types.h"

namespace facebook {
namespace fboss {

BspTransceiverContainer::BspTransceiverContainer(
    BspTransceiverMapping& tcvrMapping)
    : tcvrMapping_(tcvrMapping) {
  tcvrID_ = *tcvrMapping.tcvrId();
}

} // namespace fboss
} // namespace facebook
