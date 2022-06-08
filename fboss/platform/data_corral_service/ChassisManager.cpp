// Copyright 2014-present Facebook. All Rights Reserved.

#include "fboss/platform/data_corral_service/ChassisManager.h"

using namespace std::chrono;

namespace facebook::fboss::platform::data_corral_service {

void ChassisManager::timeoutExpired() noexcept {
  CHECK(evb_.inRunningEventBaseThread());
  auto now = steady_clock::now();
  refreshFruModules();
  programChassis();
  lastTime_ = now;
  scheduleTimeout(refreshInterval_);
}

void ChassisManager::scheduleFirstRefresh() {
  CHECK(evb_.inRunningEventBaseThread());
  lastTime_ = std::chrono::steady_clock::now();
  scheduleTimeout(refreshInterval_);
}

} // namespace facebook::fboss::platform::data_corral_service
