/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/lib/usb/WedgeI2CBus.h"

#include <glog/logging.h>

#include "fboss/lib/usb/UsbError.h"

using folly::MutableByteRange;
using std::lock_guard;

namespace facebook::fboss {
WedgeI2CBus::WedgeI2CBus() {}

void WedgeI2CBus::initBus() {
  dev_->writeByte(ADDR_SWITCH_1, 0);
  dev_->writeByte(ADDR_SWITCH_2, 0);
}

void WedgeI2CBus::verifyBus(bool autoReset) {
  // Make sure the I2C bus is functioning, and reset the CP2112 device if not.
  //
  // Immediately after wedge is powered on, the CP2112 device appears to be in
  // a state where all I2C operations time out.  (It doesn't actually signal
  // anything on the I2C bus.)  Resetting the chip fixes it.
  //
  // We have an AT24C64B EEPROM attached at address 0xa2 which we should always
  // be able to read from successfully.
  uint8_t tmpBuf[8];
  try {
    dev_->read(ADDR_EEPROM, MutableByteRange(tmpBuf, sizeof(tmpBuf)));
  } catch (const UsbError& ex) {
    // The read failed.
    // Reset the device, and then confirm that we can read this time.
    //
    // Note: This can be slightly dangerous for other reasons.  We have seen
    // some 3M QSFP optics that can lock up and hold SDA low forever.  If we
    // reset the CP2112 chip with SDA held low it can't reset successfully,
    // and disappears from the USB bus completely.  (It never re-enumerates.)
    // If the bus is in this state resetting the chip will make it completely
    // inaccessible.
    if (autoReset) {
      try {
        dev_->resetDevice();
      } catch (const UsbDeviceResetError& ex2) {
      }
    } else {
      VLOG(1) << "initial read from CP2112 failed; I2C bus appears hung";
      throw;
    }
  }
}

void WedgeI2CBus::selectQsfpImpl(unsigned int port) {
  auto oldValues = getSwitchValues(selectedPort_);
  auto newValues = getSwitchValues(port);

  // One of the new values should be 0.  Set this one first,
  // to ensure that we never have more than one port selected at a time.
  if (newValues.first == 0) {
    if (newValues.first != oldValues.first) {
      dev_->writeByte(ADDR_SWITCH_1, newValues.first);
    }
    selectedPort_ = NO_PORT; // In case the second write throws
    if (newValues.second != oldValues.second) {
      dev_->writeByte(ADDR_SWITCH_2, newValues.second);
    }
  } else {
    if (newValues.second != oldValues.second) {
      dev_->writeByte(ADDR_SWITCH_2, newValues.second);
    }
    selectedPort_ = NO_PORT; // In case the second write throws
    if (newValues.first != oldValues.first) {
      dev_->writeByte(ADDR_SWITCH_1, newValues.first);
    }
  }
  selectedPort_ = port;
}

std::pair<uint8_t, uint8_t> WedgeI2CBus::getSwitchValues(
    unsigned int port) const {
  // We swapped the top and bottom port numbering after the board was
  // taped out, so each pair has to be swapped here.
  static const std::pair<uint8_t, uint8_t> map[] = {
      {0x00, 0x00}, // NO_PORT
      {0x02, 0x00}, // Module 1 (left-most top module)
      {0x01, 0x00}, // Module 2 (left-most bottom module)
      {0x08, 0x00}, // Module 3
      {0x04, 0x00}, // Module 4
      {0x20, 0x00}, // Module 5
      {0x10, 0x00}, // Module 6
      {0x80, 0x00}, // Module 7
      {0x40, 0x00}, // Module 8
      {0x00, 0x02}, // Module 9
      {0x00, 0x01}, // Module 10
      {0x00, 0x08}, // Module 11
      {0x00, 0x04}, // Module 12
      {0x00, 0x20}, // Module 13
      {0x00, 0x10}, // Module 14
      {0x00, 0x80}, // Module 15 (right-most top module)
      {0x00, 0x40}, // Module 16 (right-most bottom module)
  };

  CHECK_LE(port, 16);
  return map[port];
}

} // namespace facebook::fboss
