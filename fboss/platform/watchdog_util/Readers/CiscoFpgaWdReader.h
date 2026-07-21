// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/platform/watchdog_util/WatchdogReader.h"

namespace facebook::fboss::platform::watchdog_util {

// Cisco FPGA watchdog (block id 33, wd_regs_v4_t)
// Registers (offsets from csrOffset):
//   ctl     @0x30
//   stg1Ctl @0x34 : [31:16] timeout, [15:14] interval unit (2=seconds), [0]
//   enable stat    @0x40 : [16] stage1 enabled, [17] stage1 expired, [15:0]
//   timerval (timeleft)
class CiscoFpgaWdReader : public WatchdogReader {
 public:
  WatchdogInfo read(const ResolvedWatchdog& resolved) override;
};

} // namespace facebook::fboss::platform::watchdog_util
