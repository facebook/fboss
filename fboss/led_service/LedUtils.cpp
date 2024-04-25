// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/led_service/LedUtils.h"
#include "fboss/led_service/if/gen-cpp2/led_structs_types.h"

namespace facebook::fboss::utility {

facebook::fboss::led::LedState constructLedState(
    facebook::fboss::led::LedColor ledColor,
    facebook::fboss::led::Blink blink) {
  facebook::fboss::led::LedState ledState;
  ledState.ledColor() = ledColor;
  ledState.blink() = blink;
  return ledState;
}

} // namespace facebook::fboss::utility
