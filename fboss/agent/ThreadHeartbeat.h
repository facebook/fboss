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
#pragma once
#include <folly/io/async/AsyncTimeout.h>
#include <chrono>
#include <folly/io/async/EventBase.h>

namespace facebook { namespace fboss {

class ThreadHeartbeat : private folly::AsyncTimeout {
  /*
   * Send heartbeat at regular interval to thread.  Measure delay between
   * time we expect heartbeat to be processed vs. time actually processed,
   * and record it to ods.
   */
 public:
  ThreadHeartbeat(folly::EventBase* evb, std::string threadName,
                  int intervalMsecs,
                  std::function<void(int, int)> heartbeatStatsFunc) :
      AsyncTimeout(evb),
      evb_(evb),
      threadName_(threadName),
      intervalMsecs_(intervalMsecs),
      heartbeatStatsFunc_(heartbeatStatsFunc) {
    VLOG(2) << "ThreadHeartbeat intervalMsecs:" << intervalMsecs_.count();
    evb_->runInEventBaseThread([this]() {
        scheduleFirstHeartbeat();
      });
  }

  ~ThreadHeartbeat() override {
    evb_->runImmediatelyOrRunInEventBaseThreadAndWait(
      [this]() {
        cancelTimeout();
      });
  }

 private:
  void timeoutExpired() noexcept override;

  void scheduleFirstHeartbeat() {
    CHECK(evb_->inRunningEventBaseThread());
    lastTime_ = std::chrono::steady_clock::now();
    scheduleTimeout(intervalMsecs_);
  }

  folly::EventBase* evb_;
  std::string threadName_;
  std::chrono::milliseconds intervalMsecs_;
  std::function<void(int, int)> heartbeatStatsFunc_;
  std::chrono::time_point<std::chrono::steady_clock> lastTime_;
  //XXX: these thresholds could be made configurable if needed
  int delayThresholdMsecs_ = 1000;
  int backlogThreshold_ = 10;
};

}} // facebook::fboss
