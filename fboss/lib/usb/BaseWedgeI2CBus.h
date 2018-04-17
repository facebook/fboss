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

#include "fboss/lib/usb/TransceiverI2CApi.h"
#include "fboss/lib/usb/CP2112.h"

#include <mutex>
#include <folly/Range.h>

namespace facebook { namespace fboss {

/*
 * A small wrapper around CP2112 which is aware of the topology of wedge's QSFP
 * I2C bus, and can select specific QSFPs to query.
 */
class BaseWedgeI2CBus : public TransceiverI2CApi {

 public:
  explicit BaseWedgeI2CBus(std::unique_ptr<CP2112Intf> dev = nullptr) {
    dev_ = (dev) ? std::move(dev) : std::make_unique<CP2112>();
  }
  ~BaseWedgeI2CBus() override {}
  void open() override;
  void close() override;
  void moduleRead(
      unsigned int module,
      uint8_t i2cAddress,
      int offset,
      int len,
      uint8_t* buf) override;
  void moduleWrite(
      unsigned int module,
      uint8_t i2cAddress,
      int offset,
      int len,
      const uint8_t* buf) override;
  void read(uint8_t i2cAddress, int offset, int len, uint8_t* buf);
  void write(uint8_t i2cAddress, int offset, int len, const uint8_t* buf);

 protected:
  enum : unsigned int {
    NO_PORT = 0,
  };

  virtual void initBus() = 0;
  virtual void verifyBus(bool autoReset = true) = 0;
  virtual void selectQsfpImpl(unsigned int module) = 0;

  std::unique_ptr<CP2112Intf> dev_;
  unsigned int selectedPort_{NO_PORT};

 private:
  /*
   * Set the PCA9548 switches so that we can read from the selected QSFP
   * module.
   */
  void selectQsfp(unsigned int module);
  void unselectQsfp();

  // Forbidden copy constructor and assignment operator
  BaseWedgeI2CBus(BaseWedgeI2CBus const &) = delete;
  BaseWedgeI2CBus& operator=(BaseWedgeI2CBus const &) = delete;
};

}} // facebook::fboss
