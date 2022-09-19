// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/lib/bsp/BspTransceiverApi.h"
#include "fboss/lib/bsp/BspSystemContainer.h"

namespace facebook::fboss {

BspTransceiverApi::BspTransceiverApi(const BspSystemContainer* systemContainer)
    : systemContainer_(systemContainer) {}

/* Trigger the QSFP hard reset for a given QSFP module
 */
void BspTransceiverApi::triggerQsfpHardReset(unsigned int module) {
  systemContainer_->triggerQsfpHardReset(module);
}

/* This function will bring all the transceivers out of reset.
 */
void BspTransceiverApi::clearAllTransceiverReset() {
  systemContainer_->clearAllTransceiverReset();
}

} // namespace facebook::fboss
