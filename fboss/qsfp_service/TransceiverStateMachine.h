/*
 *  Copyright (c) 2018-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#pragma once

#include "fboss/agent/FbossError.h"
#include "fboss/agent/types.h"

#include <boost/msm/back/state_machine.hpp>
#include <boost/msm/front/euml/euml.hpp>
#include <boost/msm/front/euml/state_grammar.hpp>
#include <folly/logging/xlog.h>

DECLARE_bool(use_new_state_machine);

namespace facebook::fboss {

using namespace boost::msm::front::euml;
using namespace boost::msm::back;

class TransceiverManager;

enum class TransceiverStateMachineEvent {
  DETECT_TRANSCEIVER,
  OPTICS_REMOVED,
  // NOTE: Such event is never invoked in our code yet
  OPTICS_RESET,
  READ_EEPROM,
  ALL_PORTS_DOWN,
  PORT_UP,
  // NOTE: Such event is never invoked in our code yet
  TRIGGER_UPGRADE,
  // NOTE: Such event is never invoked in our code yet
  FORCED_UPGRADE,
  AGENT_SYNC_TIMEOUT,
  BRINGUP_DONE,
  REMEDIATE_DONE,
  // The following events are only used in the new state machine
  PROGRAM_IPHY,
  PROGRAM_XPHY,
  PROGRAM_TRANSCEIVER,
};

std::string getTransceiverStateMachineEventName(
    TransceiverStateMachineEvent event);

enum class TransceiverStateMachineState {
  NOT_PRESENT,
  PRESENT,
  DISCOVERED,
  IPHY_PORTS_PROGRAMMED,
  XPHY_PORTS_PROGRAMMED,
  TRANSCEIVER_PROGRAMMED,
  ACTIVE,
  INACTIVE,
  UPGRADING,
};

std::string getTransceiverStateMachineStateName(
    TransceiverStateMachineState state);

/*
 * Convert current_state() return int value to TransceiverStateMachineState
 * enum. Every state is an int value assigned in the order the states have been
 * accessed in BOOST_MSM_EUML_TRANSITION_TABLE.
 */
TransceiverStateMachineState getStateByOrder(int currentStateOrder);

/**************************** Transceiver State Machine ***********************/
// TODO(joseph5wu) The following transceiver state machine is planned to replace
// the current `moduleStateMachine` and `modulePortStateMachine` in
// module/ModuleStateMachine.h. Because the change is quite massive and right
// now they have been heavily used in
// QsfpModule, it might be safer to start the new change on a new state machine
// rather than tamper the current one.

// Module state machine attribute to tell whether the iphy port/ xphy port/
// transceiver programmed is already done
BOOST_MSM_EUML_DECLARE_ATTRIBUTE(bool, isIphyProgrammed)
BOOST_MSM_EUML_DECLARE_ATTRIBUTE(bool, isXphyProgrammed)
BOOST_MSM_EUML_DECLARE_ATTRIBUTE(bool, isTransceiverProgrammed)

// Module State Machine needs to trigger iphy/xphy/transceiver programming.
// Instead of adding a lot of complicated logic here, we can use
// TransceiverManager to provide a bunch public functions there.
// Besides, we might also need to call TransceiverManager::updateState() to
// update the state accordingly in the state machine.
BOOST_MSM_EUML_DECLARE_ATTRIBUTE(class TransceiverManager*, transceiverMgrPtr)
BOOST_MSM_EUML_DECLARE_ATTRIBUTE(TransceiverID, transceiverID)

// Transceiver State Machine States
BOOST_MSM_EUML_STATE((), NOT_PRESENT)
BOOST_MSM_EUML_STATE((), PRESENT)
BOOST_MSM_EUML_STATE((), DISCOVERED)
BOOST_MSM_EUML_STATE((), IPHY_PORTS_PROGRAMMED)
BOOST_MSM_EUML_STATE((), XPHY_PORTS_PROGRAMMED)
BOOST_MSM_EUML_STATE((), TRANSCEIVER_PROGRAMMED)
BOOST_MSM_EUML_STATE((), ACTIVE)
BOOST_MSM_EUML_STATE((), INACTIVE)
BOOST_MSM_EUML_STATE((), UPGRADING)

// Transceiver State Machine Events
BOOST_MSM_EUML_EVENT(DETECT_TRANSCEIVER)
BOOST_MSM_EUML_EVENT(READ_EEPROM)
BOOST_MSM_EUML_EVENT(PROGRAM_IPHY)
BOOST_MSM_EUML_EVENT(PROGRAM_XPHY)
BOOST_MSM_EUML_EVENT(PROGRAM_TRANSCEIVER)

// Module State Machine Actions
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

// clang-format couldn't handle the marco in a pretty way. So manually turn off
// the auto formating
// clang-format off
BOOST_MSM_EUML_ACTION(logStateChanged){
template <class Event, class Fsm, class Source, class Target>
void operator()(
    const Event& /* event */,
    Fsm& /* fsm */,
    Source& source,
    Target& target) const {
  XLOG(DBG2) << "State changed from "
             << getTransceiverStateMachineStateName(stateToStateEnum(source))
             << " to "
             << getTransceiverStateMachineStateName(stateToStateEnum(target));
}
};

BOOST_MSM_EUML_ACTION(programIphyPorts) {
template <class Event, class Fsm, class Source, class Target>
bool operator()(
    const Event& /* ev */,
    Fsm& fsm,
    Source& /* src */,
    Target& /* trg */) {
  auto tcvrID = fsm.get_attribute(transceiverID);
  try {
    fsm.get_attribute(transceiverMgrPtr)->programInternalPhyPorts(tcvrID);
    fsm.get_attribute(isIphyProgrammed) = true;
    return true;
  } catch (const std::exception& ex) {
    // We have retry mechanism to handle failure. No crash here
    XLOG(WARN) << "[Transceiver:" << tcvrID
               << "] programInternalPhyPorts failed:"
               << folly::exceptionStr(ex);
    return false;
  }
}
};

BOOST_MSM_EUML_ACTION(programXphyPorts) {
template <class Event, class Fsm, class Source, class Target>
bool operator()(
    const Event& /* ev */,
    Fsm& fsm,
    Source& /* src */,
    Target& /* trg */) {
  auto tcvrID = fsm.get_attribute(transceiverID);
  try {
    fsm.get_attribute(transceiverMgrPtr)->programExternalPhyPorts(tcvrID);
    fsm.get_attribute(isXphyProgrammed) = true;
    return true;
  } catch (const std::exception& ex) {
    // We have retry mechanism to handle failure. No crash here
    XLOG(WARN) << "[Transceiver:" << tcvrID
               << "] programExternalPhyPorts failed:"
               << folly::exceptionStr(ex);
    return false;
  }
}
};
// clang-format on

// Transceiver State Machine State transition table
// clang-format off
BOOST_MSM_EUML_TRANSITION_TABLE((
//  Start                 + Event        [Guard]            / Action          == Next
// +----------------------------------------------------------------------------------------+
    NOT_PRESENT           + DETECT_TRANSCEIVER              / logStateChanged == PRESENT,
    PRESENT               + READ_EEPROM                     / logStateChanged == DISCOVERED,
    // For non-present transceiver, we still want to call port program in case optic is actually
    // inserted but just can't read the present state
    NOT_PRESENT           + PROGRAM_IPHY [programIphyPorts] / logStateChanged == IPHY_PORTS_PROGRAMMED,
    DISCOVERED            + PROGRAM_IPHY [programIphyPorts] / logStateChanged == IPHY_PORTS_PROGRAMMED,
    IPHY_PORTS_PROGRAMMED + PROGRAM_XPHY [programXphyPorts] / logStateChanged == XPHY_PORTS_PROGRAMMED
//  +---------------------------------------------------------------------------------------+
    ), TransceiverTransitionTable)
// clang-format on

// Define a Transceiver State Machine
BOOST_MSM_EUML_DECLARE_STATE_MACHINE(
    (TransceiverTransitionTable,
     init_ << NOT_PRESENT,
     no_action,
     no_action,
     attributes_ << isIphyProgrammed << isXphyProgrammed
                 << isTransceiverProgrammed << transceiverMgrPtr
                 << transceiverID),
    TransceiverStateMachine)

} // namespace facebook::fboss
