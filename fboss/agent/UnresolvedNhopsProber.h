/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#pragma once

#include "fboss/agent/NexthopToRouteCount.h"
#include "fboss/agent/StateObserver.h"

namespace facebook { namespace fboss {

class SwSwitch;
class StateDelta;

class UnresolvedNhopsProber : private folly::AsyncTimeout,
                              public AutoRegisterStateObserver {
 public:
  explicit UnresolvedNhopsProber(SwSwitch *sw) :
      AsyncTimeout(sw->getBackgroundEVB()),
      AutoRegisterStateObserver(sw, "UnresolvedNhopsProber"),
      sw_(sw),
      // Probe every 5 secs (make it faster ?)
      interval_(5) {
    start();
  }

  ~UnresolvedNhopsProber() {
    sw_->getBackgroundEVB()->runImmediatelyOrRunInEventBaseThreadAndWait(
      [this]() {
        cancelTimeout();
      });
  }

  void start() {
    sw_->getBackgroundEVB()->runInEventBaseThread([this]() {
      scheduleTimeout(interval_);
    });
  }

  void stateUpdated(const StateDelta& delta) override {
    std::lock_guard<std::mutex> g(lock_);
    nhops2RouteCount_.stateChanged(delta);
  }

  void timeoutExpired() noexcept override;

 private:
  // Need lock since we may get called from both the update
  // thread (stateChanged) and background thread (timeoutExpired)
  std::mutex lock_;
  SwSwitch* sw_{nullptr};
  NexthopToRouteCount nhops2RouteCount_;
  std::chrono::seconds interval_{0};
};

}} // facebook::fboss
