/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/platforms/wedge/WedgeI2CBusLock.h"
#include "fboss/lib/usb/UsbError.h"

using folly::MutableByteRange;
using std::lock_guard;

namespace facebook { namespace fboss {

WedgeI2CBusLock::WedgeI2CBusLock() {
}

void WedgeI2CBusLock::openLocked() {
  wedgeI2CBus_.open();
  opened_ = true;
}

void WedgeI2CBusLock::open() {
  lock_guard<std::mutex> g(busMutex_);
  openLocked();
}

void WedgeI2CBusLock::closeLocked() {
  wedgeI2CBus_.close();
  opened_ = false;
}

void WedgeI2CBusLock::close() {
  lock_guard<std::mutex> g(busMutex_);
  closeLocked();
}

void WedgeI2CBusLock::moduleRead(unsigned int module, uint8_t address,
                             int offset, int len, uint8_t *buf) {
  lock_guard<std::mutex> g(busMutex_);

  // If no one has opened the device, open it, and close it when we
  // leave.  If it has already been opened (say, when we batch a big
  // set of reads), then don't bother to open it.
  bool opened = false;

  SCOPE_EXIT {
    if (opened) {
      closeLocked();
    }
  };

  if (!opened_) {
    openLocked();
    opened = true;
  }

  wedgeI2CBus_.moduleRead(module, address, offset, len, buf);
}

void WedgeI2CBusLock::moduleWrite(unsigned int module, uint8_t address,
                              int offset, int len, uint8_t *buf) {
  lock_guard<std::mutex> g(busMutex_);

  // If no one has opened the device, open it, and close it when we
  // leave.  If it has already been opened (say, when we batch a big
  // set of reads), then don't bother to open it.
  bool opened = false;

  SCOPE_EXIT {
    if (opened) {
      closeLocked();
    }
  };

  if (!opened_) {
    openLocked();
    opened = true;
  }

  wedgeI2CBus_.moduleWrite(module, address, offset, len, buf);
}

}} // facebook::fboss
