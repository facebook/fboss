/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/lib/fpga/Wedge400TransceiverApi.h"
#include "fboss/agent/Utils.h"
#include "fboss/lib/fpga/Wedge400Fpga.h"

namespace facebook::fboss {
/* Trigger the QSFP hard reset for a given QSFP module in the minipack chassis
 * switch. For that module, this function getsthe PIM module id, PIM port id
 * and then call FPGA function to do QSFP reset
 */
void Wedge400TransceiverApi::triggerQsfpHardReset(unsigned int module) {
  // The TransceiverApi currently gets a 1 based module index where the FPGA is
  // functioning with 0 based index. So we apply the offset here.
  Wedge400Fpga::getInstance()->triggerQsfpHardReset(TransceiverID(module - 1));
}

void Wedge400TransceiverApi::clearAllTransceiverReset() {
  Wedge400Fpga::getInstance()->clearAllTransceiverReset();
}
} // namespace facebook::fboss
