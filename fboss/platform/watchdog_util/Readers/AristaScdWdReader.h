// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include <cstdint>

#include "fboss/platform/watchdog_util/WatchdogReader.h"

namespace facebook::fboss::platform::watchdog_util {

// SCD watchdog_darwin CSR offsets (@0x120 primary, @0x304 secondary). Used as a
// heuristic to pick the SCD reader when the config does not explicitly resolve
// the watchdog family.
constexpr uint64_t kScdCsrOffsetPrimary = 0x120;
constexpr uint64_t kScdCsrOffsetSecondary = 0x304;

inline bool isScdCsrOffset(uint64_t csrOffset) {
  return csrOffset == kScdCsrOffsetPrimary ||
      csrOffset == kScdCsrOffsetSecondary;
}

// Arista SCD watchdog_darwin @0x120 and @0x304
// 32-bit register:
//   BIT31 ENABLE
//   [30:29] ACTION (3)
//   [27:16] PRE_TIMEOUT (0xFFF)
//   [15:0]  TIMEOUT
// No explicit timeleft or expired in driver (driver disables WDT).
// For debug, we report timeleft = timeout if enabled else 0, expired = false.
class AristaScdWdReader : public WatchdogReader {
 public:
  WatchdogInfo read(const ResolvedWatchdog& resolved) override;
};

} // namespace facebook::fboss::platform::watchdog_util
