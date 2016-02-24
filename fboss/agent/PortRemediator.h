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

#include <chrono>
#include <folly/io/async/AsyncTimeout.h>

#include "fboss/agent/SwSwitch.h"

namespace facebook { namespace fboss {

class PortRemediator : private folly::AsyncTimeout {
 public:
  explicit PortRemediator(SwSwitch* swSwitch);
  ~PortRemediator();

  static void start(void *arg) {
    auto me = static_cast<PortRemediator*>(arg);
    me->scheduleTimeout(me->interval_);
  }

  static void stop(void* arg) {
    auto me = static_cast<PortRemediator*>(arg);
    me->cancelTimeout();
  }

  void timeoutExpired() noexcept override;

 private:
  SwSwitch* sw_;
  std::chrono::seconds interval_;
};

} // fboss
} // facebook
