// Copyright 2014-present Facebook. All Rights Reserved.

#include "fboss/platform/data_corral_service/ChassisManager.h"

using namespace std::chrono;

namespace facebook::fboss::platform::data_corral_service {

void ChassisMonitor::timeoutExpired() noexcept {
  CHECK(evb_->inRunningEventBaseThread());
  auto now = steady_clock::now();
  func_();
  lastTime_ = now;
  scheduleTimeout(refreshInterval_);
}

void ChassisMonitor::scheduleFirstRefresh() {
  CHECK(evb_->inRunningEventBaseThread());
  lastTime_ = std::chrono::steady_clock::now();
  scheduleTimeout(refreshInterval_);
}

} // namespace facebook::fboss::platform::data_corral_service
