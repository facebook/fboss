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

#include <stdint.h>
#include "fboss/lib/i2c/gen-cpp2/i2c_controller_stats_types.h"

namespace facebook::fboss {

/* This is the base class for i2c controllers.
 */
class I2cController {
 public:
  I2cController(std::string name) {
    *i2cControllerPlatformStats_.controllerName__ref() = name;
  }
  ~I2cController() {}

  // Reset all i2c stats
  void resetStats() {
    *i2cControllerPlatformStats_.readTotal__ref() = 0;
    *i2cControllerPlatformStats_.readFailed__ref() = 0;
    *i2cControllerPlatformStats_.readBytes__ref() = 0;
    *i2cControllerPlatformStats_.writeTotal__ref() = 0;
    *i2cControllerPlatformStats_.writeFailed__ref() = 0;
    *i2cControllerPlatformStats_.writeBytes__ref() = 0;
  }
  // Total number of reads
  void incrReadTotal(uint32_t count = 1) {
    *i2cControllerPlatformStats_.readTotal__ref() += count;
  }
  // Total Read failures
  void incrReadFailed(uint32_t count = 1) {
    *i2cControllerPlatformStats_.readFailed__ref() += count;
  }
  // Number of bytes read
  void incrReadBytes(uint32_t count = 1) {
    *i2cControllerPlatformStats_.readBytes__ref() += count;
  }
  // Total number of writes
  void incrWriteTotal(uint32_t count = 1) {
    *i2cControllerPlatformStats_.writeTotal__ref() += count;
  }
  // Total write failures
  void incrWriteFailed(uint32_t count = 1) {
    *i2cControllerPlatformStats_.writeFailed__ref() += count;
  }
  // Number of bytes written
  void incrWriteBytes(uint32_t count = 1) {
    *i2cControllerPlatformStats_.writeBytes__ref() += count;
  }

  /* Get the I2c transaction stats from the i2c controller
   */
  const I2cControllerStats& getI2cControllerPlatformStats() const {
    // return the structre reference of latest i2c transaction stats
    return i2cControllerPlatformStats_;
  }

 private:
  // Platform i2c controller stats
  I2cControllerStats i2cControllerPlatformStats_;
};

} // namespace facebook::fboss
