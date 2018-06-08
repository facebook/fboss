/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "WedgeQsfp.h"
#include "fboss/lib/usb/UsbError.h"
#include <folly/Conv.h>
#include <folly/Memory.h>

#include <folly/logging/xlog.h>
#include "fboss/qsfp_service/StatsPublisher.h"

using namespace facebook::fboss;
using folly::MutableByteRange;
using std::make_unique;
using folly::StringPiece;
using std::unique_ptr;

namespace facebook { namespace fboss {

WedgeQsfp::WedgeQsfp(int module, WedgeI2CBusLock* wedgeI2CBus)
  : module_(module),
    wedgeI2CBusLock_(wedgeI2CBus) {
  moduleName_ = folly::to<std::string>(module);
}

WedgeQsfp::~WedgeQsfp() {
}

// Note that the module_ starts at 0, but the USB I2C bus module
// assumes that QSFP module numbers extend from 1 to 16.
//
bool WedgeQsfp::detectTransceiver() {
  uint8_t buf[1];
  try {
    wedgeI2CBusLock_->moduleRead(module_ + 1, TransceiverI2CApi::ADDR_QSFP,
                                 0, sizeof(buf), buf);
  } catch (const UsbError& ex) {
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

int WedgeQsfp::readTransceiver(int dataAddress, int offset,
                               int len, uint8_t* fieldValue) {
  try {
    wedgeI2CBusLock_->moduleRead(module_ + 1, dataAddress, offset, len,
                                  fieldValue);
  } catch (const UsbError& ex) {
    XLOG(ERR) << "Read from transceiver " << module_ << " at offset " << offset
              << " with length " << len << " failed: " << ex.what();
    StatsPublisher::bumpReadFailure();
    throw;
  }
  return len;
}

int WedgeQsfp::writeTransceiver(int dataAddress, int offset,
                            int len, uint8_t* fieldValue) {
  try {
    wedgeI2CBusLock_->moduleWrite(module_ + 1, dataAddress, offset, len,
                                   fieldValue);
  } catch (const UsbError& ex) {
    XLOG(ERR) << "Write to transceiver " << module_ << " at offset " << offset
              << " with length " << len
              << " failed: " << folly::exceptionStr(ex);
    StatsPublisher::bumpWriteFailure();
    throw;
  }
  return len;
}

folly::StringPiece WedgeQsfp::getName() {
  return moduleName_;
}

int WedgeQsfp::getNum() const {
  return module_;
}

}}
