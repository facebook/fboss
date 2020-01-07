/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#pragma once

#include "fboss/lib/i2c/PCA9541.h"
#include "fboss/lib/usb/PCA9548MuxedBus.h"

namespace facebook::fboss {
// Handle the intricacies of routing I2C requests to appropriate QSFPs,
// based on the hardware.

class GalaxyI2CBus : public PCA9548MuxedBus<16> {
 public:
  /* Galaxy: The QSFP CPLD sits behind the I2C Mux PCA9541 which is two upstream
   * to one downstream mux. This needs to be controlled to get the QSFP CPLD
   * access from CP2112 or from BMC. The PCA9541 can be accessed from CP2112
   * at address 0x74
   */
  enum : uint8_t { ADDR_I2CMUX_PCA9541 = 0x74 };

  GalaxyI2CBus()
      : pca9541Bus_(std::make_unique<PCA9541>(this, ADDR_I2CMUX_PCA9541)) {}

  /* Trigger the QSFP hard reset for a given QSFP module in the Galaxy SWE.
   * This function access the QSFP CPLD to do the QSFP reset in Galaxy SWE.
   * The QSFP CPLD sits behind the PCA9541 which is a 2 upstream to 1 downstream
   * mux. The CP2112 need to get the I2C bus access towards QSFP CPLD before
   * trying to read/write that and this function takes care of that part as well
   */
  void triggerQsfpHardReset(unsigned int module) override;

 private:
  MuxLayer createMuxes() override;
  void wireUpPorts(PortLeaves& leaves) override;

  // In Galaxy the QSFP CPLD sits behind the PCA9541 device. Before attempting
  // to read/write to the QSFP CPLD we need to make sure the PCA9541 has
  // provided the downstream dvice access to the controller CPU. For this
  // we need to have PCA9541 object to access that device related functions
  std::unique_ptr<PCA9541> pca9541Bus_;
};

} // namespace facebook::fboss
