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

#include "fboss/lib/usb/BaseWedgeI2CBus.h"

namespace facebook::fboss {
// Handle the intricacies of routing I2C requests to appropriate QSFPs,
// based on the hardware.

class WedgeI2CBus : public BaseWedgeI2CBus {
 public:
  WedgeI2CBus();
  ~WedgeI2CBus() override {}

 protected:
  void initBus() override;
  void verifyBus(bool autoReset = true) override;
  void selectQsfpImpl(unsigned int module) override;

 private:
  enum : uint8_t {
    // Note that these addresses are shifted one to the left to
    // work with the underlying I2C libraries.
    ADDR_FPGA = 0x54,
    ADDR_EEPROM = 0xa2,
    // The PCA9548 switch selecting the first 8 QSFPs
    ADDR_SWITCH_1 = 0xe8,
    // The PCA9548 switch selecting the second 8 QSFPs
    ADDR_SWITCH_2 = 0xec,
  };

  // Forbidden copy constructor and assignment operator
  WedgeI2CBus(WedgeI2CBus const&) = delete;
  WedgeI2CBus& operator=(WedgeI2CBus const&) = delete;

  std::pair<uint8_t, uint8_t> getSwitchValues(unsigned int module) const;
};

} // namespace facebook::fboss
