/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/lib/usb/Wedge100I2CBus.h"

#include <folly/container/Enumerate.h>
#include <folly/logging/xlog.h>

namespace {

enum Wedge100Muxes {
  MUX_0_TO_7 = 0x70,
  MUX_8_TO_15 = 0x71,
  MUX_16_TO_23 = 0x72,
  MUX_24_TO_31 = 0x73,
  MUX_PCA9548_5 = 0x74
};

enum Wedge100StatusMuxChannels { PRSNT_N_LOW = 2, PRSNT_N_HIGH = 3 };

// Wedge100: SYSTEM CPLD register offset 0x34 to 0x37 controls the QSFP reset
// for the 32 QSFP modules. Each CPLD register controls reset for 8 QSFP
// modules in this platform
enum Wedge100QsfpResetReg { QSFP_RESET_REG_0 = 0x34 };

// Addresses unique to wedge100:
enum : uint8_t {
  ADDR_QSFP_PRSNT_LOWER = 0x22,
  ADDR_QSFP_PRSNT_UPPER = 0x23,
  ADDR_SYSTEM_CPLD = 0x32
};

void extractPresenceBits(
    uint8_t* buf,
    std::map<int32_t, facebook::fboss::ModulePresence>& presences,
    bool upperQsfps) {
  for (int byte = 0; byte < 2; byte++) {
    for (int bit = 0; bit < 8; bit++) {
      int tcvrIdx = byte * 8 + bit;
      if (upperQsfps) {
        tcvrIdx += 16;
      }
      ((buf[byte] >> bit) & 1)
          ? presences[tcvrIdx] = facebook::fboss::ModulePresence::ABSENT
          : presences[tcvrIdx] = facebook::fboss::ModulePresence::PRESENT;
    }
  }
}
} // namespace

namespace facebook::fboss {
void Wedge100I2CBus::scanPresence(
    std::map<int32_t, ModulePresence>& presences) {
  // If selectedPort_ != NO_PORT, then unselectAll
  if (selectedPort_) {
    selectQsfpImpl(NO_PORT);
  }
  uint8_t buf[2];
  try {
    read(ADDR_QSFP_PRSNT_LOWER, 0, 2, buf);
    extractPresenceBits(buf, presences, false);
  } catch (const I2cError& ex) {
    XLOG(ERR) << "Error reading lower QSFP presence bits";
  }

  try {
    read(ADDR_QSFP_PRSNT_UPPER, 0, 2, buf);
    extractPresenceBits(buf, presences, true);
  } catch (const I2cError& ex) {
    XLOG(ERR) << "Error reading upper QSFP presence bits";
  }
}

/* Trigger the QSFP hard reset for a given QSFP module in the wedge100.
 * This function access the CPLD do trigger the hard reset of QSFP module.
 */
void Wedge100I2CBus::triggerQsfpHardReset(unsigned int module) {
  uint8_t buf;

  /* In wedge100 the QSFP reset is controlled by Sys CPLD register 0x34-0x37
   * Each of these register controls 8 QSFP module reset
   */
  auto cpldReg = QSFP_RESET_REG_0 + (module - 1) / 8;
  auto cpldRegBit = (module - 1) % 8;

  // printf ("Resetting QSFP module %d, writing CPLD address 0x%x,
  //  offset 0x%x, bit %d\n", module, ADDR_SYSTEM_CPLD, cpldReg, cpldRegBit);

  // Read system CPLD for QSFP reset current values

  // Get the addtessed read mode from device, whether it is STOP_START mode or
  // the REPEATED_START mode
  auto origReadMode = getWriteReadMode();
  // We want to do the CPLD addressed read in REPEATED_START mode so ff it is
  // currently in STOP_START mode (origReadMode==false) then set the read mode
  // as REPEATED_START mode
  if (origReadMode == WriteReadMode::WriteReadModeStopStart) {
    setWriteReadMode(WriteReadMode::WriteReadModeRepeatedStart);
  }
  // Perform the addressed read from device
  read(ADDR_SYSTEM_CPLD, cpldReg, 1, &buf);
  // If the device read mode was set as REPEATED_START mode earlier then set
  // it back to STOP_START mode
  if (origReadMode == WriteReadMode::WriteReadModeStopStart) {
    setWriteReadMode(WriteReadMode::WriteReadModeStopStart);
  }

  XLOG(DBG0) << "Resetting the QSFP for port " << module
             << ", read value from Sys CPLD Reg " << cpldReg << ": " << buf;

  // Put the QSFP in reset (0-Reset, 1-Normal)
  buf &= ~(1 << cpldRegBit);
  write(ADDR_SYSTEM_CPLD, cpldReg, 1, &buf);

  // Wait for 10ms
  std::this_thread::sleep_for(std::chrono::milliseconds(10));

  // Take the QSFP out of reset (0-Reset, 1-Normal)
  buf |= (1 << cpldRegBit);
  write(ADDR_SYSTEM_CPLD, cpldReg, 1, &buf);
}

void Wedge100I2CBus::initBus() {
  PCA9548MuxedBus<32>::initBus();
  qsfpStatusMux_ = std::make_unique<PCA9548>(dev_.get(), MUX_PCA9548_5);
  qsfpStatusMux_->select((1 << PRSNT_N_LOW) + (1 << PRSNT_N_HIGH));
}

MuxLayer Wedge100I2CBus::createMuxes() {
  MuxLayer muxes;
  muxes.push_back(std::make_unique<QsfpMux>(dev_.get(), MUX_0_TO_7));
  muxes.push_back(std::make_unique<QsfpMux>(dev_.get(), MUX_8_TO_15));
  muxes.push_back(std::make_unique<QsfpMux>(dev_.get(), MUX_16_TO_23));
  muxes.push_back(std::make_unique<QsfpMux>(dev_.get(), MUX_24_TO_31));
  return muxes;
}

void Wedge100I2CBus::wireUpPorts(Wedge100I2CBus::PortLeaves& leaves) {
  for (auto&& mux : folly::enumerate(roots_)) {
    connectPortsToMux(leaves, (*mux).get(), mux.index * PCA9548::WIDTH, true);
  }
}

} // namespace facebook::fboss
