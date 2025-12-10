// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/led_service/LedUtils.h"
#include <folly/logging/xlog.h>
#include <gflags/gflags.h>
#include <iostream>
#include "fboss/agent/FbossError.h"
#include "fboss/led_service/LedConfig.h"
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

void initFlagDefaults(int argc, char** argv) {
  // one pass over flags, but don't clear argc/argv. We only do this
  // to extract the 'led_config' arg.
  gflags::ParseCommandLineFlags(&argc, &argv, false);

  try {
    auto ledConfig = LedConfig::fromDefaultFile();
    for (const auto& item :
         *ledConfig->thriftConfig_.defaultCommandLineArgs()) {
      // logging not initialized yet, need to use std::cerr
      std::cerr << "Overriding default flag from config: " << item.first.c_str()
                << "=" << item.second.c_str() << std::endl;
      gflags::SetCommandLineOptionWithMode(
          item.first.c_str(), item.second.c_str(), gflags::SET_FLAGS_DEFAULT);
    }
  } catch (FbossError& e) {
    XLOG(ERR) << "Setting default args failed: " << e.what();
  }
}

} // namespace facebook::fboss::utility
