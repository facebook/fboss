// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/lib/bsp/BspTransceiverApi.h"
#include "fboss/lib/bsp/BspSystemContainer.h"

namespace facebook::fboss {

BspTransceiverApi::BspTransceiverApi(const BspSystemContainer* systemContainer)
    : systemContainer_(systemContainer) {}

/* Trigger the QSFP hard reset for a given QSFP module
 */
void BspTransceiverApi::triggerQsfpHardReset(unsigned int module) {
  systemContainer_->initTransceiver(module);
}

/* This function will bring all the transceivers out of reset.
 */
void BspTransceiverApi::clearAllTransceiverReset() {
  systemContainer_->clearAllTransceiverReset();
}

void BspTransceiverApi::holdTransceiverReset(unsigned int module) {
  systemContainer_->holdTransceiverReset(module);
}

void BspTransceiverApi::releaseTransceiverReset(unsigned int module) {
  systemContainer_->releaseTransceiverReset(module);
}

} // namespace facebook::fboss
