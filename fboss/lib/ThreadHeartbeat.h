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
#include <folly/concurrency/ConcurrentHashMap.h>
#include <folly/io/async/AsyncTimeout.h>
#include <folly/io/async/EventBase.h>
#include <folly/logging/xlog.h>
#include <chrono>

namespace facebook::fboss {

class ThreadHeartbeat : private folly::AsyncTimeout {
  /*
   * Send heartbeat at regular interval to thread.  Measure delay between
   * time we expect heartbeat to be processed vs. time actually processed,
   * and record it to ods.
   */
 public:
  ThreadHeartbeat(
      folly::EventBase* evb,
      std::string threadName,
      int intervalMsecs,
      std::function<void(int, int)> heartbeatStatsFunc)
      : AsyncTimeout(evb),
        evb_(evb),
        threadName_(threadName),
        intervalMsecs_(intervalMsecs),
        heartbeatStatsFunc_(heartbeatStatsFunc) {
    XLOG(DBG2) << "ThreadHeartbeat intervalMsecs:" << intervalMsecs_.count();
    evb_->runInEventBaseThread([this]() { scheduleFirstHeartbeat(); });
  }

  ~ThreadHeartbeat() override {
    evb_->runImmediatelyOrRunInEventBaseThreadAndWait(
        [this]() { cancelTimeout(); });
  }

  std::chrono::time_point<std::chrono::steady_clock> getTimestamp() {
    return lastTime_.load(std::memory_order_relaxed);
  }

  const std::string& getThreadName() const {
    return threadName_;
  }

  void setMonitoringPaused(bool pause) {
    monitoringPaused_ = pause;
  }

  bool getMonitoringPaused() const {
    return monitoringPaused_;
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
  std::atomic<std::chrono::time_point<std::chrono::steady_clock>> lastTime_{
      std::chrono::steady_clock::time_point::min()};
  // XXX: these thresholds could be made configurable if needed
  int delayThresholdMsecs_ = 1000;
  int backlogThreshold_ = 10;
  std::atomic_bool monitoringPaused_{false};
};

// monitor thread heartbeats, and alarm if heartbeat timestamp
// not changed
class ThreadHeartbeatWatchdog {
 public:
  // intervalMsecs should be longer than the interval of monitored thread
  // heartbeats, e.g. at least 2 times
  explicit ThreadHeartbeatWatchdog(
      std::chrono::milliseconds intervalMsecs,
      std::function<void()> heartbeatMissFunc) {
    intervalMsecs_ = intervalMsecs;
    heartbeatMissFunc_ = heartbeatMissFunc;
  }

  virtual ~ThreadHeartbeatWatchdog() {
    stop();
    heartbeats_.clear();
    XLOG(DBG2) << "Thread heartbeat watchdog destroyed";
  }

  // add the heartbeat of monitored threads
  void startMonitoringHeartbeat(std::shared_ptr<ThreadHeartbeat> heartbeat) {
    if (heartbeats_.find(heartbeat) != heartbeats_.end()) {
      throw std::runtime_error(
          "Heartbeat already monitored for thread " +
          heartbeat->getThreadName());
    }
    heartbeats_.insert(heartbeat, std::chrono::steady_clock::time_point::min());
  }

  void pauseMonitoringHeartbeat(std::shared_ptr<ThreadHeartbeat> heartbeat) {
    if (heartbeats_.find(heartbeat) == heartbeats_.end()) {
      throw std::runtime_error(
          "Heartbeat not yet monitored for thread " +
          heartbeat->getThreadName());
    }
    heartbeat->setMonitoringPaused(true);
  }

  void resumeMonitoringHeartbeat(std::shared_ptr<ThreadHeartbeat> heartbeat) {
    if (heartbeats_.find(heartbeat) == heartbeats_.end()) {
      throw std::runtime_error(
          "Heartbeat not yet monitored for thread " +
          heartbeat->getThreadName());
    }
    heartbeat->setMonitoringPaused(false);
  }

  // start to monitor
  void start() {
    if (!running_) {
      running_ = true;
      thread_ = std::thread([this]() {
        XLOG(INFO) << "Start thread heartbeat watchdog";
        watchdogLoop();
      });
    }
  }

  // stop to monitor
  void stop() {
    if (running_) {
      running_ = false;
      XLOG(INFO) << "Stopping thread heartbeat watchdog";
      cv_.notify_one();
      thread_.join();
    }
  }

 private:
  void watchdogLoop();
  std::thread thread_;
  std::atomic_bool running_{false};
  std::chrono::milliseconds intervalMsecs_;
  folly::ConcurrentHashMap<
      std::shared_ptr<ThreadHeartbeat>,
      std::chrono::time_point<std::chrono::steady_clock>>
      heartbeats_;
  std::mutex m_;
  std::condition_variable cv_;
  std::function<void()> heartbeatMissFunc_;
};

} // namespace facebook::fboss
