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

#include <boost/msm/back/state_machine.hpp>
#include <boost/msm/front/euml/euml.hpp>
#include <boost/msm/front/euml/state_grammar.hpp>
#include <folly/logging/xlog.h>

DECLARE_bool(use_new_state_machine);

namespace facebook::fboss {

using namespace boost::msm::front::euml;
using namespace boost::msm::back;

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
TransceiverStateMachineState stateToStateEnum(State& state);

BOOST_MSM_EUML_ACTION(logStateChanged){
    template <class Event, class Fsm, class Source, class Target>
    void
    operator()(
        const Event& /* event */,
        Fsm& /* fsm */,
        Source& source,
        Target& target) const {
        XLOG(DBG2)
        << "State changed from "
        << getTransceiverStateMachineStateName(stateToStateEnum(source))
        << " to "
        << getTransceiverStateMachineStateName(stateToStateEnum(target));
} // namespace facebook::fboss
}
;

// Transceiver State Machine State transition table
// clang-format off
BOOST_MSM_EUML_TRANSITION_TABLE((
//  Start       + Event [Guard]       / Action          == Next
//  +------------------------------------------------------------------------------+
    NOT_PRESENT + DETECT_TRANSCEIVER  / logStateChanged == PRESENT,
    PRESENT     + READ_EEPROM         / logStateChanged == DISCOVERED
//  +------------------------------------------------------------------------------+
    ), TransceiverTransitionTable)
// clang-format on

// Define a Transceiver State Machine
BOOST_MSM_EUML_DECLARE_STATE_MACHINE(
    (TransceiverTransitionTable,
     init_ << NOT_PRESENT,
     no_action,
     no_action,
     attributes_ << isIphyProgrammed << isXphyProgrammed
                 << isTransceiverProgrammed),
    TransceiverStateMachine)

} // namespace facebook::fboss
