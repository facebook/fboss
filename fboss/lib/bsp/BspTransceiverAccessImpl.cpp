// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/lib/bsp/BspTransceiverAccessImpl.h"

#include "fboss/lib/bsp/gen-cpp2/bsp_platform_mapping_types.h"

namespace facebook {
namespace fboss {

BspTransceiverAccessImpl::BspTransceiverAccessImpl(
    uint32_t tcvr,
    BspTransceiverMapping& tcvrMapping) {
  tcvrMapping_ = tcvrMapping;
  tcvrID_ = tcvr;
}

} // namespace fboss
} // namespace facebook
