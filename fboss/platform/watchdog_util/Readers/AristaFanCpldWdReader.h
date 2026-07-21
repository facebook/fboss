// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/platform/watchdog_util/WatchdogReader.h"

namespace facebook::fboss::platform::watchdog_util {

// Arista fan-cpld-i2c watchdog
//   0x06 WDT_ENABLE (1=enabled)
//   0x07 WDT_BOOST_PWM (ignored)
//   0x08 WDT_COUNTER (timeleft, written with timeout on start/ping)
//   0x0c WDT_TIMEOUT (max timeout, multiples of 5 seconds per comment)
// Max timeout 0xFF
class AristaFanCpldWdReader : public WatchdogReader {
 public:
  WatchdogInfo read(const ResolvedWatchdog& resolved) override;
};

} // namespace facebook::fboss::platform::watchdog_util
