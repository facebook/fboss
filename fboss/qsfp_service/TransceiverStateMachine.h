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
  OPTICS_DETECTED,
  OPTICS_REMOVED,
  // NOTE: Such event is never invoked in our code yet
  OPTICS_RESET,
  EEPROM_READ,
  ALL_PORTS_DOWN,
  PORT_UP,
  // NOTE: Such event is never invoked in our code yet
  TRIGGER_UPGRADE,
  // NOTE: Such event is never invoked in our code yet
  FORCED_UPGRADE,
  AGENT_SYNC_TIMEOUT,
  BRINGUP_DONE,
  REMEDIATE_DONE,
};

std::string getTransceiverStateMachineEventName(
    TransceiverStateMachineEvent event);

/**************************** Transceiver State Machine ***********************/
// TODO(joseph5wu) The following transceiver state machine is planned to replace
// the current `moduleStateMachine` and `modulePortStateMachine` in
// module/ModuleStateMachine.h. Because the change is quite massive and right
// now they have been heavily used in
// QsfpModule, it might be safer to start the new change on a new state machine
// rather than tamper the current one.

// Transceiver State Machine States
BOOST_MSM_EUML_STATE((), TRANSCEIVER_STATE_NOT_PRESENT)
BOOST_MSM_EUML_STATE((), TRANSCEIVER_STATE_PRESENT)
BOOST_MSM_EUML_STATE((), TRANSCEIVER_STATE_DISCOVERED)
BOOST_MSM_EUML_STATE((), TRANSCEIVER_STATE_IPHY_PORTS_PROGRAMMED)
BOOST_MSM_EUML_STATE((), TRANSCEIVER_STATE_XPHY_PORTS_PROGRAMMED)
BOOST_MSM_EUML_STATE((), TRANSCEIVER_STATE_TRANSCEIVER_PROGRAMMED)
BOOST_MSM_EUML_STATE((), TRANSCEIVER_STATE_ACTIVE)
BOOST_MSM_EUML_STATE((), TRANSCEIVER_STATE_INACTIVE)
BOOST_MSM_EUML_STATE((), TRANSCEIVER_STATE_UPGRADING)

// Transceiver State Machine Events
BOOST_MSM_EUML_EVENT(TRANSCEIVER_EVENT_OPTICS_DETECTED)

// Transceiver State Machine State transition table
BOOST_MSM_EUML_TRANSITION_TABLE(
    (TRANSCEIVER_STATE_NOT_PRESENT + TRANSCEIVER_EVENT_OPTICS_DETECTED ==
     TRANSCEIVER_STATE_PRESENT),
    TransceiverTransitionTable)

// Define a Transceiver State Machine
BOOST_MSM_EUML_DECLARE_STATE_MACHINE(
    (TransceiverTransitionTable, init_ << TRANSCEIVER_STATE_NOT_PRESENT),
    TransceiverStateMachine)

} // namespace facebook::fboss
