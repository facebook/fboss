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
} // unnamed namespace

namespace facebook::fboss {
FbFpgaI2c::FbFpgaI2c(
    FbDomFpga* fpga,
    uint32_t rtcId,
    uint32_t pimId,
    int version)
    : I2cController(
          folly::to<std::string>("i2cController.pim.", pimId, ".rtc.", rtcId)),
      fpga_(fpga),
      rtcId_(rtcId),
      version_(version) {
  XLOG(DBG4, "Initialized I2C controller for rtcId=", rtcId);
}

FbFpgaI2c::FbFpgaI2c(
    std::unique_ptr<FpgaMemoryRegion> io,
    uint32_t rtcId,
    uint32_t pimId,
    int version)
    : I2cController(
          folly::to<std::string>("i2cController.pim.", pimId, ".rtc.", rtcId)),
      io_(std::make_unique<FbDomFpga>(std::move(io))),
      rtcId_(rtcId),
      version_(version) {
  fpga_ = io_.get();
  XLOG(DBG4, "Initialized I2C controller for rtcId=", rtcId);
}

bool FbFpgaI2c::waitForResponse(size_t len) {
  I2cRtcStatus rtcStatus(version_);
  uint32_t retries = 20;

  // Make the initial wait according to the length of read/write.
  usleep(100 * len);

  readReg(rtcStatus);

  while (!rtcStatus.dataUnion.desc0done && --retries) {
    usleep(1000);
    readReg(rtcStatus);
  }

  if (rtcStatus.dataUnion.desc0error) {
    XLOG(DBG5) << "I2C read/write ops has error.";
    rtcStatus.dataUnion.reg = 0xffffffff;
    return false;
  }

  return rtcStatus.dataUnion.desc0done;
}

uint8_t
FbFpgaI2c::readByte(uint8_t channel, uint8_t offset, uint8_t i2cAddress) {
  uint8_t byte = 0;
  read(channel, offset, folly::MutableByteRange(&byte, 1), i2cAddress);
  return byte;
}

void FbFpgaI2c::read(
    uint8_t channel,
    uint8_t offset,
    folly::MutableByteRange buf,
    uint8_t i2cAddress) {
  I2cDescriptorLower descLower(version_);
  I2cDescriptorUpper descUpper(version_);
  descLower.dataUnion.reg = 0;
  descUpper.dataUnion.reg = 0;

  descLower.dataUnion.op = 1; // Read
  descLower.dataUnion.len = buf.size();

  descUpper.dataUnion.offset = offset;
  descUpper.dataUnion.channel = channel;
  descUpper.dataUnion.valid = 1;
  descUpper.dataUnion.i2cA2Access = (i2cAddress == 0x51);

  writeReg(descLower);
  writeReg(descUpper);

  // Increment the counter for I2C read tranbsaction issued
  incrReadTotal();

  uint32_t readBlockAddr =
      getRegAddr(kFacebookFpgaRTCReadBlock, getRTCIOBlockSize());

  if (!waitForResponse(descLower.dataUnion.len)) {
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

void FbFpgaI2c::writeByte(
    uint8_t channel,
    uint8_t offset,
    uint8_t val,
    uint8_t i2cAddress) {
  write(channel, offset, folly::ByteRange(&val, 1), i2cAddress);
}

void FbFpgaI2c::write(
    uint8_t channel,
    uint8_t offset,
    folly::ByteRange buf,
    uint8_t i2cAddress) {
  I2cDescriptorLower descLower(version_);
  I2cDescriptorUpper descUpper(version_);
  descLower.dataUnion.reg = 0;
  descUpper.dataUnion.reg = 0;

  descLower.dataUnion.op = 0; // Write
  descLower.dataUnion.len = buf.size();

  descUpper.dataUnion.offset = offset;
  descUpper.dataUnion.channel = channel;
  descUpper.dataUnion.valid = 1;
  descUpper.dataUnion.i2cA2Access = (i2cAddress == 0x51);

  // Increment the counter for write transaction issued
  incrWriteTotal();

  uint32_t writeBlockAddr =
      getRegAddr(kFacebookFpgaRTCWriteBlock, getRTCIOBlockSize());

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

  if (!waitForResponse(descLower.dataUnion.len)) {
    // Increment the counter for I2c write transaction failure and
    // throw error
    incrWriteFailed();

    throw FbFpgaI2cError("I2C write failed.");
  }
  // Update the number of bytes write
  incrWriteBytes(buf.size());
}

template <typename Register>
void FbFpgaI2c::readReg(Register& reg) {
  reg.dataUnion.reg =
      fpga_->read(getRegAddr(reg.getBaseAddr(), reg.getAddrIncr()));
  XLOG(DBG5) << reg;
}

template <typename Register>
void FbFpgaI2c::writeReg(Register& reg) {
  XLOG(DBG5) << reg;
  fpga_->write(
      getRegAddr(reg.getBaseAddr(), reg.getAddrIncr()), reg.dataUnion.reg);
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

uint32_t FbFpgaI2c::getRTCIOBlockSize() {
  switch (version_) {
    case 1:
      return 0x80;
    default:
      return 0x200;
  }
}

FbFpgaI2cController::FbFpgaI2cController(
    FbDomFpga* fpga,
    uint32_t rtcId,
    uint32_t pim,
    int version)
    : syncedFbI2c_(folly::in_place, fpga, rtcId, pim, version),
      eventBase_(std::make_unique<folly::EventBase>()),
      thread_(new std::thread([&, pim, rtcId]() {
        initThread(folly::format("I2c_pim{:d}_rtc{:d}", pim, rtcId).str());
        eventBase_->loopForever();
      })) {
  pim_ = pim;
  rtc_ = rtcId;
}

FbFpgaI2cController::FbFpgaI2cController(
    std::unique_ptr<FpgaMemoryRegion> io,
    uint32_t rtcId,
    uint32_t pim,
    int version)
    : syncedFbI2c_(folly::in_place, std::move(io), rtcId, pim, version),
      eventBase_(std::make_unique<folly::EventBase>()),
      thread_(new std::thread([&, pim, rtcId]() {
        initThread(folly::format("I2c_pim{:d}_rtc{:d}", pim, rtcId).str());
        eventBase_->loopForever();
      })) {
  pim_ = pim;
  rtc_ = rtcId;
}

FbFpgaI2cController::~FbFpgaI2cController() {
  eventBase_->runInEventBaseThread([&] { eventBase_->terminateLoopSoon(); });
  thread_->join();
}

uint8_t FbFpgaI2cController::readByte(
    uint8_t channel,
    uint8_t offset,
    uint8_t i2cAddress) {
  uint8_t buf;
  XLOG(DBG5) << folly::sformat(
      "FbFpgaI2cController::readByte pim {:d} rtc {:d} chan {:d} offset {:d}",
      pim_,
      rtc_,
      channel,
      offset);
  // As this is a sync and blocking function, we don't have to dump it to
  // EventBase to run the functions. So that we can avoid unexpected
  // EventBase chain issue
  return syncedFbI2c_.lock()->readByte(channel, offset, i2cAddress);
}

void FbFpgaI2cController::read(
    uint8_t channel,
    uint8_t offset,
    folly::MutableByteRange buf,
    uint8_t i2cAddress) {
  XLOG(DBG5) << folly::sformat(
      "FbFpgaI2cController::read pim {:d} rtc {:d} chan {:d} offset {:d} i2cAddress {:d}",
      pim_,
      rtc_,
      channel,
      offset,
      i2cAddress);
  // As this is a sync and blocking function, we don't have to dump it to
  // EventBase to run the functions. So that we can avoid unexpected
  // EventBase chain issue
  syncedFbI2c_.lock()->read(channel, offset, buf, i2cAddress);
}

void FbFpgaI2cController::writeByte(
    uint8_t channel,
    uint8_t offset,
    uint8_t val,
    uint8_t i2cAddress) {
  XLOG(DBG5) << folly::sformat(
      "FbFpgaI2cController::writeByte pim {:d} rtc {:d} chan {:d} offset {:d} val {:d}",
      pim_,
      rtc_,
      channel,
      offset,
      val);
  // As this is a sync and blocking function, we don't have to dump it to
  // EventBase to run the functions. So that we can avoid unexpected
  // EventBase chain issue
  syncedFbI2c_.lock()->writeByte(channel, offset, val, i2cAddress);
}

void FbFpgaI2cController::write(
    uint8_t channel,
    uint8_t offset,
    folly::ByteRange buf,
    uint8_t i2cAddress) {
  XLOG(DBG5) << folly::sformat(
      "FbFpgaI2cController::write pim {:d} rtc {:d} chan {:d} offset {:d} i2cAddress {:d}",
      pim_,
      rtc_,
      channel,
      offset,
      i2cAddress);
  // As this is a sync and blocking function, we don't have to dump it to
  // EventBase to run the functions. So that we can avoid unexpected
  // EventBase chain issue
  syncedFbI2c_.lock()->write(channel, offset, buf, i2cAddress);
}

folly::EventBase* FbFpgaI2cController::getEventBase() {
  return eventBase_.get();
}

} // namespace facebook::fboss
