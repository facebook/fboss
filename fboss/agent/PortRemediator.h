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

#include <boost/container/flat_set.hpp>

#include <chrono>
#include <folly/io/async/AsyncTimeout.h>

#include "fboss/agent/SwSwitch.h"
#include "fboss/agent/state/Port.h"
#include "fboss/agent/gen-cpp2/switch_config_types.h"

namespace facebook { namespace fboss {

class PortRemediator : private folly::AsyncTimeout {
 public:
  explicit PortRemediator(SwSwitch* swSwitch);
  ~PortRemediator() override;

  static void start(void *arg) {
    auto me = static_cast<PortRemediator*>(arg);
    me->scheduleTimeout(me->interval_);
  }

  static void stop(void* arg) {
    auto me = static_cast<PortRemediator*>(arg);
    me->cancelTimeout();
  }

  void timeoutExpired() noexcept override;
  void init();

 private:
  void updatePortState(
      PortID portId,
      cfg::PortState newPortState,
      bool preventCoalescing);
  boost::container::flat_set<PortID> getUnexpectedDownPorts()
      const;
  SwSwitch* sw_;
  std::chrono::seconds interval_;
};

} // fboss
} // facebook
