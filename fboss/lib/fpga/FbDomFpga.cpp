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

bool FbDomFpga::isQsfpPresent(int qsfp) {
  uint32_t qsfpPresentReg = io_->read(kFacebookFpgaQsfpPresentReg);
  // From the lower end, each bit of this register represent the presence of a
  // QSFP.
  XLOG(DBG5) << folly::format("qsfpPresentReg value:{:#x}", qsfpPresentReg);
  return (qsfpPresentReg >> qsfp) & 1;
}

uint32_t FbDomFpga::getQsfpsPresence() {
  return io_->read(kFacebookFpgaQsfpPresentReg);
}

void FbDomFpga::ensureQsfpOutOfReset(int qsfp) {
  std::lock_guard<std::mutex> g(qsfpResetRegLock_);
  uint32_t currentResetReg = io_->read(kFacebookFpgaQsfpResetReg);
  // 1 to hold QSFP reset active. 0 to release QSFP reset.
  uint32_t newResetReg = ~(0x1 << qsfp) & currentResetReg;
  if (currentResetReg != newResetReg) {
    XLOG(DBG5) << folly::format(
        "For {}, port{:d}, old QsfpResetReg value:{:#x}, new value:{:#x}",
        io_->getName(),
        qsfp,
        currentResetReg,
        newResetReg);
    io_->write(kFacebookFpgaQsfpResetReg, newResetReg);
  }
}

/* This function triggers a hard reset to the QSFP module by toggling
 * bit in FPGA. This function is applicable to minipack16Q
 * which call it through Minipack.*Manager class
 */
void FbDomFpga::triggerQsfpHardReset(int qsfp) {
  std::lock_guard<std::mutex> g(qsfpResetRegLock_);
  uint32_t originalResetReg = io_->read(kFacebookFpgaQsfpResetReg);

  // 1: hold QSFP in reset state
  // 0: release QSFP reset for normal operation

  // Hold the QSFP in reset state
  uint32_t newResetReg = (0x1 << qsfp) | originalResetReg;

  XLOG(INFO) << folly::format(
      "For {}, port{:d}, old QsfpResetReg value:{:#x}, new value:{:#x}",
      io_->getName(),
      qsfp,
      originalResetReg,
      newResetReg);
  io_->write(kFacebookFpgaQsfpResetReg, newResetReg);

  // Wait for 10ms
  std::this_thread::sleep_for(std::chrono::milliseconds(10));

  // Release the QSFP reset
  newResetReg = ~(0x1 << qsfp) & originalResetReg;
  io_->write(kFacebookFpgaQsfpResetReg, newResetReg);
}

// This function will bring all the transceivers out of reset.
void FbDomFpga::clearAllTransceiverReset() {
  std::lock_guard<std::mutex> g(qsfpResetRegLock_);
  XLOG(DBG5) << "Clearing all transceiver out of reset.";
  // For each bit, 1 to hold QSFP reset active. 0 to release QSFP reset.
  io_->write(kFacebookFpgaQsfpResetReg, 0x0);
}

void FbDomFpga::setFrontPanelLedColor(int qsfp, FbDomFpga::LedColor ledColor) {
  uint32_t qsfpLedAddress = getPortLedAddress(qsfp);
  io_->write(qsfpLedAddress, static_cast<uint32_t>(ledColor));

  XLOG(DBG5) << folly::format(
      "Set {} qsfp={:d} LED to {:s}. Register={:#x}, new value={:#x}",
      io_->getName(),
      qsfp,
      getColorStr(ledColor),
      qsfpLedAddress,
      static_cast<uint16_t>(ledColor));
}

FbDomFpga::PimType FbDomFpga::getPimType() {
  uint32_t curPimTypeReg =
      io_->read(kFacebookFpgaPimTypeReg) & kFacebookFpgaPimTypeBase;
  switch (curPimTypeReg) {
    case static_cast<uint32_t>(FbDomFpga::PimType::MINIPACK_16Q):
    case static_cast<uint32_t>(FbDomFpga::PimType::MINIPACK_16O):
      return static_cast<FbDomFpga::PimType>(curPimTypeReg);
    case kFacebookFpgaPimTypeBase:
      throw FbossError(
          "Error in reading PIM Type from DOM FPGA, register value:",
          curPimTypeReg);
    default:
      throw FbossError(
          "Unrecognized pim type with register value:", curPimTypeReg);
  }
}

} // namespace facebook::fboss
