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

#include <boost/bimap.hpp>
#include "fboss/lib/ThreadHeartbeat.h"

#include <folly/IntrusiveList.h>
#include <folly/SpinLock.h>
#include <folly/Synchronized.h>
#include "fboss/agent/types.h"

DECLARE_int32(state_machine_update_thread_heartbeat_ms);

namespace facebook::fboss {
class SlotThreadHelper {
 public:
  explicit SlotThreadHelper(const TransceiverID& id) : id_(id) {}

  void startThread();
  void stopThread();
  folly::EventBase* getEventBase() const {
    return updateEventBase_.get();
  }

  std::shared_ptr<ThreadHeartbeat> getThreadHeartbeat() const {
    return heartbeat_;
  }

  bool isThreadActive() const {
    return isThreadActive_;
  }

 private:
  //  For logging purposes.
  TransceiverID id_;

  // Can't use ScopedEventBaseThread as it won't work well with
  // handcrafted HeaderClientChannel client instead of servicerouter client
  std::unique_ptr<std::thread> updateThread_;
  std::unique_ptr<folly::EventBase> updateEventBase_;
  std::shared_ptr<ThreadHeartbeat> heartbeat_;

  bool isThreadActive_{false};
};
} // namespace facebook::fboss
