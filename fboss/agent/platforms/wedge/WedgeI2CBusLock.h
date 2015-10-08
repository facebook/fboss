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

#include "fboss/lib/usb/WedgeI2CBus.h"

#include <mutex>
#include <folly/Range.h>

namespace facebook { namespace fboss {

/*
 * A small wrapper around CP2112 which is aware of the topology of wedge's QSFP
 * I2C bus, and can select specific QSFPs to query.
 */
class WedgeI2CBusLock {
 public:
  WedgeI2CBusLock();
  void open();
  void close();
  void moduleRead(unsigned int module, uint8_t i2cAddress,
                  int offset, int len, uint8_t* buf);
  void moduleWrite(unsigned int module, uint8_t i2cAddress,
                  int offset, int len, uint8_t* buf);

 private:

  // Forbidden copy constructor and assignment operator
  WedgeI2CBusLock(WedgeI2CBusLock const &) = delete;
  WedgeI2CBusLock& operator=(WedgeI2CBusLock const &) = delete;

  void openLocked();
  void closeLocked();

  WedgeI2CBus wedgeI2CBus_;
  mutable std::mutex busMutex_;
  bool opened_{false};
};

}} // facebook::fboss
