/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/lib/usb/GalaxyI2CBus.h"

#include <folly/container/Enumerate.h>
#include <folly/logging/xlog.h>

namespace {

enum GalaxyMuxes {
  MUX_0_TO_7 = 0x70,
  MUX_8_TO_15 = 0x71,
};

/* Galaxy: In each switching element (SWE) the port 1-16 represents the
 * front panel port and their QSFP reset is controlled by QSFP CPLD.
 * The QSFP CPLD register I2C address is 0x39. The two registers at 0x10
 * and 0x11 controls 8 ports QSFP reset each.
 * QSFP presence is indicated by QSFP CPLD registers 0x16-0x17
 * Each of these registers controls presence for 8 QSFP modules
 */
enum : uint8_t { ADDR_QSFP_CPLD = 0x39 };
enum GalaxyQsfpCpldReg {
  QSFP_RESET_REG_0 = 0x10,
  QSFP_ABS_STA_0 = 0x16,
  QSFP_ABS_STA_1 = 0x17
};

void extractPresenceBits(
    uint8_t* buf,
    std::map<int32_t, facebook::fboss::ModulePresence>& presences,
    bool upperQsfps) {
  for (int bit = 0; bit < 8; bit++) {
    int tcvrIdx = bit;
    if (upperQsfps) {
      tcvrIdx += 8;
    }
    ((buf[0] >> bit) & 1)
        ? presences[tcvrIdx] = facebook::fboss::ModulePresence::ABSENT
        : presences[tcvrIdx] = facebook::fboss::ModulePresence::PRESENT;
  }
}
} // namespace

namespace facebook::fboss {
bool GalaxyI2CBus::isPresent(unsigned int module) {
  // Each Galaxy SWE takes care of 16 QSFP so check the QSFP module id first
  if (module > 16 || module == 0) {
    XLOG(DBG0) << "Presence for module " << module
               << " can't be checked, the value needs to be in between 1..16";
    return false; // TODO: Return unknown instead
  }
  // In Galaxy the QSFP CPLD sits behind the PCA9541 device. Before attempting
  // to read/write to the QSFP CPLD we need to make sure the PCA9541 has
  // provided the downstream device access to this CP2112 CPU.
  // Check if the CP2112 have the I2C bus access towards thew QSFP CPLD. In
  // case it does not have then try to get access by writing the control
  // register of PCA device. If the bus control could not be obtained then
  // we can't proceed so return from here
  if (!pca9541Bus_->getBusControl()) {
    XLOG(DBG0)
        << "Bus control for PCA9541 could not be obtained, try again later";
    return false; // TODO: Return unknown instead
  }

  uint8_t buf;
  auto cpldReg = QSFP_ABS_STA_0 + (module - 1) / 8;
  auto cpldRegBit = (module - 1) % 8;

  try {
    // Read QSFP CPLD for QSFP presence indications
    read(ADDR_QSFP_CPLD, cpldReg, 1, &buf);

    // Return the presence status (0-Present, 1-Absent)
    return !((buf >> cpldRegBit) & 1);
  } catch (const std::exception& ex) {
    XLOG(ERR) << "Exception " << ex.what()
              << " raised while reading QSFP presence for module " << module;
    return false; // TODO: Return unknown instead
  }
}

void GalaxyI2CBus::scanPresence(std::map<int32_t, ModulePresence>& presences) {
  // In Galaxy the QSFP CPLD sits behind the PCA9541 device. Before attempting
  // to read/write to the QSFP CPLD we need to make sure the PCA9541 has
  // provided the downstream dvice access to this CP2112 CPU.
  // Check if the CP2112 have the I2C bus access towards thew QSFP CPLD. In
  // case it does not have then try to get access by writing the control
  // register of PCA device. If the bus control could not be obtained then
  // we can't proceed so return from here
  if (!pca9541Bus_->getBusControl()) {
    XLOG(DBG0)
        << "Bus control for PCA9541 could not be obtained, try again later";
    return;
  }
  uint8_t buf;
  try {
    read(ADDR_QSFP_CPLD, QSFP_ABS_STA_0, 1, &buf);
    extractPresenceBits(&buf, presences, false);
  } catch (const std::exception& ex) {
    XLOG(ERR) << "Exception " << ex.what()
              << " raised while reading QSFP presence of QSFPs 1-8";
  }

  try {
    read(ADDR_QSFP_CPLD, QSFP_ABS_STA_1, 1, &buf);
    extractPresenceBits(&buf, presences, true);
  } catch (const std::exception& ex) {
    XLOG(ERR) << "Exception " << ex.what()
              << " raised while reading QSFP presence of QSFPs 9-16";
  }
}

MuxLayer GalaxyI2CBus::createMuxes() {
  MuxLayer muxes;
  muxes.push_back(std::make_unique<QsfpMux>(dev_.get(), MUX_0_TO_7));
  muxes.push_back(std::make_unique<QsfpMux>(dev_.get(), MUX_8_TO_15));
  return muxes;
}

void GalaxyI2CBus::wireUpPorts(GalaxyI2CBus::PortLeaves& leaves) {
  for (auto&& mux : folly::enumerate(roots_)) {
    connectPortsToMux(leaves, (*mux).get(), mux.index * PCA9548::WIDTH);
  }
}

/* Trigger the QSFP hard reset for a given QSFP module in the Galaxy SWE.
 * This function access the QSFP CPLD to do the QSFP reset in Galaxy SWE.
 * The QSFP CPLD sits behind the PCA9541 which is a 2 upstream to 1 downstream
 * mux. The CP2112 need to get the I2C bus access towards QSFP CPLD before
 * trying to read/write that and this function takes care of that part as well
 */
void GalaxyI2CBus::triggerQsfpHardReset(unsigned int module) {
  uint8_t buf;

  // Each Galaxy SWE takes care of 16 QFP so check the QSFP module id first
  if (module > 16) {
    XLOG(DBG0) << "module " << module
               << " can't be reset, the value needs to be in between 1..16";
    return;
  }

  // In Galaxy the QSFP CPLD sits behind the PCA9541 device. Before attempting
  // to read/write to the QSFP CPLD we need to make sure the PCA9541 has
  // provided the downstream dvice access to this CP2112 CPU.
  // Check if the CP2112 have the I2C bus access towards thew QSFP CPLD. In
  // case it does not have then try to get access by writing the control
  // register of PCA device. If the bus control could not be obtained then
  // we can't proceed so return from here
  if (!pca9541Bus_->getBusControl()) {
    XLOG(DBG0)
        << "Bus control for PCA9541 could not be obtained, try again later";
    return;
  }

  // In Galaxy the QSFP reset is controlled by QSFP CPLD register 0x10-0x11
  // Each of these register controls 8 QSFP module reset
  auto cpldReg = QSFP_RESET_REG_0 + (module - 1) / 8;
  auto cpldRegBit = (module - 1) % 8;

  XLOG(DBG0) << "Resetting the QSFP for port " << module;

  // Read QSFP CPLD for QSFP reset current values
  read(ADDR_QSFP_CPLD, cpldReg, 1, &buf);

  XLOG(DBG0) << "Read value from QSFP CPLD Reg " << cpldReg << ": " << buf;

  // Put the QSFP in reset state (0-Reset, 1-Normal)
  buf &= ~(1 << cpldRegBit);
  write(ADDR_QSFP_CPLD, cpldReg, 1, &buf);

  // Wait for 10ms
  std::this_thread::sleep_for(std::chrono::milliseconds(10));

  // Take the QSFP out of reset state (0-Reset, 1-Normal)
  buf |= (1 << cpldRegBit);
  write(ADDR_QSFP_CPLD, cpldReg, 1, &buf);
}

} // namespace facebook::fboss
