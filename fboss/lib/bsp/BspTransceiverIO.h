// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include <chrono>
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
  virtual ~BspTransceiverIO() {}
  void read(const TransceiverAccessParameter& param, uint8_t* buf);
  void write(const TransceiverAccessParameter& param, const uint8_t* buf);

  void i2cTimeProfilingStart() override;
  void i2cTimeProfilingEnd() override;
  std::pair<uint64_t, uint64_t> getI2cTimeProfileMsec() override;

 private:
  std::unique_ptr<I2cDevIo> i2cDev_;
  BspTransceiverMapping tcvrMapping_;
  int tcvrID_;
  bool ioRdWrProfilingInProgress_{false};
  std::chrono::duration<uint32_t, std::milli> ioWriteProfilingTime_{0};
  std::chrono::duration<uint32_t, std::milli> ioReadProfilingTime_{0};
};

} // namespace fboss
} // namespace facebook
