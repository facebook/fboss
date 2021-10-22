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
    case TransceiverStateMachineEvent::OPTICS_DETECTED:
      return "OPTICS_DETECTED";
    case TransceiverStateMachineEvent::OPTICS_REMOVED:
      return "OPTICS_REMOVED";
    case TransceiverStateMachineEvent::OPTICS_RESET:
      return "OPTICS_RESET";
    case TransceiverStateMachineEvent::EEPROM_READ:
      return "EEPROM_READ";
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
  }
  throw FbossError("Unsupported TransceiverStateMachineEvent");
}

template <class State>
std::string stateToName(State& /* state */) {
  if constexpr (std::
                    is_same_v<State, decltype(TRANSCEIVER_STATE_NOT_PRESENT)>) {
    return "NOT_PRESENT";
  } else if constexpr (std::is_same_v<
                           State,
                           decltype(TRANSCEIVER_STATE_PRESENT)>) {
    return "PRESENT";
  } else if constexpr (std::is_same_v<
                           State,
                           decltype(TRANSCEIVER_STATE_DISCOVERED)>) {
    return "DISCOVERED";
  } else if constexpr (std::is_same_v<
                           State,
                           decltype(TRANSCEIVER_STATE_IPHY_PORTS_PROGRAMMED)>) {
    return "IPHY_PORTS_PROGRAMMED";
  } else if constexpr (std::is_same_v<
                           State,
                           decltype(TRANSCEIVER_STATE_XPHY_PORTS_PROGRAMMED)>) {
    return "XPHY_PORTS_PROGRAMMED";
  } else if constexpr (
      std::is_same_v<
          State,
          decltype(TRANSCEIVER_STATE_TRANSCEIVER_PROGRAMMED)>) {
    return "TRANSCEIVER_PROGRAMMED";
  } else if constexpr (std::is_same_v<
                           State,
                           decltype(TRANSCEIVER_STATE_ACTIVE)>) {
    return "ACTIVE";
  } else if constexpr (std::is_same_v<
                           State,
                           decltype(TRANSCEIVER_STATE_INACTIVE)>) {
    return "INACTIVE";
  } else if constexpr (std::is_same_v<
                           State,
                           decltype(TRANSCEIVER_STATE_UPGRADING)>) {
    return "UPGRADING";
  }

  throw FbossError("Unsupported ModuleStateMachineState");
}

template std::string stateToName<decltype(TRANSCEIVER_STATE_NOT_PRESENT)>(
    decltype(TRANSCEIVER_STATE_NOT_PRESENT)& state);
template std::string stateToName<decltype(TRANSCEIVER_STATE_PRESENT)>(
    decltype(TRANSCEIVER_STATE_PRESENT)& state);
template std::string stateToName<decltype(TRANSCEIVER_STATE_DISCOVERED)>(
    decltype(TRANSCEIVER_STATE_DISCOVERED)& state);
} // namespace facebook::fboss
