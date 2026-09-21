// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/lib/bsp/BspTransceiverContainer.h"
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

void BspTransceiverContainer::releaseTransceiverReset() const {
  tcvrAccess_->releaseReset();
}

void BspTransceiverContainer::holdTransceiverReset() const {
  tcvrAccess_->holdReset();
}

void BspTransceiverContainer::initTcvr() const {
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

void BspTransceiverContainer::i2cTimeProfilingStart() const {
  tcvrIO_->i2cTimeProfilingStart();
}

void BspTransceiverContainer::i2cTimeProfilingEnd() const {
  tcvrIO_->i2cTimeProfilingEnd();
}

std::pair<uint64_t, uint64_t> BspTransceiverContainer::getI2cTimeProfileMsec()
    const {
  return tcvrIO_->getI2cTimeProfileMsec();
}

} // namespace fboss
} // namespace facebook
