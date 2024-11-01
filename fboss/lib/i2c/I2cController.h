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

#include <folly/Synchronized.h>
#include <stdint.h>
#include "fboss/lib/i2c/gen-cpp2/i2c_controller_stats_types.h"

namespace facebook::fboss {

/* This is the base class for i2c controllers.
 */
class I2cController {
 public:
  explicit I2cController(std::string name) {
    *i2cControllerPlatformStats_.wlock()->controllerName_() = name;
  }
  virtual ~I2cController() {}

  // Reset all i2c stats
  void resetStats() {
    auto stats = i2cControllerPlatformStats_.wlock();
    *stats->readTotal_() = 0;
    *stats->readFailed_() = 0;
    *stats->readBytes_() = 0;
    *stats->writeTotal_() = 0;
    *stats->writeFailed_() = 0;
    *stats->writeBytes_() = 0;
  }
  // Total number of reads
  void incrReadTotal(uint32_t count = 1) {
    *i2cControllerPlatformStats_.wlock()->readTotal_() += count;
  }
  // Total Read failures
  void incrReadFailed(uint32_t count = 1) {
    *i2cControllerPlatformStats_.wlock()->readFailed_() += count;
  }
  // Number of bytes read
  void incrReadBytes(uint32_t count = 1) {
    *i2cControllerPlatformStats_.wlock()->readBytes_() += count;
  }
  // Total number of writes
  void incrWriteTotal(uint32_t count = 1) {
    *i2cControllerPlatformStats_.wlock()->writeTotal_() += count;
  }
  // Total write failures
  void incrWriteFailed(uint32_t count = 1) {
    *i2cControllerPlatformStats_.wlock()->writeFailed_() += count;
  }
  // Number of bytes written
  void incrWriteBytes(uint32_t count = 1) {
    *i2cControllerPlatformStats_.wlock()->writeBytes_() += count;
  }

  /* Get the I2c transaction stats from the i2c controller
   */
  const I2cControllerStats getI2cControllerPlatformStats() const {
    // return the structre reference of latest i2c transaction stats
    return *i2cControllerPlatformStats_.rlock();
  }

  virtual void i2cTimeProfilingStart() {}
  virtual void i2cTimeProfilingEnd() {}
  virtual std::pair<uint64_t, uint64_t> getI2cTimeProfileMsec() {
    return std::make_pair(0, 0);
  }

 private:
  // Platform i2c controller stats
  folly::Synchronized<I2cControllerStats> i2cControllerPlatformStats_;
};

} // namespace facebook::fboss
