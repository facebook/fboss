// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include "fboss/agent/if/gen-cpp2/ctrl_types.h"
#include "fboss/agent/types.h"
#include "fboss/lib/fpga/FbDomFpga.h"

namespace facebook::fboss {

class Wedge400LedUtils {
 public:
  static FbDomFpga::LedColor
  getLedState(uint8_t numLanes, bool up, bool adminUp);
  static FbDomFpga::LedColor getLedExternalState(PortLedExternalState lfs);
};

} // namespace facebook::fboss
