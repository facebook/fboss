// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/lib/fpga/FbDomFpga.h"

#include <folly/Format.h>
#include <folly/logging/xlog.h>
#include "fboss/agent/FbossError.h"

namespace {
const std::string getColorStr(facebook::fboss::FbDomFpga::LedColor ledColor) {
  switch (ledColor) {
    case facebook::fboss::FbDomFpga::LedColor::OFF:
      return "OFF";
    case facebook::fboss::FbDomFpga::LedColor::WHITE:
      return "White";
    case facebook::fboss::FbDomFpga::LedColor::CYAN:
      return "Cyan";
    case facebook::fboss::FbDomFpga::LedColor::BLUE:
      return "Blue";
    case facebook::fboss::FbDomFpga::LedColor::PINK:
      return "Pink";
    case facebook::fboss::FbDomFpga::LedColor::RED:
      return "Red";
    case facebook::fboss::FbDomFpga::LedColor::ORANGE:
      return "Orange";
    case facebook::fboss::FbDomFpga::LedColor::YELLOW:
      return "Yellow";
    case facebook::fboss::FbDomFpga::LedColor::GREEN:
      return "Green";
    default:
      return "Unknown";
  }
}

constexpr uint32_t kFacebookFpgaPimTypeReg = 0x0;
constexpr uint32_t kFacebookFpgaPortLedBase = 0x0310;
constexpr uint32_t kFacebookFpgaQsfpPresentReg = 0x0048;
constexpr uint32_t kFacebookFpgaQsfpResetReg = 0x0070;
constexpr uint32_t kFacebookFpgaPimTypeBase = 0xFF000000;

inline uint32_t getPortLedAddress(int port) {
  return kFacebookFpgaPortLedBase + port * 4;
}

} // namespace

namespace facebook::fboss {
FbDomFpga::FbDomFpga(uint32_t domBaseAddr, uint32_t domFpgaSize, uint8_t pim)
    : phyMem32_(std::make_unique<FbFpgaPhysicalMemory32>(
          domBaseAddr,
          domFpgaSize,
          false)),
      pim_{pim},
      domBaseAddr_(domBaseAddr),
      domFpgaSize_(domFpgaSize) {
  XLOG(DBG2) << folly::format(
      "Creating Fb DOM FPGA at address={:#x} size={:d}",
      domBaseAddr_,
      domFpgaSize_);
}

void FbDomFpga::initHW() {
  // mmap the 32bit io physical memory
  phyMem32_->mmap();
}

uint32_t FbDomFpga::read(uint32_t offset) const {
  CHECK(offset >= 0 && offset <= domFpgaSize_);
  uint32_t ret = phyMem32_->read(offset);
  XLOG(DBG5) << folly::format("FPGA read {:#x}={:#x}", offset, ret);
  return ret;
}

void FbDomFpga::write(uint32_t offset, uint32_t value) {
  CHECK(offset >= 0 && offset <= domFpgaSize_);
  XLOG(DBG5) << folly::format("FPGA write {:#x} to {:#x}", value, offset);
  phyMem32_->write(offset, value);
}

bool FbDomFpga::isQsfpPresent(int qsfp) {
  uint32_t qsfpPresentReg = read(kFacebookFpgaQsfpPresentReg);
  // From the lower end, each bit of this register represent the presence of a
  // QSFP.
  XLOG(DBG5) << folly::format("qsfpPresentReg value:{:#x}", qsfpPresentReg);
  return (qsfpPresentReg >> qsfp) & 1;
}

uint32_t FbDomFpga::getQsfpsPresence() {
  return read(kFacebookFpgaQsfpPresentReg);
}

void FbDomFpga::ensureQsfpOutOfReset(int qsfp) {
  uint32_t currentResetReg = read(kFacebookFpgaQsfpResetReg);
  // 1 to hold QSFP reset active. 0 to release QSFP reset.
  uint32_t newResetReg = ~(0x1 << qsfp) & currentResetReg;
  if (currentResetReg != newResetReg) {
    XLOG(DBG5) << folly::format(
        "For pim_{:d}, port{:d}, old QsfpResetReg value:{:#x}, new value:{:#x}",
        pim_,
        qsfp,
        currentResetReg,
        newResetReg);
    write(kFacebookFpgaQsfpResetReg, newResetReg);
  }
}

/* This function triggers a hard reset to the QSFP module by toggling
 * bit in FPGA. This function is applicable to minipack16Q, minipack4dd
 * which call it through Minipack.*Manager class
 */
void FbDomFpga::triggerQsfpHardReset(int qsfp) {
  uint32_t originalResetReg = read(kFacebookFpgaQsfpResetReg);

  // 1: hold QSFP in reset state
  // 0: release QSFP reset for normal operation

  // Hold the QSFP in reset state
  uint32_t newResetReg = (0x1 << qsfp) | originalResetReg;

  XLOG(INFO) << folly::format(
      "For pim_{:d}, port{:d}, old QsfpResetReg value:{:#x}, new value:{:#x}",
      pim_,
      qsfp,
      originalResetReg,
      newResetReg);
  write(kFacebookFpgaQsfpResetReg, newResetReg);

  // Wait for 10ms
  std::this_thread::sleep_for(std::chrono::milliseconds(10));

  // Release the QSFP reset
  newResetReg = ~(0x1 << qsfp) & originalResetReg;
  write(kFacebookFpgaQsfpResetReg, newResetReg);
}

void FbDomFpga::setFrontPanelLedColor(int qsfp, FbDomFpga::LedColor ledColor) {
  uint32_t qsfpLedAddress = getPortLedAddress(qsfp);
  write(qsfpLedAddress, static_cast<uint32_t>(ledColor));

  XLOG(DBG5) << folly::format(
      "Set pim={:d} qsfp={:d} LED to {:s}. Register={:#x}, new value={:#x}",
      pim_,
      qsfp,
      getColorStr(ledColor),
      qsfpLedAddress,
      static_cast<uint16_t>(ledColor));
}

FbDomFpga::PimType FbDomFpga::getPimType() {
  uint32_t curPimTypeReg =
      read(kFacebookFpgaPimTypeReg) & kFacebookFpgaPimTypeBase;
  switch (curPimTypeReg) {
    case static_cast<uint32_t>(FbDomFpga::PimType::MINIPACK_16Q):
    case static_cast<uint32_t>(FbDomFpga::PimType::MINIPACK_16O):
      return static_cast<FbDomFpga::PimType>(curPimTypeReg);
    default:
      throw FbossError(
          "Unrecoginized pim type with register value:", curPimTypeReg);
  }
}

} // namespace facebook::fboss
