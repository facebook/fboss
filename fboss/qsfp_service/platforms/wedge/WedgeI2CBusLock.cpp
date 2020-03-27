/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/qsfp_service/platforms/wedge/WedgeI2CBusLock.h"
#include "fboss/lib/usb/UsbError.h"

#include "fboss/qsfp_service/StatsPublisher.h"

using folly::MutableByteRange;
using std::lock_guard;

namespace facebook { namespace fboss {

WedgeI2CBusLock::WedgeI2CBusLock(std::unique_ptr<BaseWedgeI2CBus> wedgeI2CBus)
    : wedgeI2CBus_(std::move(wedgeI2CBus)) {}

void WedgeI2CBusLock::openLocked() {
  StatsPublisher::bumpPciLockHeld();
  wedgeI2CBus_->open();
  opened_ = true;
}

void WedgeI2CBusLock::open() {
  lock_guard<std::mutex> g(busMutex_);
  openLocked();
}

void WedgeI2CBusLock::closeLocked() {
  wedgeI2CBus_->close();
  opened_ = false;
}

void WedgeI2CBusLock::close() {
  lock_guard<std::mutex> g(busMutex_);
  closeLocked();
}

void WedgeI2CBusLock::verifyBus(bool autoReset) {
  BusGuard g(this);
  wedgeI2CBus_->verifyBus(autoReset);
}

void WedgeI2CBusLock::moduleRead(unsigned int module, uint8_t address,
                             int offset, int len, uint8_t *buf) {
  BusGuard g(this);
  wedgeI2CBus_->moduleRead(module, address, offset, len, buf);
}

void WedgeI2CBusLock::moduleWrite(unsigned int module, uint8_t address,
                              int offset, int len, const uint8_t *buf) {
  BusGuard g(this);
  wedgeI2CBus_->moduleWrite(module, address, offset, len, buf);
}

void WedgeI2CBusLock::read(uint8_t address, int offset,
                           int len, uint8_t *buf) {
  BusGuard g(this);
  wedgeI2CBus_->read(address, offset, len, buf);
}

void WedgeI2CBusLock::write(uint8_t address, int offset,
                            int len, const uint8_t *buf) {
  BusGuard g(this);
  wedgeI2CBus_->write(address, offset, len, buf);
}

bool WedgeI2CBusLock::isPresent(unsigned int module) {
  BusGuard g(this);
  return wedgeI2CBus_->isPresent(module);
}

void WedgeI2CBusLock::scanPresence(
    std::map<int32_t, ModulePresence>& presence) {
  BusGuard g(this);
  wedgeI2CBus_->scanPresence(presence);
}

void WedgeI2CBusLock::ensureOutOfReset(unsigned int module) {
  BusGuard g(this);
  wedgeI2CBus_->ensureOutOfReset(module);
}

/* Platform function to count the i2c transactions in a platform. This
 * function gets the i2c controller stats and returns it in form of a vector
 * to the caller
 */
std::vector<std::reference_wrapper<const I2cControllerStats>>
WedgeI2CBusLock::getI2cControllerStats() {
  BusGuard g(this);
  return wedgeI2CBus_->getI2cControllerStats();
}

folly::EventBase* WedgeI2CBusLock::getEventBase(unsigned int module) {
  return wedgeI2CBus_->getEventBase(module);
}
}} // facebook::fboss
