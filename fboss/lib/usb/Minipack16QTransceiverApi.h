/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#pragma once

#include "fboss/lib/usb/TransceiverPlatformApi.h"

namespace facebook::fboss {
/* This is the class for Minipack FPGA related API. This is platform specific
 * class and inherits generic class TransceiverPlatformApi
 */
class Minipack16QTransceiverApi : public TransceiverPlatformApi {
 public:
  Minipack16QTransceiverApi() {}
  ~Minipack16QTransceiverApi() override {}

  /* Trigger the QSFP hard reset for a given QSFP module in the minipack chassis
   * switch. For that module, this function getsthe PIM module id, PIM port id
   * and then call FPGA function to do QSFP reset
   */
  void triggerQsfpHardReset(unsigned int module) override;

  /* This function will bring all the transceivers out of reset. Just clear the
   * reset bits of all the transceivers through FPGA.
   */
  void clearAllTransceiverReset() override;
};

} // namespace facebook::fboss
