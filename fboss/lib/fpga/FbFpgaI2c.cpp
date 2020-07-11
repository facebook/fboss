// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/lib/fpga/FbFpgaI2c.h"

#include "fboss/agent/Utils.h"
#include "fboss/lib/fpga/FbFpgaRegisters.h"

#include <folly/CppAttributes.h>
#include <folly/Format.h>
#include <folly/Synchronized.h>
#include <folly/futures/Future.h>
#include <folly/logging/xlog.h>

#include <algorithm>
#include <thread>

namespace {
constexpr uint32_t kFacebookFpgaRTCWriteBlock = 0x2000;
constexpr uint32_t kFacebookFpgaRTCReadBlock = 0x3000;
constexpr uint32_t kFacebookFpgaRTCIOBlockSize = 0x0200;
} // unnamed namespace

namespace facebook::fboss {
FbFpgaI2c::FbFpgaI2c(FbDomFpga* fpga, uint32_t rtcId, uint32_t pimId)
    : I2cController(
          folly::to<std::string>("i2cController.pim.", pimId, ".rtc.", rtcId)),
      fpga_(fpga),
      rtcId_(rtcId) {
  XLOG(DBG4, "Initialized I2C controller for rtcId={:d}", rtcId);
}

bool FbFpgaI2c::waitForResponse(size_t len) {
  I2cRtcStatus rtcStatus;
  uint32_t retries = 20;

  // Make the initial wait according to the length of read/write.
  usleep(100 * len);

  rtcStatus = readReg<I2cRtcStatus>();

  while (!rtcStatus.desc0done && --retries) {
    usleep(1000);
    rtcStatus = readReg<I2cRtcStatus>();
  }

  if (rtcStatus.desc0error) {
    XLOG(DBG5) << "I2C read/write ops has error.";
    rtcStatus.reg = 0xffffffff;
    return false;
  }

  return rtcStatus.desc0done;
}

uint8_t FbFpgaI2c::readByte(uint8_t channel, uint8_t offset) {
  uint8_t byte = 0;
  read(channel, offset, folly::MutableByteRange(&byte, 1));
  return byte;
}

void FbFpgaI2c::read(
    uint8_t channel,
    uint8_t offset,
    folly::MutableByteRange buf) {
  I2cDescriptorLower descLower;
  I2cDescriptorUpper descUpper;
  descLower.reg = 0;
  descUpper.reg = 0;

  descLower.op = 1; // Read
  descLower.len = buf.size();

  descUpper.offset = offset;
  descUpper.channel = channel;
  descUpper.valid = 1;

  writeReg(descLower);
  writeReg(descUpper);

  // Increment the counter for I2C read tranbsaction issued
  incrReadTotal();

  uint32_t readBlockAddr =
      getRegAddr(kFacebookFpgaRTCReadBlock, kFacebookFpgaRTCIOBlockSize);

  if (!waitForResponse(descLower.len)) {
    // Increment the counter for I2C read transaction failure and
    // throw error
    incrReadFailed();

    throw FbFpgaI2cError("I2C read failed.");
  } else {
    for (int bytesRead = 0; bytesRead < buf.size(); bytesRead += 4) {
      uint32_t data = fpga_->read(readBlockAddr + bytesRead);
      std::memcpy(
          buf.begin() + bytesRead,
          &data,
          std::min(buf.size() - bytesRead, (size_t)4));
    }
    // Update the number of bytes read
    incrReadBytes(buf.size());
  }
}

void FbFpgaI2c::writeByte(uint8_t channel, uint8_t offset, uint8_t val) {
  write(channel, offset, folly::ByteRange(&val, 1));
}

void FbFpgaI2c::write(uint8_t channel, uint8_t offset, folly::ByteRange buf) {
  I2cDescriptorLower descLower;
  I2cDescriptorUpper descUpper;
  descLower.reg = 0;
  descUpper.reg = 0;

  descLower.op = 0; // Write
  descLower.len = buf.size();

  descUpper.offset = offset;
  descUpper.channel = channel;
  descUpper.valid = 1;

  // Increment the counter for write transaction issued
  incrWriteTotal();

  uint32_t writeBlockAddr =
      getRegAddr(kFacebookFpgaRTCWriteBlock, kFacebookFpgaRTCIOBlockSize);

  for (int bytesWritten = 0; bytesWritten < buf.size(); bytesWritten += 4) {
    uint32_t data;
    std::memcpy(
        &data,
        buf.begin() + bytesWritten,
        std::min(buf.size() - bytesWritten, (size_t)4));
    fpga_->write(writeBlockAddr + bytesWritten, data);
  }

  writeReg(descLower);
  writeReg(descUpper);

  if (!waitForResponse(descLower.len)) {
    // Increment the counter for I2c write transaction failure and
    // throw error
    incrWriteFailed();

    throw FbFpgaI2cError("I2C write failed.");
  }
  // Update the number of bytes write
  incrWriteBytes(buf.size());
}

template <typename Register>
Register FbFpgaI2c::readReg() {
  Register ret;
  ret.reg = fpga_->read(
      getRegAddr(Register::baseAddr::value, Register::addrIncr::value));
  XLOG(DBG5) << ret;
  return ret;
}

template <typename Register>
void FbFpgaI2c::writeReg(Register value) {
  XLOG(DBG5) << value;
  fpga_->write(
      getRegAddr(Register::baseAddr::value, Register::addrIncr::value),
      value.reg);
}

uint32_t FbFpgaI2c::getRegAddr(uint32_t regBase, uint32_t regIncr) {
  // Since the FbFpga group RTC registers based on their function and not
  // completely according to RTC index, here we will need the base address of a
  // function group of registers and the incremental size of that register to
  // determine the correct address of the register we want to access.
  //
  // For example in an FPGA that has four RTCs in it, the register layout would
  // be like:
  // RTC0 DescriptorLower | RTC0 DescriptorUpper
  // RTC1 DescriptorLower | RTC1 DescriptorUpper
  // RTC2 DescriptorLower | RTC2 DescriptorUpper
  // RTC3 DescriptorLower | RTC3 DescriptorUpper
  // ...
  // RTC0 Status | RTC1 Status | RTC2 Status | RTC3 Status
  return regBase + regIncr * rtcId_;
}

FbFpgaI2cController::FbFpgaI2cController(
    FbDomFpga* fpga,
    uint32_t rtcId,
    uint32_t pim)
    : syncedFbI2c_(folly::in_place, fpga, rtcId, pim),
      eventBase_(std::make_unique<folly::EventBase>()),
      thread_(new std::thread([&, pim, rtcId]() {
        initThread(folly::format("I2c_pim{:d}_rtc{:d}", pim, rtcId).str());
        eventBase_->loopForever();
      })) {}

FbFpgaI2cController::~FbFpgaI2cController() {
  eventBase_->runInEventBaseThread([&] { eventBase_->terminateLoopSoon(); });
  thread_->join();
}

uint8_t FbFpgaI2cController::readByte(uint8_t channel, uint8_t offset) {
  uint8_t buf;
  if (eventBase_->isInEventBaseThread()) {
    buf = syncedFbI2c_.lock()->readByte(channel, offset);
  } else {
    via(eventBase_.get())
        .thenValue([&](auto&&) mutable {
          buf = syncedFbI2c_.lock()->readByte(channel, offset);
        })
        .get();
  }
  return buf;
}

void FbFpgaI2cController::read(
    uint8_t channel,
    uint8_t offset,
    folly::MutableByteRange buf) {
  if (eventBase_->isInEventBaseThread()) {
    syncedFbI2c_.lock()->read(channel, offset, buf);
  } else {
    via(eventBase_.get())
        .thenValue([=](auto&&) mutable {
          syncedFbI2c_.lock()->read(channel, offset, buf);
        })
        .get();
  }
}

void FbFpgaI2cController::writeByte(
    uint8_t channel,
    uint8_t offset,
    uint8_t val) {
  if (eventBase_->isInEventBaseThread()) {
    syncedFbI2c_.lock()->writeByte(channel, offset, val);
  } else {
    via(eventBase_.get())
        .thenValue([=](auto&&) mutable {
          syncedFbI2c_.lock()->writeByte(channel, offset, val);
        })
        .get();
  }
}

void FbFpgaI2cController::write(
    uint8_t channel,
    uint8_t offset,
    folly::ByteRange buf) {
  if (eventBase_->isInEventBaseThread()) {
    syncedFbI2c_.lock()->write(channel, offset, buf);
  } else {
    via(eventBase_.get())
        .thenValue([=](auto&&) mutable {
          syncedFbI2c_.lock()->write(channel, offset, buf);
        })
        .get();
  }
}

folly::EventBase* FbFpgaI2cController::getEventBase() {
  return eventBase_.get();
}

} // namespace facebook::fboss
