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
#include "fboss/qsfp_service/if/gen-cpp2/transceiver_types.h"

#include <boost/msm/back/state_machine.hpp>
#include <boost/msm/front/euml/euml.hpp>
#include <boost/msm/front/euml/state_grammar.hpp>
#include <folly/logging/xlog.h>
#include <thrift/lib/cpp/util/EnumUtils.h>

namespace facebook::fboss {

using namespace boost::msm::front::euml;
using namespace boost::msm::back;

class PortManager;

/*
 * Convert current_state() return int value to PortStateMachineState
 * enum. Every state is an int value assigned in the order the states have been
 * accessed in BOOST_MSM_EUML_TRANSITION_TABLE.
 */
PortStateMachineState getPortStateByOrder(int currentStateOrder);

/**************************** Port State Machine ***********************/

BOOST_MSM_EUML_DECLARE_ATTRIBUTE(class PortManager*, portMgrPtr)
BOOST_MSM_EUML_DECLARE_ATTRIBUTE(PortID, portID)
BOOST_MSM_EUML_DECLARE_ATTRIBUTE(std::string, portName)

// Use this bool value to gate whether we need to reset the data path for
// xphy.
// Some of the high speed optics will stuck in a bad state if iphy reset during
// agent coldboot because iphy port might send some unstable signals during the
// coldboot. Therefore, we need to reset the data path for xphy/tcvr after
// agent coldboot and reset the iphy.
BOOST_MSM_EUML_DECLARE_ATTRIBUTE(bool, xphyNeedResetDataPath)

BOOST_MSM_EUML_ACTION(updatePortCache){
    template <class Event, class Fsm, class State>
    void operator()(const Event& /* event */, Fsm & fsm, State & currState)
        const {auto portId = fsm.get_attribute(portID);
auto portMgr = fsm.get_attribute(portMgrPtr);
auto newState = portStateToStateEnum(currState);
bool isEnabled = newState == PortStateMachineState::INITIALIZED;
XLOG(DBG2) << "[Port:" << portId << "] State changed to "
           << apache::thrift::util::enumNameSafe(newState);

portMgr->clearTransceiversReadyForProgramming(portId);
portMgr->setPortEnabledStatusInCache(portId, isEnabled);
portMgr->clearEnabledTransceiversForPort(portId);
portMgr->clearMultiTcvrMappings(portId);
} // namespace facebook::fboss
}
;

// Port State Machine States
BOOST_MSM_EUML_STATE((updatePortCache), PORT_STATE_UNINITIALIZED)
BOOST_MSM_EUML_STATE((updatePortCache), PORT_STATE_INITIALIZED)
BOOST_MSM_EUML_STATE((), PORT_STATE_IPHY_PORTS_PROGRAMMED)
BOOST_MSM_EUML_STATE((), PORT_STATE_XPHY_PORTS_PROGRAMMED)
BOOST_MSM_EUML_STATE((), PORT_STATE_TRANSCEIVERS_PROGRAMMED)
BOOST_MSM_EUML_STATE((), PORT_STATE_PORT_UP)
BOOST_MSM_EUML_STATE((), PORT_STATE_PORT_DOWN)

// Port State Machine Events
BOOST_MSM_EUML_EVENT(INITIALIZE_PORT)
BOOST_MSM_EUML_EVENT(PORT_PROGRAM_IPHY)
BOOST_MSM_EUML_EVENT(PORT_PROGRAM_XPHY)
BOOST_MSM_EUML_EVENT(CHECK_TCVRS_PROGRAMMED)
BOOST_MSM_EUML_EVENT(SET_PORT_UP)
BOOST_MSM_EUML_EVENT(SET_PORT_DOWN)
BOOST_MSM_EUML_EVENT(RESET_TO_UNINITIALIZED)
BOOST_MSM_EUML_EVENT(RESET_TO_INITIALIZED)

// For logging purposes.
template <class State>
PortStateMachineState portStateToStateEnum(State& /* state */) {
  if constexpr (std::is_same_v<State, decltype(PORT_STATE_UNINITIALIZED)>) {
    return PortStateMachineState::UNINITIALIZED;
  } else if constexpr (std::
                           is_same_v<State, decltype(PORT_STATE_INITIALIZED)>) {
    return PortStateMachineState::INITIALIZED;
  } else if constexpr (std::is_same_v<
                           State,
                           decltype(PORT_STATE_IPHY_PORTS_PROGRAMMED)>) {
    return PortStateMachineState::IPHY_PORTS_PROGRAMMED;
  } else if constexpr (std::is_same_v<
                           State,
                           decltype(PORT_STATE_XPHY_PORTS_PROGRAMMED)>) {
    return PortStateMachineState::XPHY_PORTS_PROGRAMMED;
  } else if constexpr (std::is_same_v<
                           State,
                           decltype(PORT_STATE_TRANSCEIVERS_PROGRAMMED)>) {
    return PortStateMachineState::TRANSCEIVERS_PROGRAMMED;
  } else if constexpr (std::is_same_v<State, decltype(PORT_STATE_PORT_UP)>) {
    return PortStateMachineState::PORT_UP;
  } else if constexpr (std::is_same_v<State, decltype(PORT_STATE_PORT_DOWN)>) {
    return PortStateMachineState::PORT_DOWN;
  }
  throw FbossError("Unsupported PortStateMachineState");
}

BOOST_MSM_EUML_ACTION(portLogStateChanged){
    template <class Event, class Fsm, class Source, class Target>
    void operator()(
        const Event& /* event */,
        Fsm &
            fsm,
        Source &
            source,
        Target &
            target) const {auto portId = fsm.get_attribute(portID);
auto name = fsm.get_attribute(portName);
XLOG(DBG2) << "[Port: " << name << ", PortID: " << portId
           << "] State changed from "
           << apache::thrift::util::enumNameSafe(portStateToStateEnum(source))
           << " to "
           << apache::thrift::util::enumNameSafe(portStateToStateEnum(target));
} // namespace facebook::fboss
}
;

BOOST_MSM_EUML_ACTION(portProgramIphyPorts){
    template <class Event, class Fsm, class Source, class Target>
    bool operator()(
        const Event& /* ev */,
        Fsm &
            fsm,
        Source& /* src */,
        Target& /* trg */){auto portId = fsm.get_attribute(portID);
auto name = fsm.get_attribute(portName);
auto portMgr = fsm.get_attribute(portMgrPtr);
if (!portMgr) {
  return false;
}

try {
  if (portMgr->isLowestIndexedInitializedPortForTransceiverPortGroup(portId)) {
    // Port should orchestrate PHY programming.
    // First transceiver will communicate which pins need to be programmed (for
    // both ports) - so only one transceiver needs to request information from
    // agent.

    portMgr->programInternalPhyPorts(
        portMgr->getLowestIndexedStaticTransceiverForPort(portId));
  } else {
    // Port shouldn't orchestrate PHY programming. Port needs to check state
    // of orchestrating port to proceed to next stage.
    auto lowestIdxPort =
        portMgr->getLowestIndexedInitializedPortForTransceiverPortGroup(portId);
    return portMgr->hasPortFinishedIphyProgramming(lowestIdxPort);
  }
  return true;
} catch (const std::exception& ex) {
  // We have retry mechanism to handle failure. No crash here
  XLOG(WARN) << "[Port:" << name
             << "] programInternalPhyPorts failed:" << folly::exceptionStr(ex);
  return false;
}
}
}
;

BOOST_MSM_EUML_ACTION(portProgramXphyPorts){
    template <class Event, class Fsm, class Source, class Target>
    bool operator()(
        const Event& /* ev */,
        Fsm &
            fsm,
        Source& /* src */,
        Target& /* trg */){auto portId = fsm.get_attribute(portID);
auto name = fsm.get_attribute(portName);
auto portMgr = fsm.get_attribute(portMgrPtr);
if (!portMgr) {
  return false;
}

try {
  if (portMgr->isLowestIndexedInitializedPortForTransceiverPortGroup(portId)) {
    // Port should orchestrate PHY programming.
    // TODO(smenta) – When Y-Cable support is needed for XPHY tcvrs, may need to
    // loop through all transceivers for ports.
    auto tcvrId = portMgr->getLowestIndexedStaticTransceiverForPort(portId);
    portMgr->programExternalPhyPorts(
        tcvrId, fsm.get_attribute(xphyNeedResetDataPath));
  } else {
    // Port shouldn't orchestrate PHY programming. Port needs to check state
    // of orchestrating port to proceed to next stage.
    auto lowestIdxPort =
        portMgr->getLowestIndexedInitializedPortForTransceiverPortGroup(portId);
    return portMgr->hasPortFinishedXphyProgramming(lowestIdxPort);
  }
  return true;
} catch (const std::exception& ex) {
  // We have retry mechanism to handle failure. No crash here
  XLOG(WARN) << "[Port:" << name
             << "] programExternalPhyPorts failed:" << folly::exceptionStr(ex);
  return false;
}
}
}
;

BOOST_MSM_EUML_ACTION(checkTransceiversProgrammed){
    template <class Event, class Fsm, class Source, class Target>
    bool operator()(
        const Event& /* ev */,
        Fsm &
            fsm,
        Source& /* src */,
        Target& /* trg */){auto portId = fsm.get_attribute(portID);
auto portMgr = fsm.get_attribute(portMgrPtr);
if (!portMgr) {
  return false;
}

if (portMgr->arePortTcvrsProgrammed(portId)) {
  fsm.get_attribute(xphyNeedResetDataPath) = false;
  return true;
}
return false;
}
}
;

// Port State Machine State transition table
// clang-format off
BOOST_MSM_EUML_TRANSITION_TABLE((
//  Start                             + Event                      [Guard]                                 / Action              == Next
// +----------------------------------------------------------------------------------------------------------------------------------+
    // Initialize port state machine if we are using this specific port speed / lane configuration.
    PORT_STATE_UNINITIALIZED              + INITIALIZE_PORT                                                / portLogStateChanged == PORT_STATE_INITIALIZED,

    // Programming Events
    PORT_STATE_INITIALIZED                + PORT_PROGRAM_IPHY      [portProgramIphyPorts]                  / portLogStateChanged == PORT_STATE_IPHY_PORTS_PROGRAMMED,
    PORT_STATE_IPHY_PORTS_PROGRAMMED      + PORT_PROGRAM_XPHY      [portProgramXphyPorts]                  / portLogStateChanged == PORT_STATE_XPHY_PORTS_PROGRAMMED,
    // If all transceivers assigned to a port are programmed, progress to next stage. If there is no XPHY, we can progress directly to tcvrs programmed.
    PORT_STATE_IPHY_PORTS_PROGRAMMED      + CHECK_TCVRS_PROGRAMMED [checkTransceiversProgrammed]           / portLogStateChanged == PORT_STATE_TRANSCEIVERS_PROGRAMMED,
    PORT_STATE_XPHY_PORTS_PROGRAMMED      + CHECK_TCVRS_PROGRAMMED [checkTransceiversProgrammed]           / portLogStateChanged == PORT_STATE_TRANSCEIVERS_PROGRAMMED,

    // Update port state based on updates from agent.
    PORT_STATE_TRANSCEIVERS_PROGRAMMED    + SET_PORT_UP                                                    / portLogStateChanged == PORT_STATE_PORT_UP,
    PORT_STATE_TRANSCEIVERS_PROGRAMMED    + SET_PORT_DOWN                                                  / portLogStateChanged == PORT_STATE_PORT_DOWN,
    PORT_STATE_PORT_UP                    + SET_PORT_DOWN                                                  / portLogStateChanged == PORT_STATE_PORT_DOWN,
    PORT_STATE_PORT_DOWN                  + SET_PORT_UP                                                    / portLogStateChanged == PORT_STATE_PORT_UP,

    // When agent config changes or port enabled status changes, we want to uninitialize certain ports, because port speed / lane config might change.
    PORT_STATE_INITIALIZED                + RESET_TO_UNINITIALIZED                                         / portLogStateChanged == PORT_STATE_UNINITIALIZED,
    PORT_STATE_IPHY_PORTS_PROGRAMMED      + RESET_TO_UNINITIALIZED                                         / portLogStateChanged == PORT_STATE_UNINITIALIZED,
    PORT_STATE_XPHY_PORTS_PROGRAMMED      + RESET_TO_UNINITIALIZED                                         / portLogStateChanged == PORT_STATE_UNINITIALIZED,
    PORT_STATE_TRANSCEIVERS_PROGRAMMED    + RESET_TO_UNINITIALIZED                                         / portLogStateChanged == PORT_STATE_UNINITIALIZED,
    PORT_STATE_PORT_UP                    + RESET_TO_UNINITIALIZED                                         / portLogStateChanged == PORT_STATE_UNINITIALIZED,
    PORT_STATE_PORT_DOWN                  + RESET_TO_UNINITIALIZED                                         / portLogStateChanged == PORT_STATE_UNINITIALIZED,

    // When we reset transceiver, remove transceiver, firmware upgrade, etc., port needs to be reset to initialized
    // so it can go through programming events again.
    PORT_STATE_IPHY_PORTS_PROGRAMMED      + RESET_TO_INITIALIZED                                           / portLogStateChanged == PORT_STATE_INITIALIZED,
    PORT_STATE_XPHY_PORTS_PROGRAMMED      + RESET_TO_INITIALIZED                                           / portLogStateChanged == PORT_STATE_INITIALIZED,
    PORT_STATE_TRANSCEIVERS_PROGRAMMED    + RESET_TO_INITIALIZED                                           / portLogStateChanged == PORT_STATE_INITIALIZED,
    PORT_STATE_PORT_DOWN                  + RESET_TO_INITIALIZED                                           / portLogStateChanged == PORT_STATE_INITIALIZED
//  +--------------------------------------------------------------------------------------------------------------------------------+
 ), PortTransitionTable)
// clang-format on

// Define a Port State Machine
BOOST_MSM_EUML_DECLARE_STATE_MACHINE(
    (PortTransitionTable,
     init_ << PORT_STATE_UNINITIALIZED,
     no_action,
     no_action,
     attributes_ << portMgrPtr << portID << portName << xphyNeedResetDataPath),
    PortStateMachine)

} // namespace facebook::fboss
