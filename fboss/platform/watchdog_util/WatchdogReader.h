// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/platform/watchdog_util/WatchdogInfo.h"

namespace facebook::fboss::platform::watchdog_util {

class WatchdogReader {
 public:
  virtual ~WatchdogReader() = default;
  virtual WatchdogInfo read(const ResolvedWatchdog& resolved) = 0;
};

} // namespace facebook::fboss::platform::watchdog_util
