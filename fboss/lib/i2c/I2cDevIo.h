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

#include <memory>
#include <optional>
#include <string>
#include "fboss/lib/i2c/I2cDevImpl.h"

namespace facebook::fboss {

enum I2cIoType {
  I2cIoTypeUnknown,
  I2cIoTypeRdWr,
  I2cIoTypeSmbus,
  I2cIoTypeForTest,
};

class I2cDevIo {
 public:
  explicit I2cDevIo(
      const std::string& devName,
      std::optional<I2cIoType> ioType = std::nullopt);

  virtual ~I2cDevIo();
  virtual void read(uint8_t addr, uint8_t offset, uint8_t* buf, int len);
  virtual void write(uint8_t addr, uint8_t offset, const uint8_t* buf, int len);
  I2cIoType getIoType();

 protected:
  int getFileHandle() {
    return fd_;
  }

 private:
  void openFile();
  void createI2cDevImpl(std::optional<I2cIoType> ioType);
  std::string devName_;
  int fd_;
  std::unique_ptr<I2cDevImpl> i2cDevImpl_;
};

} // namespace facebook::fboss
