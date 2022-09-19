// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/lib/bsp/gen-cpp2/bsp_platform_mapping_types.h"
#include "fboss/lib/i2c/I2cController.h"
#include "fboss/lib/i2c/I2cDevIo.h"
#include "fboss/lib/usb/TransceiverI2CApi.h"

namespace facebook {
namespace fboss {

class BspTransceiverIOError : public I2cError {
 public:
  explicit BspTransceiverIOError(const std::string& what) : I2cError(what) {}
};

class BspTransceiverIO : public I2cController {
 public:
  BspTransceiverIO(uint32_t tcvr, BspTransceiverMapping& tcvrMapping);
  ~BspTransceiverIO() {}
  void read(const TransceiverAccessParameter& param, uint8_t* buf);
  void write(const TransceiverAccessParameter& param, const uint8_t* buf);

 private:
  std::unique_ptr<I2cDevIo> i2cDev_;
  BspTransceiverMapping tcvrMapping_;
  int tcvrID_;
};

} // namespace fboss
} // namespace facebook
