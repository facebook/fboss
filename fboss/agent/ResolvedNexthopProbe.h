// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include "fboss/agent/state/RouteNextHop.h"
#include "fboss/lib/ExponentialBackoff.h"

#include <folly/io/async/AsyncTimeout.h>
#include <folly/io/async/EventBase.h>

namespace facebook::fboss {

class SwSwitch;

class ResolvedNextHopProbe : public folly::AsyncTimeout {
 public:
  ResolvedNextHopProbe(
      SwSwitch* sw,
      folly::EventBase* evb,
      ResolvedNextHop nexthop);

  ~ResolvedNextHopProbe() override {
    stop();
  }

  void start() {
    evb_->runImmediatelyOrRunInEventBaseThreadAndWait([=]() { _start(); });
  }

  void stop() {
    evb_->runImmediatelyOrRunInEventBaseThreadAndWait([=]() { _stop(); });
  }

 private:
  void _start() {
    scheduleTimeout(backoff_.getTimeRemainingUntilRetry());
  }

  void _stop() {
    cancelTimeout();
    backoff_.reportSuccess();
  }
  void timeoutExpired() noexcept override;

  SwSwitch* sw_;
  folly::EventBase* evb_;
  ResolvedNextHop nexthop_;
  ExponentialBackoff<std::chrono::milliseconds> backoff_;
};

} // namespace facebook::fboss
