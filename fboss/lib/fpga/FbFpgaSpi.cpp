// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/lib/fpga/FbFpgaSpi.h"

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
constexpr uint32_t kFacebookFpgaSPIWriteBlock = 0x4000;
constexpr uint32_t kFacebookFpgaSPIReadBlock = 0x4200;
constexpr uint32_t kFlowControlAsserted = 0xFF;
constexpr uint32_t kDataIOBlockSize = 0x400;
} // unnamed namespace

namespace facebook::fboss {
FbFpgaSpi::FbFpgaSpi(FbDomFpga* fpga, uint32_t spiId, uint32_t pimId)
    : I2cController(
          folly::to<std::string>("spiController.pim.", pimId, ".spi.", spiId)),
      fpga_(fpga),
      spiId_(spiId) {
  XLOG(INFO, "Initialized SPI controller for spiId=", spiId_);
}

FbFpgaSpi::FbFpgaSpi(
    std::unique_ptr<FpgaMemoryRegion> io,
    uint32_t spiId,
    uint32_t pimId)
    : I2cController(
          folly::to<std::string>("spiController.pim.", pimId, ".spi.", spiId)),
      io_(std::make_unique<FbDomFpga>(std::move(io))),
      spiId_(spiId) {
  fpga_ = io_.get();
  XLOG(INFO, "Initialized SPI controller for spiId=", spiId_);
}

uint8_t FbFpgaSpi::readByte(uint8_t offset, int page) {
  uint8_t byte = 0;
  read(offset, page, folly::MutableByteRange(&byte, 1));
  return byte;
}

void FbFpgaSpi::read(uint8_t offset, int page, folly::MutableByteRange buf) {
  SpiDescriptorLower descLower;
  SpiDescriptorUpper descUpper;
  SpiWriteDataBlock writeDataBlock;
  std::string logMsgPrefix = folly::sformat(
      "SpiRead: SpiId: {:d}, offset: {:x}, page: {:x}", spiId_, offset, page);

  // Increment the counter for total read transactions issued
  incrReadTotal();

  if (!oboReadyForIO()) {
    incrReadFailed();
    throw FbFpgaSpiError(folly::to<std::string>(
        logMsgPrefix, " - read failed, OBO is not ready"));
  }

  // Write the IO information in the data block
  writeDataBlock.dataUnion.reg = 0;
  writeDataBlock.dataUnion.transferDirection = 0; // read
  writeDataBlock.dataUnion.numBytes = buf.size() - 1;
  writeDataBlock.dataUnion.pageNumber = page;
  writeDataBlock.dataUnion.registerOffset = offset;
  writeReg(writeDataBlock);

  descLower.dataUnion.reg = 0;
  descLower.dataUnion.spiDoneIntrEnable = 1;
  descLower.dataUnion.spiErrIntrEnable = 1;
  descLower.dataUnion.lengthOfData = buf.size() + 6;
  descLower.dataUnion.valid = 1;

  descUpper.dataUnion.reg = 0;
  descUpper.dataUnion.spiDone = 1;
  descUpper.dataUnion.spiErr = 1;

  writeReg(descUpper);
  writeReg(descLower);

  if (!waitForSpiDone()) {
    incrReadFailed();
    throw FbFpgaSpiError(folly::to<std::string>(
        logMsgPrefix, " - read failed, SPI txn is not done"));
  } else {
    uint32_t readBlockAddr =
        getRegAddr(kFacebookFpgaSPIReadBlock, kDataIOBlockSize);
    size_t numBytes = 0;
    // The first 6 bytes are preheader and need to be skipped in the read.
    // Therefore, start from offset 4 which skips the first 4 bytes, and then
    // trim the 2nd byte to remove the other 2 (6-4) bytes
    for (size_t bytesRead = 4; bytesRead < 6 + buf.size(); bytesRead += 4) {
      uint32_t data = fpga_->read(readBlockAddr + bytesRead);
      size_t bytesToCopy = std::min(buf.size() - numBytes, (size_t)4);
      if (bytesRead == 4) {
        data = data >> 16;
        bytesToCopy = std::min(bytesToCopy, (size_t)2);
      }
      std::memcpy(buf.begin() + numBytes, &data, bytesToCopy);
      numBytes += bytesToCopy;
    }
    // Update the number of bytes read
    incrReadBytes(buf.size());
  }
}

void FbFpgaSpi::writeByte(uint8_t offset, uint8_t val, int page) {
  write(offset, page, folly::ByteRange(&val, 1));
}

void FbFpgaSpi::write(uint8_t offset, int page, folly::ByteRange buf) {
  SpiDescriptorLower descLower;
  SpiDescriptorUpper descUpper;
  SpiWriteDataBlock writeDataBlock;
  std::string logMsgPrefix = folly::sformat(
      "SpiWrite: SpiId: {:d}, offset: {:x}, page: {:x}", spiId_, offset, page);

  // Increment the counter for total write transactions issued
  incrWriteTotal();

  if (!oboReadyForIO()) {
    incrWriteFailed();
    throw FbFpgaSpiError(folly::to<std::string>(
        logMsgPrefix, " - write failed, OBO is not ready"));
  }

  // Write the IO information in the data block
  writeDataBlock.dataUnion.reg = 0;
  writeDataBlock.dataUnion.transferDirection = 1; // write
  writeDataBlock.dataUnion.numBytes = buf.size() - 1;
  writeDataBlock.dataUnion.pageNumber = 0;
  writeDataBlock.dataUnion.registerOffset = offset;
  writeReg(writeDataBlock);

  // Set data Buffer
  uint32_t writeBlockAddr =
      getRegAddr(kFacebookFpgaSPIWriteBlock, kDataIOBlockSize);
  size_t numBytes = 0;
  for (size_t bytesWritten = 4; bytesWritten < 6 + buf.size();
       bytesWritten += 4) {
    uint32_t data;
    size_t bytesToCopy = std::min(buf.size() - numBytes, (size_t)4);
    if (bytesWritten == 4) {
      bytesToCopy = std::min(bytesToCopy, (size_t)2);
    }
    std::memcpy(&data, buf.begin() + numBytes, bytesToCopy);
    if (bytesWritten == 4) {
      data = data << 16;
    }
    fpga_->write(writeBlockAddr + bytesWritten, data);
    numBytes += bytesToCopy;
  }

  // Set Upper Descriptor
  descUpper.dataUnion.reg = 0;
  descUpper.dataUnion.spiDone = 1;
  descUpper.dataUnion.spiErr = 1;
  writeReg(descUpper);

  // Set Lower Descriptor

  descLower.dataUnion.reg = 0;
  descLower.dataUnion.spiDoneIntrEnable = 1;
  descLower.dataUnion.spiErrIntrEnable = 1;
  descLower.dataUnion.lengthOfData = buf.size() + 6;
  descLower.dataUnion.valid = 1;
  writeReg(descLower);
  if (!waitForSpiDone()) {
    incrWriteFailed();
    throw FbFpgaSpiError(folly::to<std::string>(
        logMsgPrefix, " - write failed, SPI txn is not done"));
  }
  // Update the number of bytes written
  incrWriteBytes(buf.size());
}

template <typename Register>
void FbFpgaSpi::readReg(Register& reg) {
  reg.dataUnion.reg =
      fpga_->read(getRegAddr(reg.getBaseAddr(), reg.getAddrIncr()));
}

template <typename Register>
void FbFpgaSpi::writeReg(Register& reg) {
  fpga_->write(
      getRegAddr(reg.getBaseAddr(), reg.getAddrIncr()), reg.dataUnion.reg);
}

uint32_t FbFpgaSpi::getRegAddr(uint32_t regBase, uint32_t regIncr) {
  return regBase + regIncr * spiId_;
}

void FbFpgaSpi::initializeSPIController(bool forceInit) {
  SpiMasterCsr masterCsr;
  readReg(masterCsr);
  // If tbb is not 6us and clock is not 12.5MHz, then set those attributes. If
  // the values are already set, don't reset the SPI controller unnecessarily
  if (forceInit || masterCsr.dataUnion.tbb != 6 ||
      masterCsr.dataUnion.clkDivider != 1) {
    XLOG(INFO) << "Configuring SPI controller " << spiId_;
    SpiCtrlReset ctrlReset;
    ctrlReset.dataUnion.reg = 0;
    ctrlReset.dataUnion.reset = 1;
    writeReg(ctrlReset);

    masterCsr.dataUnion.tbb = 6;
    masterCsr.dataUnion.clkDivider = 1;
    writeReg(masterCsr);
  }
}

bool FbFpgaSpi::flowControlAsserted() {
  // Check if the flow control byte 5 (indexed 0) is 0xff (flow control
  // asserted)
  uint32_t readBlockAddr =
      getRegAddr(kFacebookFpgaSPIReadBlock, kDataIOBlockSize);
  uint32_t data = fpga_->read(readBlockAddr + 4);
  return ((data >> 16) & 0xF) == kFlowControlAsserted;
}

bool FbFpgaSpi::waitForSpiDone() {
  SpiDescriptorUpper descUpper;
  readReg(descUpper);
  int retries = 20;
  while ((!descUpper.dataUnion.spiErr && !descUpper.dataUnion.spiDone) &&
         --retries) {
    /* sleep override */
    usleep(1000);
    readReg(descUpper);
  }

  if (descUpper.dataUnion.spiErr || !descUpper.dataUnion.spiDone) {
    XLOG(DBG3) << "SPI: " << spiId_ << ", SPI done not set or SPI Error set "
               << descUpper.dataUnion;
    return false;
  }

  if (flowControlAsserted()) {
    XLOG(DBG3) << "SPI: " << spiId_ << ", Flow control asserted";
    return false;
  }
  return true;
}

bool FbFpgaSpi::oboReadyForIO() {
  // Here we trigger a dummy read of 1 byte in page 0xA0 which we know
  // doesn't result in flow control assertion
  SpiDescriptorLower descLower;
  SpiDescriptorUpper descUpper;
  SpiWriteDataBlock writeDataBlock;

  descLower.dataUnion.reg = 0;
  descUpper.dataUnion.reg = 0;
  writeDataBlock.dataUnion.reg = 0;

  writeDataBlock.dataUnion.transferDirection = 0; // read
  writeDataBlock.dataUnion.numBytes = 0; // 1 byte
  writeDataBlock.dataUnion.pageNumber = 0xa0;
  writeDataBlock.dataUnion.registerOffset = 0x80; // 128
  writeReg(writeDataBlock);

  descLower.dataUnion.spiDoneIntrEnable = 1;
  descLower.dataUnion.spiErrIntrEnable = 1;
  descLower.dataUnion.lengthOfData = 7; // 6 preheader + 1 byte of read
  descLower.dataUnion.valid = 1;

  // Writing 1 to spiDone and spiErr clears these bits so that we can read the
  // updated status later
  descUpper.dataUnion.spiDone = 1;
  descUpper.dataUnion.spiErr = 1;

  writeReg(descUpper);
  writeReg(descLower);

  return waitForSpiDone();
}

FbFpgaSpiController::FbFpgaSpiController(
    FbDomFpga* fpga,
    uint32_t spiId,
    uint32_t pim)
    : syncedFbSpi_(folly::in_place, fpga, spiId, pim),
      eventBase_(std::make_unique<folly::EventBase>()),
      thread_(new std::thread([&, pim, spiId]() {
        initThread(fmt::format("SPI_pim{:d}_spi{:d}", pim, spiId));
        eventBase_->loopForever();
      })) {
  pim_ = pim;
  spiId_ = spiId;
}

FbFpgaSpiController::FbFpgaSpiController(
    std::unique_ptr<FpgaMemoryRegion> io,
    uint32_t spiId,
    uint32_t pim)
    : syncedFbSpi_(folly::in_place, std::move(io), spiId, pim),
      eventBase_(std::make_unique<folly::EventBase>()),
      thread_(new std::thread([&, pim, spiId]() {
        initThread(fmt::format("SPI_pim{:d}_spi{:d}", pim, spiId));
        eventBase_->loopForever();
      })) {
  pim_ = pim;
  spiId_ = spiId;
}

FbFpgaSpiController::~FbFpgaSpiController() {
  eventBase_->runInEventBaseThread([&] { eventBase_->terminateLoopSoon(); });
  thread_->join();
}

uint8_t FbFpgaSpiController::readByte(uint8_t offset, int page) {
  uint8_t buf;
  XLOG(DBG5) << folly::sformat(
      "FbFpgaSpiController::readByte pim {:d} spi {:d} offset {:d}",
      pim_,
      spiId_,
      offset);
  buf = syncedFbSpi_.lock()->readByte(offset, page);
  return buf;
}

void FbFpgaSpiController::read(
    uint8_t offset,
    int page,
    folly::MutableByteRange buf) {
  XLOG(DBG5) << folly::sformat(
      "FbFpgaSpiController::read pim {:d} spi {:d} offset {:d}",
      pim_,
      spiId_,
      offset);
  syncedFbSpi_.lock()->read(offset, page, buf);
}

void FbFpgaSpiController::writeByte(uint8_t offset, uint8_t val, int page) {
  XLOG(DBG5) << folly::sformat(
      "FbFpgaSpiController::writeByte pim {:d} spi {:d} offset {:d} val {:d}",
      pim_,
      spiId_,
      offset,
      val);
  syncedFbSpi_.lock()->writeByte(offset, val, page);
}

void FbFpgaSpiController::write(
    uint8_t offset,
    int page,
    folly::ByteRange buf) {
  XLOG(DBG5) << folly::sformat(
      "FbFpgaSpiController::write pim {:d} spi {:d} offset {:d}",
      pim_,
      spiId_,
      offset);

  syncedFbSpi_.lock()->write(offset, page, buf);
}

void FbFpgaSpiController::initializeSPIController(bool forceInit) {
  syncedFbSpi_.lock()->initializeSPIController(forceInit);
}

folly::EventBase* FbFpgaSpiController::getEventBase() {
  return eventBase_.get();
}

} // namespace facebook::fboss
