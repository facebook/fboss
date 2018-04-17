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

namespace {

enum Wedge100Muxes {
  MUX_0_TO_7 = 0x70,
  MUX_8_TO_15 = 0x71,
  MUX_16_TO_23 = 0x72,
  MUX_24_TO_31 = 0x73
};

}

namespace facebook {
namespace fboss {

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

} // namespace fboss
} // namespace facebook
