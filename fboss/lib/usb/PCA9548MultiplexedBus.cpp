/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/lib/usb/PCA9548MultiplexedBus.h"

#include <glog/logging.h>

#include "fboss/lib/usb/UsbError.h"

using folly::MutableByteRange;
using std::lock_guard;

namespace facebook::fboss {
void PCA9548MultiplexedBus::initBus() {
  // We step through them all multiplexer, set them
  // all to zero to clear them. Then in selectQsfpImpl,
  // based on port select the multiplexer and again based
  // on port write to address on that multiplexer to select
  // the QSFP.
  for (int offset = 0; offset < numMultiplexers_ * 2; offset += 2) {
    dev_->writeByte(multiplexerStartAddr_ + offset, 0);
  }
}

void PCA9548MultiplexedBus::verifyBus(bool) {
  // TODO(klahey):  Determine appropriate read from SYSCPLD or EEPROM
  //                for verification.
  return;
}

void PCA9548MultiplexedBus::selectQsfpImpl(unsigned int port) {
  // If any of these writes throws, we'll be in an invalid state.

  VLOG(5) << "setting port to " << port << " from " << selectedPort_;
  CHECK_LE(port, numPorts_);
  if (port == NO_PORT) {
    DCHECK_NE(selectedPort_, NO_PORT); // Checked in BaseWedgeI2CBus
    int offset = ((selectedPort_ - 1) / 8) * 2;
    VLOG(4) << "unsetting " << selectedPort_ << " via " << offset;
    dev_->writeByte(multiplexerStartAddr_ + offset, 0);
  } else {
    // Ports are reversed on 32-port hardware, just like 16-port hardware.
    // Each 8 ports are on one I2C multiplexor, so we choose the address;
    // due to the way we do I2C addressing, every multiplexor address is
    // separated by two rather than one.
    int offset = ((port - 1) / 8) * 2;
    int bit = qsfpAddressMap_[(port - 1) % 8];
    if (selectedPort_ != NO_PORT) {
      int oldOffset = ((selectedPort_ - 1) / 8) * 2;
      if (offset != oldOffset) {
        LOG(INFO) << "clearing " << selectedPort_ << " via " << oldOffset;
        dev_->writeByte(multiplexerStartAddr_ + oldOffset, 0);
        selectedPort_ = NO_PORT; // In case the next write throws
      }
    }
    VLOG(4) << "setting " << port << " via " << offset << " bit " << bit;
    dev_->writeByte(multiplexerStartAddr_ + offset, bit);
  }

  selectedPort_ = port;
}

} // namespace facebook::fboss
