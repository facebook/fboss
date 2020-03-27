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

#include "fboss/lib/usb/BaseWedgeI2CBus.h"

#include <mutex>
#include <folly/Range.h>

namespace facebook { namespace fboss {

/*
 * A small wrapper around CP2112 which is aware of the topology of wedge's QSFP
 * I2C bus, and can select specific QSFPs to query.
 */
class WedgeI2CBusLock : public TransceiverI2CApi {
 public:
  explicit WedgeI2CBusLock(std::unique_ptr<BaseWedgeI2CBus> wedgeI2CBus);
  void open() override;
  void close() override;
  void moduleRead(unsigned int module, uint8_t i2cAddress,
                  int offset, int len, uint8_t* buf) override;
  void moduleWrite(unsigned int module, uint8_t i2cAddress,
                  int offset, int len, const uint8_t* buf) override;
  void read(uint8_t i2cAddress, int offset, int len, uint8_t* buf);
  void write(uint8_t i2cAddress, int offset, int len, const uint8_t* buf);

  void verifyBus(bool autoReset) override;
  bool isPresent(unsigned int module) override;
  void scanPresence(std::map<int32_t, ModulePresence>& presence) override;
  void ensureOutOfReset(unsigned int module) override;

  /* Platform function to count the i2c transactions in a platform. This
   * function gets the i2c controller stats and returns it in form of a vector
   * to the caller
   */
  std::vector<std::reference_wrapper<const I2cControllerStats>>
  getI2cControllerStats() override;

  folly::EventBase* getEventBase(unsigned int module) override;

 private:
  // Forbidden copy constructor and assignment operator
  WedgeI2CBusLock(WedgeI2CBusLock const &) = delete;
  WedgeI2CBusLock& operator=(WedgeI2CBusLock const &) = delete;

  void openLocked();
  void closeLocked();

  std::unique_ptr<BaseWedgeI2CBus> wedgeI2CBus_{nullptr};
  mutable std::mutex busMutex_;
  bool opened_{false};

  class BusGuard {
    /* This class is a simple guard that:
       1. locks access to the device
       2. opens/closes the device if it is not already open

       This makes sure that only one person is accessing the device,
       but allows us to only open the device once in the case of batch
       read/writes.
    */
   public:
    explicit BusGuard(WedgeI2CBusLock* busLock) :
        busLock_(busLock),
        lock_(busLock->busMutex_)
      {
        if (!busLock_->opened_) {
          busLock_->openLocked();
          performedOpen_ = true;
        }
      }

    ~BusGuard() {
      if (performedOpen_) {
        busLock_->closeLocked();
      }
    }

   private:
    WedgeI2CBusLock* busLock_{nullptr};
    bool performedOpen_{false};
    std::lock_guard<std::mutex> lock_;
  };
};

}} // facebook::fboss
