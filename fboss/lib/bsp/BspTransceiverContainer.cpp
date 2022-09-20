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

void BspTransceiverContainer::tcvrRead(
    const TransceiverAccessParameter& param,
    uint8_t* buf) const {
  return tcvrIO_->read(param, buf);
}

void BspTransceiverContainer::tcvrWrite(
    const TransceiverAccessParameter& param,
    const uint8_t* buf) const {
  return tcvrIO_->write(param, buf);
}

const I2cControllerStats BspTransceiverContainer::getI2cControllerStats()
    const {
  return tcvrIO_->getI2cControllerPlatformStats();
}

} // namespace fboss
} // namespace facebook
