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

#include "fboss/lib/usb/PCA9548MultiplexedBus.h"

namespace facebook {
namespace fboss {

// Handle the intricacies of routing I2C requests to appropriate QSFPs,
// based on the hardware.

class Wedge100I2CBus : public PCA9548MultiplexedBus {
  enum : uint8_t {
    MULTIPLEXERS = 5,
    MULTIPLEXERS_START_ADDR = 0x70,
    NUM_PORTS = 32,
  };
  // QSFP_ADDR_MAP represents PCA 9548 multiplexer pin address
  // to QSFP mapping. Entry 0 in this array maps to first
  // QSFP handled by this multiplexer
  static constexpr QsfpAddressMap_t QSFP_ADDR_MAP = {{
      0x2,
      0x1,
      0x8,
      0x4,
      0x20,
      0x10,
      0x80,
      0x40,
  }};

 public:
  Wedge100I2CBus()
      : PCA9548MultiplexedBus(
            MULTIPLEXERS_START_ADDR,
            MULTIPLEXERS,
            NUM_PORTS,
            QSFP_ADDR_MAP) {}
  ~Wedge100I2CBus() override {}

 private:
  // Forbidden copy constructor and assignment operator
  Wedge100I2CBus(Wedge100I2CBus const&) = delete;
  Wedge100I2CBus& operator=(Wedge100I2CBus const&) = delete;
};
}
} // facebook::fboss
