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

namespace facebook { namespace fboss {

// Handle the intricacies of routing I2C requests to appropriate QSFPs,
// based on the hardware.

class Wedge100I2CBus : public BaseWedgeI2CBus {
 public:
  Wedge100I2CBus();
  virtual ~Wedge100I2CBus() {}

 protected:
  virtual void initBus() override;
  virtual void verifyBus(bool autoReset = true) override;
  virtual void selectQsfpImpl(unsigned int module) override;

 private:
  enum : uint8_t {
    MULTIPLEXERS = 4,
    // The PCA9548 switch selecting for 32 port controllers
    // Note that the address is shifted one to the left to
    // work with the underlying I2C libraries.
    ADDR_SWITCH_32 = 0xe0,
  };

  // Forbidden copy constructor and assignment operator
  Wedge100I2CBus(Wedge100I2CBus const &) = delete;
  Wedge100I2CBus& operator=(Wedge100I2CBus const &) = delete;
};

}} // facebook::fboss
