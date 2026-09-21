// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include "fboss/agent/if/gen-cpp2/ctrl_types.h"
#include "fboss/agent/types.h"
#include "fboss/lib/fpga/MinipackLed.h"

namespace facebook::fboss {

class MinipackLedUtils {
 public:
  static MinipackLed::Color getLedState(bool up, bool adminUp);
  static MinipackLed::Color getLedExternalState(
      PortLedExternalState lfs,
      MinipackLed::Color internalState);
};

} // namespace facebook::fboss
