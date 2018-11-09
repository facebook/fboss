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

#include <cstdint>
#include <stdexcept>
#include <string>

namespace facebook { namespace fboss {

class I2cError : public std::exception {
 public:
  I2cError(const std::string& what) : what_(what) {}

  const char* what() const noexcept override {
    return what_.c_str();
  }

 private:
  std::string what_;
};

/*
 * Abstract away some of the details of handling the I2C bus to query
 * QSFP and SFP transceiver modules.
 */
class TransceiverI2CApi {
 public:
  TransceiverI2CApi() {};
  virtual ~TransceiverI2CApi() {}
  virtual void open() = 0;
  virtual void close() = 0;
  virtual void moduleRead(unsigned int module, uint8_t i2cAddress,
                          int offset, int len, uint8_t* buf) = 0;
  virtual void moduleWrite(unsigned int module, uint8_t i2cAddress,
                           int offset, int len, const uint8_t* buf) = 0;

  virtual void verifyBus(bool autoReset) = 0;

  virtual bool isPresent(unsigned int module) {
    uint8_t buf = 0;
    try {
      moduleRead(module, TransceiverI2CApi::ADDR_QSFP,
                                   0, sizeof(buf), &buf);
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

  // Addresses to be queried by external callers:
  enum : uint8_t {
    ADDR_QSFP = 0x50,
  };
};

}} // facebook::fboss
