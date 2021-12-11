/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/qsfp_service/TransceiverStateMachine.h"

DEFINE_bool(use_new_state_machine, false, "Use the new state machine logic");

namespace facebook::fboss {

std::string getTransceiverStateMachineEventName(
    TransceiverStateMachineEvent event) {
  switch (event) {
    case TransceiverStateMachineEvent::DETECT_TRANSCEIVER:
      return "DETECT_TRANSCEIVER";
    case TransceiverStateMachineEvent::REMOVE_TRANSCEIVER:
      return "REMOVE_TRANSCEIVER";
    case TransceiverStateMachineEvent::RESET_TRANSCEIVER:
      return "RESET_TRANSCEIVER";
    case TransceiverStateMachineEvent::READ_EEPROM:
      return "READ_EEPROM";
    case TransceiverStateMachineEvent::ALL_PORTS_DOWN:
      return "ALL_PORTS_DOWN";
    case TransceiverStateMachineEvent::PORT_UP:
      return "PORT_UP";
    case TransceiverStateMachineEvent::TRIGGER_UPGRADE:
      return "TRIGGER_UPGRADE";
    case TransceiverStateMachineEvent::FORCED_UPGRADE:
      return "FORCED_UPGRADE";
    case TransceiverStateMachineEvent::AGENT_SYNC_TIMEOUT:
      return "AGENT_SYNC_TIMEOUT";
    case TransceiverStateMachineEvent::BRINGUP_DONE:
      return "BRINGUP_DONE";
    case TransceiverStateMachineEvent::REMEDIATE_DONE:
      return "REMEDIATE_DONE";
    case TransceiverStateMachineEvent::PROGRAM_IPHY:
      return "PROGRAM_IPHY";
    case TransceiverStateMachineEvent::PROGRAM_XPHY:
      return "PROGRAM_XPHY";
    case TransceiverStateMachineEvent::PROGRAM_TRANSCEIVER:
      return "PROGRAM_TRANSCEIVER";
    case TransceiverStateMachineEvent::RESET_TO_DISCOVERED:
      return "RESET_TO_DISCOVERED";
    case TransceiverStateMachineEvent::RESET_TO_NOT_PRESENT:
      return "RESET_TO_NOT_PRESENT";
  }
  throw FbossError("Unsupported TransceiverStateMachineEvent");
}

TransceiverStateMachineState getStateByOrder(int currentStateOrder) {
  if (currentStateOrder == 0) {
    return TransceiverStateMachineState::NOT_PRESENT;
  } else if (currentStateOrder == 1) {
    return TransceiverStateMachineState::PRESENT;
  } else if (currentStateOrder == 2) {
    return TransceiverStateMachineState::DISCOVERED;
  } else if (currentStateOrder == 3) {
    return TransceiverStateMachineState::IPHY_PORTS_PROGRAMMED;
  } else if (currentStateOrder == 4) {
    return TransceiverStateMachineState::XPHY_PORTS_PROGRAMMED;
  } else if (currentStateOrder == 5) {
    return TransceiverStateMachineState::TRANSCEIVER_PROGRAMMED;
  } else if (currentStateOrder == 6) {
    return TransceiverStateMachineState::ACTIVE;
  } else if (currentStateOrder == 7) {
    return TransceiverStateMachineState::INACTIVE;
  }
  // TODO(joseph5wu) Need to support other states
  throw FbossError(
      "Unsupported TransceiverStateMachineState order: ", currentStateOrder);
}
} // namespace facebook::fboss
