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

namespace {

enum GalaxyMuxes {
  MUX_0_TO_7 = 0x70,
  MUX_8_TO_15 = 0x71,
};

}

namespace facebook {
namespace fboss {

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

}
} // facebook::fboss
