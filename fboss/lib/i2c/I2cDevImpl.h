// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include <string>
#include "fboss/lib/i2c/I2cDevError.h"

namespace facebook::fboss {

class I2cDevImpl {
 public:
  I2cDevImpl(const std::string& devName, int fd) : devName_(devName), fd_(fd) {}
  virtual ~I2cDevImpl() {}

  virtual void read(uint8_t addr, uint8_t offset, uint8_t* buf, int len) = 0;
  virtual void
  write(uint8_t addr, uint8_t offset, const uint8_t* buf, int len) = 0;

  const std::string& devName() {
    return devName_;
  }

  int fd() {
    return fd_;
  }

 private:
  std::string devName_;
  int fd_;
};

} // namespace facebook::fboss
