/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/qsfp_service/SlotThreadHelper.h"

DEFINE_int32(
    state_machine_update_thread_heartbeat_ms,
    20000,
    "State machine update thread's heartbeat interval (ms)");

namespace facebook::fboss {
void SlotThreadHelper::startThread() {
  updateEventBase_ = std::make_unique<folly::EventBase>("slot update thread");
  updateThread_.reset(
      new std::thread([this] { updateEventBase_->loopForever(); }));
  auto heartbeatStatsFunc = [](int /* delay */, int /* backLog */) {};
  heartbeat_ = std::make_shared<ThreadHeartbeat>(
      updateEventBase_.get(),
      folly::to<std::string>("thread_", id_, "_"),
      FLAGS_state_machine_update_thread_heartbeat_ms,
      heartbeatStatsFunc);
  isThreadActive_ = true;
}

void SlotThreadHelper::stopThread() {
  isThreadActive_ = false;
  if (updateThread_) {
    updateEventBase_->terminateLoopSoon();
    updateThread_->join();
  }
}
} // namespace facebook::fboss
