// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include "fboss/lib/fpga/FbDomFpga.h"
#include "fboss/lib/i2c/I2cController.h"
#include "fboss/lib/usb/TransceiverI2CApi.h"

#include <folly/Range.h>
#include <folly/Synchronized.h>
#include <folly/io/async/EventBase.h>

#include <stdint.h>
#include <thread>

namespace facebook::fboss {
inline uint8_t getI2cControllerIdx(uint8_t port) {
  return port / 4;
}

inline uint8_t getI2cControllerChannel(uint8_t port) {
  return port % 4;
}

class FbFpgaI2cError : public I2cError {
 public:
  explicit FbFpgaI2cError(const std::string& what) : I2cError(what) {}
};

class FbFpgaI2c : public I2cController {
 public:
  FbFpgaI2c(FbDomFpga* fpga, uint32_t rtcId, uint32_t pim);

  uint8_t readByte(uint8_t channel, uint8_t offset);
  void read(uint8_t channel, uint8_t offset, folly::MutableByteRange buf);

  void writeByte(uint8_t channel, uint8_t offset, uint8_t val);
  void write(uint8_t channel, uint8_t offset, folly::ByteRange buf);

 private:
  bool waitForResponse(size_t len);
  uint32_t getRegAddr(uint32_t regBase, uint32_t regIncr);

  template <typename Register>
  Register readReg();
  template <typename Register>
  void writeReg(Register value);

  FbDomFpga* fpga_{nullptr};

  int rtcId_{-1};
};

class FbFpgaI2cController {
 public:
  FbFpgaI2cController(FbDomFpga* fpga, uint32_t rtcId, uint32_t pim);
  ~FbFpgaI2cController();

  uint8_t readByte(uint8_t channel, uint8_t offset);
  void read(uint8_t channel, uint8_t offset, folly::MutableByteRange buf);

  void writeByte(uint8_t channel, uint8_t offset, uint8_t val);
  void write(uint8_t channel, uint8_t offset, folly::ByteRange buf);

  folly::EventBase* getEventBase();

  /* Get the I2c transaction stats from this controller with the lock
   */
  const I2cControllerStats& getI2cControllerPlatformStats() const {
    return syncedFbI2c_.lock()->getI2cControllerPlatformStats();
  }

 private:
  folly::Synchronized<FbFpgaI2c, std::mutex> syncedFbI2c_;
  std::unique_ptr<folly::EventBase> eventBase_;
  std::unique_ptr<std::thread> thread_;
};

} // namespace facebook::fboss
