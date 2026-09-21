/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/lib/usb/Minipack16QTransceiverApi.h"
#include "fboss/lib/fpga/MinipackSystemContainer.h"

namespace {
constexpr uint32_t kPortsPerPim = 16;
using facebook::fboss::MinipackSystemContainer;

inline uint8_t getPimID(int module) {
  return MinipackSystemContainer::kPimStartNum + (module - 1) / kPortsPerPim;
}

inline uint8_t getQsfpPimPort(int module) {
  return (module - 1) % kPortsPerPim;
}

} // namespace

namespace facebook::fboss {
Minipack16QTransceiverApi::Minipack16QTransceiverApi() {
  MinipackSystemContainer::getInstance()->initHW();
}

/* Trigger the QSFP hard reset for a given QSFP module in the minipack chassis
 * switch. For that module, this function getsthe PIM module id, PIM port id
 * and then call FPGA function to do QSFP reset
 */
void Minipack16QTransceiverApi::triggerQsfpHardReset(unsigned int module) {
  auto pimID = getPimID(module);
  auto port = getQsfpPimPort(module);

  MinipackSystemContainer::getInstance()
      ->getPimContainer(pimID)
      ->getPimQsfpController()
      ->triggerQsfpHardReset(port);
}

/* This function will bring all the transceivers out of reset. Just clear the
 * reset bits of all the transceivers through FPGA.
 */
void Minipack16QTransceiverApi::clearAllTransceiverReset() {
  for (auto pimIndex = 0; pimIndex < MinipackSystemContainer::kNumberPim;
       pimIndex++) {
    auto pimID = MinipackSystemContainer::kPimStartNum;
    MinipackSystemContainer::getInstance()
        ->getPimContainer(pimID)
        ->getPimQsfpController()
        ->clearAllTransceiverReset();
  }
}

} // namespace facebook::fboss
