// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include "fboss/agent/types.h"

#include "fboss/agent/if/gen-cpp2/ctrl_types.h"
#include "fboss/qsfp_service/if/gen-cpp2/transceiver_types.h"

namespace facebook::fboss {

class Wedge100LedUtils {
 public:
  enum class LedColor : uint32_t {
    OFF = 0b000,
    BLUE = 0b001,
    GREEN = 0b010,
    CYAN = 0b011,
    RED = 0b100,
    MAGENTA = 0b101,
    YELLOW = 0b110,
    WHITE = 0b111,
  };

  static int getPipe(PortID port);
  static int getPortIndex(PortID port);
  static std::pair<int, int>
  getCompactPortIndexes(PortID port, bool isTop, bool isQuad);
  static LedColor getDesiredLEDState(int numberOfLanes, bool up, bool adminUp);
  static LedColor getDesiredLEDState(
      PortLedExternalState externalState,
      LedColor currentColor);
  static bool isTop(std::optional<TransceiverID> transciver);
  static folly::ByteRange defaultLedCode();
  static size_t getPortOffset(int index);
  static std::optional<uint32_t> getLEDProcessorNumber(PortID port);
  static LedColor
  getExpectedLEDState(int numberOfLanes, bool up, bool adminUp) {
    if (up && adminUp) {
      switch (numberOfLanes) {
        case 4: // Quaid
          return LedColor::BLUE;
        case 2: // DUAL
          return LedColor::MAGENTA;
        case 1: // Single
          return LedColor::GREEN;
      }
    }
    return LedColor::OFF;
  }
};

} // namespace facebook::fboss
