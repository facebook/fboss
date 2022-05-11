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
      io_(std::make_unique<FbDomFpga>(move(io))),
      spiId_(spiId) {
  fpga_ = io_.get();
  XLOG(INFO, "Initialized SPI controller for spiId=", spiId_);
}

uint8_t FbFpgaSpi::readByte(uint8_t offset, int page) {
  uint8_t byte = 0;
  read(offset, page, folly::MutableByteRange(&byte, 1));
  return byte;
}

void FbFpgaSpi::read(
    uint8_t /* offset */,
    int /* page */,
    folly::MutableByteRange /* buf */) {
  // TODO:
}

void FbFpgaSpi::writeByte(uint8_t offset, uint8_t val, int page) {
  write(offset, page, folly::ByteRange(&val, 1));
}

void FbFpgaSpi::write(
    uint8_t /* offset */,
    int /* page */,
    folly::ByteRange /* buf */) {
  // TODO:
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
    : syncedFbSpi_(folly::in_place, move(io), spiId, pim),
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

folly::EventBase* FbFpgaSpiController::getEventBase() {
  return eventBase_.get();
}

} // namespace facebook::fboss
