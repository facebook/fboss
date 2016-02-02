/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/lib/usb/Wedge100I2CBus.h"

#include "fboss/lib/usb/UsbError.h"

using folly::MutableByteRange;
using std::lock_guard;

namespace facebook { namespace fboss {

Wedge100I2CBus::Wedge100I2CBus() {
}

void Wedge100I2CBus::initBus() {
  for (int offset = 0; offset <= MULTIPLEXERS * 2; offset += 2) {
    dev_.writeByte(ADDR_SWITCH_32 + offset, 0);
  }
}

void Wedge100I2CBus::verifyBus(bool) {
  // TODO(klahey):  Determine appropriate read from SYSCPLD or EEPROM
  //                for verification.
  return;
}

static constexpr unsigned char bitmap[]= {
  0x2, 0x1, 0x8, 0x4, 0x20, 0x10, 0x80, 0x40,
};

void Wedge100I2CBus::selectQsfpImpl(unsigned int port) {
  // There are four multiplexers; we step through them, setting them
  // all to zero to clear them, then setting the one that needs to be set.
  // If any of these writes throws, we'll be in an invalid state.

  VLOG(5) << "setting port to " << port << " from " << selectedPort_;
  CHECK_LE(port, 32);
  if (port == NO_PORT) {
    DCHECK_NE(selectedPort_, NO_PORT);  // Checked in BaseWedgeI2CBus
    int offset = ((selectedPort_ - 1) / 8) * 2;
    VLOG(4) << "unsetting " << selectedPort_ << " via " << offset;
    dev_.writeByte(ADDR_SWITCH_32 + offset, 0);
  } else {
    // Ports are reversed on 32-port hardware, just like 16-port hardware.
    // Each 8 ports are on one I2C multiplexor, so we choose the address;
    // due to the way we do I2C addressing, every multiplexor address is
    // separated by two rather than one.
    int offset = ((port - 1) / 8) * 2;
    int bit = bitmap[(port - 1) % 8];
    if (selectedPort_ != NO_PORT) {
      int oldOffset = ((selectedPort_ - 1) / 8) * 2;
      if (offset != oldOffset) {
        VLOG(4) << "clearing " << selectedPort_ << " via " << oldOffset;
        dev_.writeByte(ADDR_SWITCH_32 + oldOffset, 0);
        selectedPort_ = NO_PORT;  // In case the next write throws
      }
    }
    VLOG(4) << "setting " << port << " via " << offset << " bit " << bit;
    dev_.writeByte(ADDR_SWITCH_32 + offset, bit);
  }

  selectedPort_ = port;
}

}} // facebook::fboss
