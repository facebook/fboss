// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/lib/i2c/I2cDevImpl.h"

namespace facebook::fboss {

class I2cSmbusIo : public I2cDevImpl {
  using I2cDevImpl::I2cDevImpl;

 public:
  void read(uint8_t addr, uint8_t offset, uint8_t* buf, int len) override;
  void write(uint8_t addr, uint8_t offset, const uint8_t* buf, int len)
      override;
};

} // namespace facebook::fboss
