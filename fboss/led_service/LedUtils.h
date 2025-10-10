// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/led_service/if/gen-cpp2/led_structs_types.h"

namespace facebook::fboss::utility {
facebook::fboss::led::LedState constructLedState(
    facebook::fboss::led::LedColor ledColor,
    facebook::fboss::led::Blink blink);

void initFlagDefaults(int argc, char** argv);
} // namespace facebook::fboss::utility
