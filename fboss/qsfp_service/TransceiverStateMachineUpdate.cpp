/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/qsfp_service/TransceiverStateMachineUpdate.h"

#include "fboss/agent/FbossError.h"
#include "fboss/qsfp_service/TransceiverManager.h"

#include <fmt/format.h>

namespace facebook::fboss {
TransceiverStateMachineUpdate::TransceiverStateMachineUpdate(
    TransceiverID id,
    TransceiverStateMachineEvent event)
    : id_(id),
      event_(event),
      name_(fmt::format(
          "[Transceiver: {}, Event:{}]",
          id,
          getTransceiverStateMachineEventName(event))) {}

void TransceiverStateMachineUpdate::applyUpdate(
    state_machine<TransceiverStateMachine>& curState) {
  switch (event_) {
    case TransceiverStateMachineEvent::DETECT_TRANSCEIVER:
      curState.process_event(DETECT_TRANSCEIVER);
      return;
    case TransceiverStateMachineEvent::READ_EEPROM:
      curState.process_event(READ_EEPROM);
      return;
    case TransceiverStateMachineEvent::PROGRAM_IPHY:
      curState.process_event(PROGRAM_IPHY);
      return;
    case TransceiverStateMachineEvent::PROGRAM_XPHY:
      curState.process_event(PROGRAM_XPHY);
      return;
    case TransceiverStateMachineEvent::PROGRAM_TRANSCEIVER:
      curState.process_event(PROGRAM_TRANSCEIVER);
      return;
    case TransceiverStateMachineEvent::ALL_PORTS_DOWN:
      curState.process_event(ALL_PORTS_DOWN);
      return;
    case TransceiverStateMachineEvent::PORT_UP:
      curState.process_event(PORT_UP);
      return;
    case TransceiverStateMachineEvent::RESET_TO_NOT_PRESENT:
      curState.process_event(RESET_TO_NOT_PRESENT);
      return;
    case TransceiverStateMachineEvent::RESET_TO_DISCOVERED:
      curState.process_event(RESET_TO_DISCOVERED);
      return;
    case TransceiverStateMachineEvent::REMOVE_TRANSCEIVER:
      curState.process_event(REMOVE_TRANSCEIVER);
      return;
    default:
      throw FbossError("Unsupported TransceiverStateMachine for ", name_);
  }
}

void TransceiverStateMachineUpdate::onError(const std::exception& ex) noexcept {
  XLOG(WARN) << "Unexpected error applying state update for " << name_ << ": "
             << folly::exceptionStr(ex);
  // TODO(joseph5wu) Need to figure out how to properly handle error cases/
  // Like how to provide a retry mechanism.
}

void TransceiverStateMachineUpdate::onSuccess() {
  XLOG(INFO) << "Successfully applied state update for " << name_;
  // TODO(joseph5wu) Need to figure out whether we need to notify anything when
  // the state is applied successfully
}

void BlockingTransceiverStateMachineUpdateResult::wait() {
  std::unique_lock<std::mutex> guard(lock_);
  cond_.wait(guard, [this] { return done_; });
  // TODO(joseph5wu) Need to figure out how to properly handle error cases/
  // Like how to provide a retry mechanism.
}

void BlockingTransceiverStateMachineUpdateResult::signalSuccess() {
  {
    std::unique_lock<std::mutex> guard(lock_);
    CHECK(!done_);
    done_ = true;
  }
  cond_.notify_one();
}

void BlockingTransceiverStateMachineUpdateResult::signalError(
    const std::exception_ptr& ex) noexcept {
  {
    std::unique_lock<std::mutex> guard(lock_);
    CHECK(!done_);
    error_ = ex;
    done_ = true;
  }
  cond_.notify_one();
}

void BlockingTransceiverStateMachineUpdate::onSuccess() {
  TransceiverStateMachineUpdate::onSuccess();
  result_->signalSuccess();
}

void BlockingTransceiverStateMachineUpdate::onError(
    const std::exception& ex) noexcept {
  TransceiverStateMachineUpdate::onError(ex);
  // Note that we need to use std::current_exception() here rather than
  // std::make_exception_ptr() -- make_exception_ptr will lose the original
  // exception type information.
  // TransceiverManager guarantees that the exception
  // passed in will still be available as std::current_exception() when
  // onError() is called.
  result_->signalError(std::current_exception());
}

} // namespace facebook::fboss
