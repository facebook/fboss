/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
// Copyright 2014-present Facebook. All Rights Reserved.
#include "fboss/lib/ThreadHeartbeat.h"
#include <folly/logging/xlog.h>

using namespace std::chrono;

namespace facebook::fboss {

void ThreadHeartbeat::timeoutExpired() noexcept {
  CHECK(evb_->inRunningEventBaseThread());
  auto now = steady_clock::now();
  auto elapsed = duration_cast<milliseconds>(
      now - lastTime_.load(std::memory_order_relaxed));
  auto delay = elapsed - intervalMsecs_;
  auto evbQueueSize = evb_->getNotificationQueueSize();
  heartbeatStatsFunc_(delay.count(), evbQueueSize);
  if (delay.count() > delayThresholdMsecs_ ||
      evbQueueSize > backlogThreshold_) {
    XLOG(DBG3) << threadName_ << ": heartbeat elapsed ms:" << elapsed.count()
               << " delay ms:" << delay.count()
               << " event queue size:" << evbQueueSize;
  }
  lastTime_ = now;
  scheduleTimeout(intervalMsecs_);
}

void ThreadHeartbeatWatchdog::watchdogLoop() {
  while (running_) {
    for (auto& heartbeat : heartbeats_) {
      auto timestamp = heartbeat.first->getTimestamp();
      auto paused = heartbeat.first->getMonitoringPaused();
      if (timestamp <= heartbeat.second &&
          timestamp != std::chrono::steady_clock::time_point::min()) {
        XLOG(ERR) << heartbeat.first->getThreadName()
                  << "thread heartbeat missed!";
        if (!paused) {
          heartbeatMissFunc_();
        } else {
          XLOG(INFO)
              << heartbeat.first->getThreadName()
              << "thread heartbeat monitoring paused. Not reporting the missed heartbeat";
        }
      }
      // Update the timestamp only if we are monitoring this thread
      if (!paused) {
        heartbeats_.assign(heartbeat.first, timestamp);
      }
    }
    std::unique_lock<std::mutex> l(m_);
    cv_.wait_for(l, intervalMsecs_, [this]() { return !running_; });
  }
}

} // namespace facebook::fboss
