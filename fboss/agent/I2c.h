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
#include <folly/File.h>

namespace facebook { namespace fboss {

class I2cDevice {
 public:
  enum : int {
    I2C_BLOCK_SIZE = 32,
    I2C_WORD_SIZE = 2,
  };
  I2cDevice(int i2cBus, uint32_t address, bool slaveForce);

  void read(int offset, int length, uint8_t fieldValue[]);
  uint8_t readByte(int offset);
  uint16_t readWord(int offset);
  int readBlock(int offset, uint8_t fieldValue[]);
 private:
  int fd();

  int i2cBus_{0};
  std::string devName;
  folly::File file_;
  // Forbidden copy contructor and assignment operator
  I2cDevice(I2cDevice const &) = delete;
  I2cDevice& operator=(I2cDevice const &) = delete;
};

}} // namespace facebook::fboss
