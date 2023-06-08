// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.
#include <chrono>

#include <folly/logging/xlog.h>

#include "fboss/platform/platform_manager/PlatformExplorer.h"

namespace facebook::fboss::platform::platform_manager {

PlatformExplorer::PlatformExplorer(std::chrono::seconds exploreInterval) {
  scheduler_.addFunction([this]() { explore(); }, exploreInterval);
  scheduler_.start();
}

void PlatformExplorer::explore() {
  XLOG(INFO) << "Exploring the device";
}

} // namespace facebook::fboss::platform::platform_manager
