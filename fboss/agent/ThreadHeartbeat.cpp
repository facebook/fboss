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
#include "fboss/agent/ThreadHeartbeat.h"
#include <folly/logging/xlog.h>

using namespace std::chrono;

namespace facebook::fboss {

void ThreadHeartbeat::timeoutExpired() noexcept {
  CHECK(evb_->inRunningEventBaseThread());
  auto now = steady_clock::now();
  auto elapsed = duration_cast<milliseconds>(now - lastTime_);
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

} // namespace facebook::fboss
