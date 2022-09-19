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
  tcvrIO_ = std::make_unique<BspTransceiverIO>(tcvrID_, tcvrMapping);
  tcvrAccess_ = std::make_unique<BspTransceiverAccess>(tcvrID_, tcvrMapping);
}

void BspTransceiverContainer::clearTransceiverReset() const {
  tcvrAccess_->init(false);
}

void BspTransceiverContainer::triggerTcvrHardReset() const {
  tcvrAccess_->init(true);
}

bool BspTransceiverContainer::isTcvrPresent() const {
  return tcvrAccess_->isPresent();
}

} // namespace fboss
} // namespace facebook
