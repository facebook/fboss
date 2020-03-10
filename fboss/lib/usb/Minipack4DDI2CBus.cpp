/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/lib/usb/Minipack4DDI2CBus.h"

#include <folly/container/Enumerate.h>

namespace {

enum Minipack4DDPimMuxes {
  MUX_PIM_SELECT = 0x70,
  MUX_0_TO_3 = 0x71,
};

}

namespace facebook::fboss {
MuxLayer Minipack4DDI2CBus::createMuxes() {
  auto root = std::make_unique<QsfpMux>(dev_.get(), MUX_PIM_SELECT);
  for (uint8_t i = 0; i < PCA9548::WIDTH; ++i) {
    root->registerChildMux(dev_.get(), i, MUX_0_TO_3);
  }
  MuxLayer layer;
  layer.push_back(std::move(root));
  return layer;
}

void Minipack4DDI2CBus::wireUpPorts(Minipack4DDI2CBus::PortLeaves& leaves) {
  auto& root = roots_.front();
  for (int i = 0; i < PCA9548::WIDTH; ++i) {
    // TODO: determine if we need to flip the ports or do anything like that.
    auto& mux = root->children(i).front();
    connectPortsToMux(
        leaves, mux.get(), i * MODULES_PER_PIM, false, false, MODULES_PER_PIM);
  }
}

} // namespace facebook::fboss
