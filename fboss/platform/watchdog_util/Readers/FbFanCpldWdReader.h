// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/platform/watchdog_util/WatchdogReader.h"

namespace facebook::fboss::platform::watchdog_util {

// Covers fbfancpld, mp3_fancpld, w800_fancpld, icecube_fancpld,
// anacapa_fancpld, tahansb_fancpld, leh800b_pdbcpld, etc. All share register
// layout:
//   0x1F CTRL0: BIT4 ENABLE, BIT5 PET, BIT6 TIMEOUT/EXPIRED (mp3 only)
//   0x20 CTRL1: timeout = reg * 5 seconds
//   0x21 STA: counter, timeleft = (max_timeout - counter) * 5
//   0x22 PWM (ignored)
class FbFanCpldWdReader : public WatchdogReader {
 public:
  WatchdogInfo read(const ResolvedWatchdog& resolved) override;
};

} // namespace facebook::fboss::platform::watchdog_util
