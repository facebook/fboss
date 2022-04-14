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

#include <string>

namespace facebook::fboss {

class I2cDevIoError : public std::exception {
 public:
  explicit I2cDevIoError(const std::string& what) : what_(what) {}

  const char* what() const noexcept override {
    return what_.c_str();
  }

 private:
  std::string what_;
};

class I2cDevIo {
 public:
  explicit I2cDevIo(const std::string& devName);
  ~I2cDevIo();
  void read(uint8_t addr, uint8_t offset, uint8_t* buf, int len);
  void write(uint8_t addr, uint8_t offset, const uint8_t* buf, int len);

 private:
  std::string devName_;
  int fd_;
};

} // namespace facebook::fboss
