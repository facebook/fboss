// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/lib/fpga/Wedge400Fpga.h"

#include <folly/Singleton.h>
#include <folly/logging/xlog.h>

#include "fboss/agent/FbossError.h"
#include "fboss/lib/PciAccess.h"

namespace {
constexpr uint32_t kLeftFpgaBaseAddr = 0xfbd00000;
constexpr uint32_t kRightFpgaBaseAddr = 0xfbb00000;
constexpr uint32_t kFacebookDomFpgaSize = 0x8000;
constexpr uint32_t kTopPortLedBase = 0x0350;
constexpr uint32_t kBotPortLedBase = 0x0310;

const std::string leftFpgaDevPath = "/sys/bus/pci/devices/0000:05:00.0";
const std::string rightFpgaDevPath = "/sys/bus/pci/devices/0000:08:00.0";

// Clears the nth 5 bit block in the prevLedStates
inline uint32_t clearNthLedState(uint32_t prevLedStates, uint8_t n) {
  return (prevLedStates & ~(0b11111 << n * 5));
}

// sets the nth 5 bit block in prevLedStates to newState.
inline uint32_t
setNthState(uint32_t prevLedStates, uint8_t n, uint8_t newState) {
  CHECK_EQ((newState & ~0b11111), 0);
  return clearNthLedState(prevLedStates, n) | (newState << n * 5);
}

inline uint32_t getNthState(uint32_t ledStates, uint8_t n) {
  return (ledStates >> n * 5) & 0b11111;
}
} // unnamed namespace

namespace facebook {
namespace fboss {

folly::Singleton<Wedge400Fpga> _wedge400Fpga;
std::shared_ptr<Wedge400Fpga> Wedge400Fpga::getInstance() {
  return _wedge400Fpga.try_get();
}

Wedge400Fpga::Wedge400Fpga() {
  // Initialize FPGA. to distinguish the two FPGAs, name the left as pim 1 and
  // the right as pim 2.

  // The FPGA device on this platform needs memory space access before
  // using. So here we do that first before anything else.
  auto leftFpgaAccess = PciAccess(leftFpgaDevPath);
  auto rightFpgaAccess = PciAccess(rightFpgaDevPath);

  leftFpgaAccess.enableMemSpaceAccess();
  rightFpgaAccess.enableMemSpaceAccess();

  // Create FpgaDevices using hardcoded address because we have 2 Fpga devices
  // with the same vendor/device id. Change this when Fpga can support device
  // topology
  auto leftDevice =
      std::make_unique<FpgaDevice>(kLeftFpgaBaseAddr, kFacebookDomFpgaSize);
  auto rightDevice =
      std::make_unique<FpgaDevice>(kRightFpgaBaseAddr, kFacebookDomFpgaSize);
  leftDevice->mmap();
  rightDevice->mmap();

  impl = std::make_unique<Wedge400FpgaImpl>(
      std::move(leftDevice), std::move(rightDevice));
}

Wedge400FpgaImpl::Wedge400FpgaImpl(
    std::unique_ptr<FpgaDevice> leftDevice,
    std::unique_ptr<FpgaDevice> rightDevice)
    : leftDevice_(std::move(leftDevice)), rightDevice_(std::move(rightDevice)) {
  leftFpga_ = std::make_unique<FbDomFpga>(std::make_unique<FpgaMemoryRegion>(
      "leftFpga", leftDevice_.get(), 0, leftDevice_->getSize()));

  rightFpga_ = std::make_unique<FbDomFpga>(std::make_unique<FpgaMemoryRegion>(
      "rightFpga", rightDevice_.get(), 0, rightDevice_->getSize()));
}

uint32_t Wedge400FpgaImpl::read(TransceiverID tcvr, uint32_t offset) const {
  return getFpga(tcvr)->read(offset);
}

void Wedge400FpgaImpl::write(
    TransceiverID tcvr,
    uint32_t offset,
    uint32_t value) {
  getFpga(tcvr)->write(offset, value);
}

/**  For more information on the LED logic see
 *  https://fb.quip.com/7Vt0ALatyIJL
 */
void Wedge400FpgaImpl::setLedStatus(
    TransceiverID tcvr,
    uint8_t channel,
    FbDomFpga::LedColor color) {
  std::lock_guard<std::mutex> guard(locks_[tcvrLedGroup(tcvr)]);
  const uint8_t state = static_cast<uint8_t>(color);
  if (tcvr < 16) { // top module
    const uint32_t newD1Led =
        state | (state << 10) | (state << 15) | (state << 20) | (state << 25);
    write(tcvr, getPortLedAddress(tcvr), newD1Led);
    const auto q1Tcvr = TransceiverID(2 * tcvr + 16);
    const uint32_t oldQ1Led = read(tcvr, getPortLedAddress(q1Tcvr));
    const uint32_t newQ1Led = setNthState(oldQ1Led, 1, state);
    write(tcvr, getPortLedAddress(q1Tcvr), newQ1Led);
  } else {
    const uint32_t oldLed = read(tcvr, getPortLedAddress(tcvr));
    uint32_t newLed = setNthState(oldLed, channel + 2, state);
    if (channel == 0) {
      newLed = setNthState(newLed, 0, state);
    }
    write(tcvr, getPortLedAddress(tcvr), newLed);
  }
}

FbDomFpga::LedColor Wedge400FpgaImpl::getLedStatus(
    TransceiverID tcvr,
    uint8_t channel) const {
  const uint32_t led = read(tcvr, getPortLedAddress(tcvr));
  // need to add 2 because for bottom leds first 2 states are reserved
  // top led is set to states 1, 3, 4, and 5. channel == 0 if this is top tcvr
  // so we can just get state 2 for simplicty
  return static_cast<FbDomFpga::LedColor>(getNthState(led, channel + 2));
}

void Wedge400FpgaImpl::triggerQsfpHardReset(TransceiverID tcvr) {
  getFpga(tcvr)->triggerQsfpHardReset(getFpgaPortIndex(tcvr));
}

void Wedge400FpgaImpl::clearAllTransceiverReset() {
  leftFpga_->clearAllTransceiverReset();
  rightFpga_->clearAllTransceiverReset();
}

/**
 *  The transceiver ids are laid out as follows on the front panel:
 *      0 -- 7  ,  8  --  15
 *     16 -- 31 , 32  --  47
 *  The mapping logic is based on this:
 *  transceiver [0, 7] + [16, 31] are accessed through the leftFpga and
 *  [8, 15] + [32, 47] are accessed through rightFpga. Furthermore [0, 15] are
 *  QSFP-DD uplink ports and [16, 47] are downlink QSFP ports.
 */
FbDomFpga* Wedge400FpgaImpl::getFpga(TransceiverID tcvr) const {
  CHECK_LT(tcvr, 48);
  return tcvrLedGroup(tcvr) < 8 ? leftFpga_.get() : rightFpga_.get();
}
/**
 * [0 .. 7] returns [16 .. 23]
 * [8 .. 15] return [16 .. 23]
 * [16 .. 31] returns [0 .. 15]
 * [32 .. 47] returns [0 .. 15]
 */
inline uint32_t Wedge400FpgaImpl::getFpgaPortIndex(TransceiverID tcvr) const {
  CHECK_LT(tcvr, 48);
  if (tcvr < 8) {
    return tcvr + 16;
  } else if (tcvr < 16) {
    return tcvr + 8;
  } else if (tcvr < 32) {
    return tcvr - 16;
  } else {
    return tcvr - 32;
  }
}

/**
 *  LED settings are 4 byte blocks with the following offsets:
 *  [0x0350, 0x038C] are the offsets for the top leds. There are two LEDs per
 *  module but we only use the left one so we need to skip every other 4 byte
 *  block.
 *  [0x0310, 0x034C] are the offsets for the bottom leds, one for each
 *  module.
 */
inline uint32_t Wedge400FpgaImpl::getPortLedAddress(TransceiverID tcvr) const {
  CHECK_LT(tcvr, 48);
  if (tcvr < 8) {
    return kTopPortLedBase + tcvr * 8;
  } else if (tcvr < 16) {
    return kTopPortLedBase + (tcvr - 8) * 8;
  } else if (tcvr < 32) {
    return kBotPortLedBase + (tcvr - 16) * 4;
  } else {
    return kBotPortLedBase + (tcvr - 32) * 4;
  }
}

/**  We define each group of 4 LEDs into a group and have a lock for each group
 *  so we can read and write multiple LEDs in the same group without a race
 *  condition from another port
 */
inline uint32_t Wedge400FpgaImpl::tcvrLedGroup(TransceiverID tcvr) const {
  CHECK_LT(tcvr, 48);
  return tcvr < 16 ? tcvr : (tcvr - 16) / 2;
}

} // namespace fboss
} // namespace facebook
