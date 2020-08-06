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

#include <folly/io/async/EventBase.h>
#include "fboss/lib/i2c/gen-cpp2/i2c_controller_stats_types.h"

#include <cstdint>
#include <map>
#include <stdexcept>
#include <string>

namespace facebook::fboss {
enum class ModulePresence { PRESENT, ABSENT, UNKNOWN };

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
  TransceiverI2CApi(){};
  virtual ~TransceiverI2CApi() {}

  virtual void open() = 0;
  virtual void close() = 0;
  virtual void moduleRead(
      unsigned int module,
      uint8_t i2cAddress,
      int offset,
      int len,
      uint8_t* buf) = 0;

  virtual void moduleWrite(
      unsigned int module,
      uint8_t i2cAddress,
      int offset,
      int len,
      const uint8_t* buf) = 0;

  virtual void verifyBus(bool autoReset) = 0;

  virtual bool isPresent(unsigned int module) = 0;
  /*
   * Function will detect transceiver presence according to the indexes provided
   */
  virtual void scanPresence(std::map<int32_t, ModulePresence>& presences) = 0;

  /*
   * Function bring transceiver out of reset whenever a transceiver has been
   * detected plugging in.
   */
  virtual void ensureOutOfReset(unsigned int module){};

  /* This function does a hard reset to the QSFP through I2C/CPLD interface
   * in the given platform. This is a virtual function at this level and it
   * will be overridden by appropriate platform class like wedge100I2CBus etc.
   */
  virtual void triggerQsfpHardReset(unsigned int module){};

  /* This function will bring all the transceivers out of reset. This is a
   * virtual function at this level and it will be overridden by appropriate
   * platform class. Some platforms clear transceiver from reset by default.
   * So function we will stay no op for those platforms.
   */
  virtual void clearAllTransceiverReset(){};

  /*
   * Function that returns the eventbase that suppose to execute the I2C txn
   * associated with the module. At this moment, only Minipack and Yamp which
   * are the models using FPGA has the ability of doing I2C txn parallelly.
   * Other models that has single I2C bus will return nullptr by default which
   * means no relevant eventbase.
   */
  virtual folly::EventBase* getEventBase(unsigned int module) {
    return nullptr;
  };

  /* Virtual function to count the i2c transactions in a platform. This
   * will be overridden by derived classes which are platform specific
   * and has the platform specific implementation for this counter
   */
  virtual std::vector<std::reference_wrapper<const I2cControllerStats>>
  getI2cControllerStats() {
    std::vector<std::reference_wrapper<const I2cControllerStats>> dummyStat;
    return dummyStat;
  }

  // Addresses to be queried by external callers:
  enum : uint8_t {
    ADDR_QSFP = 0x50,
  };
};

} // namespace facebook::fboss
