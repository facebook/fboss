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

#include "fboss/agent/FbossError.h"

DEFINE_bool(use_new_state_machine, false, "Use the new state machine logic");

namespace facebook::fboss {

std::string getTransceiverStateMachineEventName(
    TransceiverStateMachineEvent event) {
  switch (event) {
    case TransceiverStateMachineEvent::DETECT_TRANSCEIVER:
      return "DETECT_TRANSCEIVER";
    case TransceiverStateMachineEvent::OPTICS_REMOVED:
      return "OPTICS_REMOVED";
    case TransceiverStateMachineEvent::OPTICS_RESET:
      return "OPTICS_RESET";
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
  }
  throw FbossError("Unsupported TransceiverStateMachineEvent");
}

std::string getTransceiverStateMachineStateName(
    TransceiverStateMachineState state) {
  switch (state) {
    case TransceiverStateMachineState::NOT_PRESENT:
      return "NOT_PRESENT";
    case TransceiverStateMachineState::PRESENT:
      return "PRESENT";
    case TransceiverStateMachineState::DISCOVERED:
      return "DISCOVERED";
    case TransceiverStateMachineState::IPHY_PORTS_PROGRAMMED:
      return "IPHY_PORTS_PROGRAMMED";
    case TransceiverStateMachineState::XPHY_PORTS_PROGRAMMED:
      return "XPHY_PORTS_PROGRAMMED";
    case TransceiverStateMachineState::TRANSCEIVER_PROGRAMMED:
      return "TRANSCEIVER_PROGRAMMED";
    case TransceiverStateMachineState::ACTIVE:
      return "ACTIVE";
    case TransceiverStateMachineState::INACTIVE:
      return "INACTIVE";
    case TransceiverStateMachineState::UPGRADING:
      return "UPGRADING";
  }
  throw FbossError("Unsupported TransceiverStateMachineState");
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
  }
  // TODO(joseph5wu) Need to support other states
  throw FbossError(
      "Unsupported TransceiverStateMachineState order: ", currentStateOrder);
}

template <class State>
TransceiverStateMachineState stateToStateEnum(State& /* state */) {
  if constexpr (std::is_same_v<State, decltype(NOT_PRESENT)>) {
    return TransceiverStateMachineState::NOT_PRESENT;
  } else if constexpr (std::is_same_v<State, decltype(PRESENT)>) {
    return TransceiverStateMachineState::PRESENT;
  } else if constexpr (std::is_same_v<State, decltype(DISCOVERED)>) {
    return TransceiverStateMachineState::DISCOVERED;
  } else if constexpr (std::is_same_v<State, decltype(IPHY_PORTS_PROGRAMMED)>) {
    return TransceiverStateMachineState::IPHY_PORTS_PROGRAMMED;
  } else if constexpr (std::is_same_v<State, decltype(XPHY_PORTS_PROGRAMMED)>) {
    return TransceiverStateMachineState::XPHY_PORTS_PROGRAMMED;
  } else if constexpr (std::
                           is_same_v<State, decltype(TRANSCEIVER_PROGRAMMED)>) {
    return TransceiverStateMachineState::TRANSCEIVER_PROGRAMMED;
  } else if constexpr (std::is_same_v<State, decltype(ACTIVE)>) {
    return TransceiverStateMachineState::ACTIVE;
  } else if constexpr (std::is_same_v<State, decltype(INACTIVE)>) {
    return TransceiverStateMachineState::INACTIVE;
  } else if constexpr (std::is_same_v<State, decltype(UPGRADING)>) {
    return TransceiverStateMachineState::UPGRADING;
  }

  throw FbossError("Unsupported TransceiverStateMachineState");
}

template TransceiverStateMachineState stateToStateEnum<decltype(NOT_PRESENT)>(
    decltype(NOT_PRESENT)& state);
template TransceiverStateMachineState stateToStateEnum<decltype(PRESENT)>(
    decltype(PRESENT)& state);
template TransceiverStateMachineState stateToStateEnum<decltype(DISCOVERED)>(
    decltype(DISCOVERED)& state);
template TransceiverStateMachineState stateToStateEnum<
    decltype(IPHY_PORTS_PROGRAMMED)>(decltype(IPHY_PORTS_PROGRAMMED)& state);
} // namespace facebook::fboss
