/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/mdio/FbFpgaMdio.h"
#include <folly/logging/xlog.h>
#include <chrono>
#include <sstream>
#include "fboss/mdio/MdioError.h"

namespace {

constexpr uint32_t kDefaultTxnWaitMillis = 10000;
constexpr uint32_t kFacebookFpgaSlpcParityReg = 0x80;
constexpr uint32_t kFacebookFpgaSlpcParityMask = 0xFFFF0000;
constexpr uint32_t kFacebookFpgaSlpcParityBitshift = 16;
} // namespace

namespace facebook::fboss {

FbFpgaMdio::FbFpgaMdio(
    FpgaMemoryRegion* io,
    uint32_t baseAddr,
    FbFpgaMdioVersion version)
    : io_(io), baseAddr_(baseAddr), version_(version) {
  if (version_ == FbFpgaMdioVersion::UNKNOWN) {
    XLOG(DBG4)
        << "unknown verion, do auto-detection by reading version register";
    throw MdioError("Mdio version auto-detection not implemented");
    // TODO: read fpga version register and set up version_
  }
}

void FbFpgaMdio::reset() {
  auto config = readReg<MdioConfig>();

  config.reset = 1;
  XLOG(DBG1)
      << "Resetting mdio controller and bringing mdio controller out of reset";
  writeReg(config);
  sleep(1);
  config.reset = 0;
  writeReg(config);
}

// Set the clock divisor. By default, it's 20, resulting in a 2.5MHz MDC clock
// on EVT Fuji (but not MP1), we can set it to 10 for a clock speed of 5MHz
// on DVT Fuji we will be able to set it to 4 for a clock speed of 12.5MHz
void FbFpgaMdio::setClockDivisor(int div) {
  auto config = readReg<MdioConfig>();
  config.mdcFreq = div;
  XLOG(DBG1) << "Setting mdio controller divisor to " << div;
  writeReg(config);
}

// Set mdio fast mode (reduces idle time)
void FbFpgaMdio::setFastMode(bool enable) {
  auto config = readReg<MdioConfig>();
  config.fastMode = ((enable) ? 1 : 0);
  XLOG(DBG1) << "Setting mdio controller fastMode to "
             << (enable ? "enabled" : "disabled");
  writeReg(config);
}

void FbFpgaMdio::clearStatus() {
  MdioStatus status;
  status.reg = 0;
  status.done = 1;
  status.err = 1;
  writeReg(status);
  status = readReg<MdioStatus>();
  XCHECK(!status.done && !status.err)
      << "Failed to clear mdio status reg for fpgaDevice " << io_->getName();
}

void FbFpgaMdio::waitUntilDone(uint32_t millis, MdioCommand command) {
  auto throwErr = [command](auto&& msg) {
    std::stringstream ss;
    // add 1 to pim to make 2-offset
    ss << msg << ": " << command;
    throw std::runtime_error(ss.str());
  };

  auto endTime =
      std::chrono::steady_clock::now() + std::chrono::milliseconds(millis);
  while (std::chrono::steady_clock::now() < endTime) {
    auto status = readReg<MdioStatus>();
    if (status.done) {
      if (status.err) {
        printSlpcError();
        throwErr("Mdio transaction error");
      }
      return;
    }
    usleep(10);
  }
  printSlpcError();
  throwErr("Mdio transaction timed out");
}

phy::Cl45Data FbFpgaMdio::readCl45(
    phy::PhyAddress physAddr,
    phy::Cl45DeviceAddress devAddr,
    phy::Cl45RegisterAddress regAddr) {
  // needed?
  clearStatus();

  MdioCommand command;
  command.reg = 0;
  command.devAddr = devAddr;
  command.regAddr = regAddr;
  command.rw = 1;
  command.phySel = physAddr & 0b11111;
  writeReg(command);
  waitUntilDone(kDefaultTxnWaitMillis, command);

  auto readData = readReg<MdioRead>();
  return phy::Cl45Data(readData.data);
}

void FbFpgaMdio::writeCl45(
    phy::PhyAddress physAddr,
    phy::Cl45DeviceAddress devAddr,
    phy::Cl45RegisterAddress regAddr,
    phy::Cl45Data data) {
  MdioWrite writeData;
  writeData.data = data;
  writeReg(writeData);

  // needed?
  clearStatus();

  MdioCommand command;
  command.reg = 0;
  command.devAddr = devAddr;
  command.regAddr = regAddr;
  command.rw = 0;
  command.phySel = physAddr & 0b11111;
  writeReg(command);
  waitUntilDone(kDefaultTxnWaitMillis, command);
}

template <typename Register>
Register FbFpgaMdio::readReg() {
  Register ret;
  uint32_t offset = Register::addr::value + baseAddr_;
  ret.reg = io_->read(offset);
  XLOG(DBG5) << ret;
  return ret;
}

template <typename Register>
void FbFpgaMdio::writeReg(Register value) {
  XLOG(DBG5) << value;
  uint32_t offset = Register::addr::value + baseAddr_;
  io_->write(offset, value.reg);
}

void FbFpgaMdio::printSlpcError() {
  uint32_t offset = baseAddr_ + kFacebookFpgaSlpcParityReg;
  uint32_t slpcErr = (io_->read(offset) & kFacebookFpgaSlpcParityMask) >>
      kFacebookFpgaSlpcParityBitshift;
  XLOG(ERR) << fmt::format(
      "For {}, SLPC parity error: {:d}", io_->getName(), slpcErr);
}
} // namespace facebook::fboss
