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

namespace facebook::fboss {
void TransceiverStateMachineUpdate::applyUpdate(
    state_machine<TransceiverStateMachine>* curState) {
  switch (transceiverEvent_) {
    case TransceiverStateMachineEvent::OPTICS_DETECTED:
      curState->process_event(TRANSCEIVER_EVENT_OPTICS_DETECTED);
      break;
    default:
      throw FbossError(
          "Unsupported TransceiverStateMachineEvent: ",
          getTransceiverStateMachineEventName(transceiverEvent_));
  }
}

} // namespace facebook::fboss
