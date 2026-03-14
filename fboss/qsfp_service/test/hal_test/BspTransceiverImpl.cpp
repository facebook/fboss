// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/qsfp_service/test/hal_test/BspTransceiverImpl.h"

#include <unistd.h>

namespace facebook::fboss {

BspTransceiverImpl::BspTransceiverImpl(
    int module,
    std::string name,
    BspTransceiverMapping bspMapping)
    : module_(module),
      moduleName_(std::move(name)),
      tcvrContainer_(std::make_unique<BspTransceiverContainer>(bspMapping)) {}

int BspTransceiverImpl::readTransceiver(
    const TransceiverAccessParameter& param,
    uint8_t* fieldValue,
    const int /*field*/) {
  tcvrContainer_->tcvrRead(param, fieldValue);
  return param.len;
}

int BspTransceiverImpl::writeTransceiver(
    const TransceiverAccessParameter& param,
    const uint8_t* fieldValue,
    uint64_t /* delay */,
    const int /*field*/) {
  tcvrContainer_->tcvrWrite(param, fieldValue);
  return param.len;
}

bool BspTransceiverImpl::detectTransceiver() {
  return tcvrContainer_->isTcvrPresent();
}

void BspTransceiverImpl::triggerQsfpHardReset() {
  tcvrContainer_->holdTransceiverReset();
  usleep(100000); // 100ms
  tcvrContainer_->releaseTransceiverReset();
  /* sleep override */
  sleep(2);
}

folly::StringPiece BspTransceiverImpl::getName() {
  return moduleName_;
}

int BspTransceiverImpl::getNum() const {
  return module_;
}

void BspTransceiverImpl::ensureOutOfReset() {
  tcvrContainer_->releaseTransceiverReset();
}

void BspTransceiverImpl::i2cTimeProfilingStart() const {
  tcvrContainer_->i2cTimeProfilingStart();
}

void BspTransceiverImpl::i2cTimeProfilingEnd() const {
  tcvrContainer_->i2cTimeProfilingEnd();
}

std::pair<uint64_t, uint64_t> BspTransceiverImpl::getI2cTimeProfileMsec()
    const {
  return tcvrContainer_->getI2cTimeProfileMsec();
}

} // namespace facebook::fboss
