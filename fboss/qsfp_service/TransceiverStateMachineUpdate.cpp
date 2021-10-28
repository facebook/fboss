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
    TransceiverStateMachineEvent transceiverEvent)
    : id_(id),
      transceiverEvent_(transceiverEvent),
      name_(fmt::format(
          "[Transceiver: {}, Event:{}]",
          id,
          getTransceiverStateMachineEventName(transceiverEvent))) {}

void TransceiverStateMachineUpdate::applyUpdate(
    state_machine<TransceiverStateMachine>& curState) {
  switch (transceiverEvent_) {
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
      // TODO(joseph5wu) Will call PhyManager programOnePort()
      return;
    case TransceiverStateMachineEvent::PROGRAM_TRANSCEIVER:
      // TODO(joseph5wu) Will call Transceiver customizeTransceiver()
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

} // namespace facebook::fboss
