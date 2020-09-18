// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include "fboss/agent/types.h"

namespace facebook::fboss {

class GalaxyLedUtils {
 public:
  static void getDesiredLEDState(uint32_t* state, bool up, bool adminUp);
  static int getPortIndex(PortID physicalPort);
  static size_t getPortOffset(int index);
  static folly::ByteRange defaultLed0Code();
  static folly::ByteRange defaultLed1Code();
  static std::optional<uint32_t> getLEDProcessorNumber(PortID port);
  static uint32_t getExpectedLEDState(bool up, bool /*adminUp*/) {
    if (up) {
      return 3;
    }
    return 2;
  }
};

} // namespace facebook::fboss
