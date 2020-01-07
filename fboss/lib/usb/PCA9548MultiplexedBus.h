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

#include <array>
#include "fboss/lib/usb/BaseWedgeI2CBus.h"

namespace facebook::fboss {
// PCA9548MultiplexedBus represents a I2C bus connected
// to a bank of PCA9548 multiplexers. We assume that
// a) multiplexers are at contigous addresses,
// b) QSFPs are ordered w.r.t to multiplexers, so QSFP 1-8 go to
// multiplexer 1, 9-16 go to multplexer 2  and so on
// This is true on the platforms we have. If things change its
// easy to adapt this class to take a mapping of multiplexer
// addresses and QSFP to multiplexer mapping and work off that.

class PCA9548MultiplexedBus : public BaseWedgeI2CBus {
 public:
  using QsfpAddressMap_t = std::array<uint8_t, 8>;
  PCA9548MultiplexedBus(
      uint8_t multiplexerStartAddr,
      uint8_t numMultiplexers,
      unsigned int numPorts,
      QsfpAddressMap_t qsfpAddressMap)
      : // Note that the multiplexer address is shifted one to the left to
        // work with the underlying I2C libraries.
        multiplexerStartAddr_(static_cast<uint8_t>(multiplexerStartAddr << 1)),
        numMultiplexers_(numMultiplexers),
        numPorts_(numPorts),
        qsfpAddressMap_(qsfpAddressMap) {}

 protected:
  void initBus() override;
  void verifyBus(bool autoReset = true) override;
  void selectQsfpImpl(unsigned int module) override;

 private:
  // Forbidden copy constructor and assignment operator
  PCA9548MultiplexedBus(PCA9548MultiplexedBus const&) = delete;
  PCA9548MultiplexedBus& operator=(PCA9548MultiplexedBus const&) = delete;

  const uint8_t multiplexerStartAddr_{0};
  const uint8_t numMultiplexers_{0};
  const unsigned int numPorts_{0};
  const QsfpAddressMap_t qsfpAddressMap_;
};

} // namespace facebook::fboss
