/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/lib/usb/BaseWedgeI2CBus.h"

#include <glog/logging.h>

#include "fboss/lib/usb/UsbError.h"

using folly::MutableByteRange;
using std::lock_guard;

namespace facebook::fboss {
void BaseWedgeI2CBus::open() {
  dev_->open();

  selectedPort_ = NO_PORT;
  verifyBus(true);
  initBus();

  VLOG(4) << "successfully opened wedge CP2112 I2C bus";
}

void BaseWedgeI2CBus::close() {
  dev_->close();
}

void BaseWedgeI2CBus::read(uint8_t address, int offset, int len, uint8_t* buf) {
  CHECK_LE(offset, 255);

  // Before accessing the CP2112 device, check if the device is opened on USB
  if (!dev_->isOpen()) {
    VLOG(3) << "The Wedge CP2112 I2c bus not opened";
    return;
  }

  // CP2112 uses addresses in the on-the-wire format, while we generally
  // pass them around in Linux standard format.
  address <<= 1;

  // Note that we don't use the writeRead() command by default, since this
  // locks up the CP2112 chip if it times out.  We perform a separate write,
  // followed by a read.  This releases the I2C bus between operations, but
  // that's okay since there aren't any other master devices on the bus.

  // Also note that we can't read more than 128 bytes at a time.
  if (writeReadMode_ == WriteReadMode::WriteReadModeStopStart) {
    // Default addressed read happens in the STOP_START mode. In this mode
    // the "offset" is written to the first address and then we read
    // from the given address.
    dev_->writeByte(address, offset);
    if (len > 128) {
      dev_->read(address, MutableByteRange(buf, 128));
      dev_->writeByte(address, offset + 128);
      dev_->read(address, MutableByteRange(buf + 128, len - 128));
    } else {
      dev_->read(address, MutableByteRange(buf, len));
    }
  } else {
    // This is no default mode of reading and it is REPEATED_START mode. The
    // addressed read is invoked from the CP2112 device. This is the
    // sequence from device (CPAS for cp2112 to slave direction and small
    // for slave to cp2112 direction):
    // [START, SLAVE_ADDRESS, WRITE, ack, DATA_ADDRESS, ack, REPEATED_START,
    //  SLAVE_ADDRESS, READ, ack, data, STOP]
    // We have seen in the past that when timeout happens during this
    // operation then the chip cp2112 locks up so this is not the default
    // mode of reading
    uint8_t targetOffset = offset;

    // It appears that we can't read more than 128 bytes at a time so in
    // case the read length is more than 128 then we need to split the read
    // into two.
    if (len > 128) {
      // Call device writeReadUnsafe for 0..127
      dev_->writeReadUnsafe(
          address,
          folly::ByteRange(&targetOffset, 1),
          MutableByteRange(buf, 128));

      targetOffset += 128;
      // Call device writeReadUnsafe for 128..len-1
      dev_->writeReadUnsafe(
          address,
          folly::ByteRange(&targetOffset, 1),
          MutableByteRange(buf + 128, len - 128));

    } else {
      // Call cp2112 writeReadUnsafe for 0..len-1
      dev_->writeReadUnsafe(
          address,
          folly::ByteRange(&targetOffset, 1),
          MutableByteRange(buf, len));
    }
  }
}

void BaseWedgeI2CBus::write(
    uint8_t address,
    int offset,
    int len,
    const uint8_t* buf) {
  CHECK_LE(offset, 255);

  // Before accessing the CP2112 device, check if the device is opened on USB
  if (!dev_->isOpen()) {
    VLOG(3) << "The Wedge CP2112 I2c bus not opened";
    return;
  }

  // CP2112 uses addresses in the on-the-wire format, while we generally
  // pass them around in Linux standard format.
  address <<= 1;

  // The CP2112 can only write 61 bytes at a time, and we burn one for
  // the offset
  CHECK_LE(len, 60);

  // XXX:  surely there's an easier way to do this?
  uint8_t output[61]; // USB buffer size;
  output[0] = offset;
  memcpy(output + 1, buf, len);
  dev_->write(address, MutableByteRange(output, len + 1));
}

void BaseWedgeI2CBus::moduleRead(
    unsigned int module,
    uint8_t address,
    int offset,
    int len,
    uint8_t* buf) {
  selectQsfp(module);
  CHECK_NE(selectedPort_, NO_PORT);

  read(address, offset, len, buf);

  // TODO: remove this after we ensure exclusive access to cp2112 chip
  unselectQsfp();
}

void BaseWedgeI2CBus::moduleWrite(
    unsigned int module,
    uint8_t address,
    int offset,
    int len,
    const uint8_t* buf) {
  selectQsfp(module);
  CHECK_NE(selectedPort_, NO_PORT);

  write(address, offset, len, buf);

  // TODO: remove this after we ensure exclusive access to cp2112 chip
  unselectQsfp();
}

bool BaseWedgeI2CBus::isPresent(unsigned int module) {
  uint8_t buf = 0;
  try {
    moduleRead(module, TransceiverI2CApi::ADDR_QSFP, 0, sizeof(buf), &buf);
  } catch (const I2cError& ex) {
    /*
     * This can either mean that we failed to open the USB device
     * because it was already in use, or that the I2C read failed.
     * At some point we might want to return more a more accurate
     * status value to higher-level functions.
     */
    return false;
  }
  return true;
}

void BaseWedgeI2CBus::scanPresence(
    std::map<int32_t, ModulePresence>& presences) {
  for (auto& presence : presences) {
    uint8_t buf = 0;
    try {
      moduleRead(
          presence.first + 1,
          TransceiverI2CApi::ADDR_QSFP,
          0,
          sizeof(buf),
          &buf);
    } catch (const I2cError& ex) {
      /*
       * This can either mean that we failed to open the USB device
       * because it was already in use, or that the I2C read failed.
       * At some point we might want to return more a more accurate
       * status value to higher-level functions.
       */
      presence.second = ModulePresence::ABSENT;
      continue;
    }
    presence.second = ModulePresence::PRESENT;
  }
}

void BaseWedgeI2CBus::selectQsfp(unsigned int port) {
  VLOG(4) << "selecting QSFP " << port;
  CHECK_GT(port, 0);
  if (port != selectedPort_) {
    selectQsfpImpl(port);
  }
}

void BaseWedgeI2CBus::unselectQsfp() {
  VLOG(4) << "unselecting all QSFPs";
  if (selectedPort_ != NO_PORT) {
    selectQsfpImpl(NO_PORT);
  }
}

} // namespace facebook::fboss
