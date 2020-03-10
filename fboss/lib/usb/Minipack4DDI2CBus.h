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

#include "fboss/lib/usb/PCA9548MuxedBus.h"

namespace facebook::fboss {
class Minipack4DDI2CBus : public PCA9548MuxedBus<32> {
 private:
  static constexpr uint8_t MODULES_PER_PIM = 4;

  MuxLayer createMuxes() override;
  void wireUpPorts(PortLeaves& leaves) override;
};

} // namespace facebook::fboss
