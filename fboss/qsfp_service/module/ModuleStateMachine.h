// Copyright 2004-present Facebook. All Rights Reserved.

/*
 * This file contans implementation of the State Machine to describe Optics
 * Module software states and transitions. The state machine is implemented
 * using C++ Boost library Meta State Machine (MSM) with eUML.
 * This file has Module State Machine and the Port State Machine. One Optics
 * Module will have one Module SM and one or more than one Port SM. The State
 * Machine objects will be declared outside this file.
 */

#pragma once

#include <boost/msm/back/state_machine.hpp>
#include <boost/msm/front/euml/euml.hpp>
#include <boost/msm/front/euml/state_grammar.hpp>
#include <iostream>
#include <vector>

namespace facebook {
namespace fboss {

namespace msm = boost::msm;
using namespace boost::msm::front::euml;

class QsfpModule;

/**************************** Module State Machine ***************************/

// Module state machine attribute telling whether the module bring up is
// already done or not yet
BOOST_MSM_EUML_DECLARE_ATTRIBUTE(bool, moduleBringupDone)

// Module State Machine will also have one attribute containing the QsfpModule
// object raw pointer. This is needed because the MSM calls event action
// functions asynchronously and in those function calls sometime we need to
// access QsfpModule object variables like Port State Machine handler etc
BOOST_MSM_EUML_DECLARE_ATTRIBUTE(class QsfpModule*, qsfpModuleObjPtr)

/*
 * moduleDiscoveredStateEntryFn
 * Type: State Entry Function
 *
 * State Entry Function for the state MODULE_STATE_DISCOVERED
 * This function will create the Port State Machines for all ports present in
 * this module
 */
BOOST_MSM_EUML_ACTION(moduleDiscoveredStateEntryFn){
    template <class Event, class Fsm, class State>
    void
    operator()(const Event& /* ev */, Fsm& fsm, State& /* currState */) const {
        std::cout << "moduleDiscoveredStateEntryFn: "
                  << "Entering state MODULE_STATE_DISCOVERED, "
                  << "need to create Port State Machines\n";
// Module is discovered, now create port state machines
fsm.get_attribute(qsfpModuleObjPtr)->addModulePortStateMachines();
} // namespace fboss
}; // namespace facebook

/*
 * moduleDiscoveredStateExitFn
 * Type: State Exit Function
 *
 * State Exit Function for state MODULE_STATE_DISCOVERED
 */
BOOST_MSM_EUML_ACTION(moduleDiscoveredStateExitFn){
    template <class Event, class Fsm, class State>
    void
    operator()(const Event& /* ev */, Fsm& /* fsm */, State& /* currState */)
        const {
            std::cout << "moduleDiscoveredStateExitFn: "
                      << "Exiting state MODULE_STATE_DISCOVERED\n";
}
}
;

/*
 * moduleInactiveStateEntryFn
 * Type: State Entry Function
 *
 * State Entry Function for the state MODULE_STATE_INACTIVE
 * When the Module SM enters the Synced Not Active state then
 * (a) It needs to create a timer thread to bring up and remediate action.
 * (b) In the first invocation of timer the Bring up should be attempted
 *     and the Bring up done event should be generated.
 * (c) In the second onward invocation (if attribute bring up done is true)
 *     it should perform remediate action and issue remediate done event
 */
BOOST_MSM_EUML_ACTION(moduleInactiveStateEntryFn){
    template <class Event, class Fsm, class State>
    void
    operator()(const Event& /* ev */, Fsm& /* fsm */, State& /* currState */)
        const {
            std::cout << "moduleInactiveStateEntryFn: "
                      << "Entering state MODULE_STATE_INACTIVE\n";
}
}
;

/*
 * moduleInactiveStateExitFn
 * Type: State Exit Function
 *
 * State Exit Function for the state MODULE_STATE_INACTIVE
 * When the Module SM exits the Synced Not Active state then it needs
 * to delete the timer thread which was create during entry of state
 */
BOOST_MSM_EUML_ACTION(moduleInactiveStateExitFn){
    template <class Event, class Fsm, class State>
    void
    operator()(const Event& /* ev */, Fsm& /* fsm */, State& /* currState */)
        const {
            std::cout << "moduleInactiveStateExitFn: "
                      << "Exiting state MODULE_STATE_INACTIVE\n";
}
}
;

// Module State Machine States
BOOST_MSM_EUML_STATE((), MODULE_STATE_NOT_PRESENT)
BOOST_MSM_EUML_STATE((), MODULE_STATE_PRESENT)
BOOST_MSM_EUML_STATE(
    (moduleDiscoveredStateEntryFn, moduleDiscoveredStateExitFn),
    MODULE_STATE_DISCOVERED)
BOOST_MSM_EUML_STATE(
    (moduleInactiveStateEntryFn, moduleInactiveStateExitFn),
    MODULE_STATE_INACTIVE)
BOOST_MSM_EUML_STATE((), MODULE_STATE_ACTIVE)
BOOST_MSM_EUML_STATE((), MODULE_STATE_UPGRADING)

// Module State Machine Events
BOOST_MSM_EUML_EVENT(MODULE_EVENT_OPTICS_DETECTED)
BOOST_MSM_EUML_EVENT(MODULE_EVENT_OPTICS_REMOVED)
BOOST_MSM_EUML_EVENT(MODULE_EVENT_EEPROM_READ)
BOOST_MSM_EUML_EVENT(MODULE_EVENT_PSM_MODPORTS_DOWN)
BOOST_MSM_EUML_EVENT(MODULE_EVENT_PSM_MODPORT_UP)
BOOST_MSM_EUML_EVENT(MODULE_EVENT_AGENT_SYNC_TIMEOUT)
BOOST_MSM_EUML_EVENT(MODULE_EVENT_BRINGUP_DONE)
BOOST_MSM_EUML_EVENT(MODULE_EVENT_REMEDIATE_DONE)
BOOST_MSM_EUML_EVENT(MODULE_EVENT_TRIGGER_UPGRADE)
BOOST_MSM_EUML_EVENT(MODULE_EVENT_FORCED_UPGRADE)
BOOST_MSM_EUML_EVENT(MODULE_EVENT_OPTICS_RESET)

/*
 * onOpticsDetected
 * Type: Event based action
 *
 * On optics detection the state transitions to Module Present
 */
BOOST_MSM_EUML_ACTION(
    onOpticsDetected){template <class Event, class Fsm> void operator()(
    const Event& /* ev */,
    Fsm& /* fsm */,
    BOOST_MSM_EUML_STATE_NAME(MODULE_STATE_NOT_PRESENT) & /* sourceState */,
    BOOST_MSM_EUML_STATE_NAME(MODULE_STATE_PRESENT) &
    /* targetState */) const {
    std::cout << "onOpticsDetected: Transitioning to MODULE_STATE_PRESENT\n";
}
}
;

/*
 * onEepromRead
 * Type: Event based action
 *
 * After Module EEPROM read the state transitions to Module Discovered
 */
BOOST_MSM_EUML_ACTION(
    onEepromRead){template <class Event, class Fsm> void operator()(
    const Event& /* ev */,
    Fsm& /* fsm */,
    BOOST_MSM_EUML_STATE_NAME(MODULE_STATE_PRESENT) & /* sourceState */,
    BOOST_MSM_EUML_STATE_NAME(MODULE_STATE_DISCOVERED) &
    /* targetState */) const {
    std::cout << "onEepromRead: Transitioning to MODULE_STATE_DISCOVERED\n";
}
}
;

/*
 * onModuleAllPortsDown
 * Type: Event based action
 *
 * Once the Agent sync up happens and all ports of a module is identified as
 * Down then the module state transitions to Module Synced Down state when the
 * module first time bring up has not happened. If the first time bring up
 * has happened then the module state transitions to module not active state
 */
BOOST_MSM_EUML_ACTION(
    onModuleAllPortsDown){template <class Event, class Fsm> void operator()(
    const Event& /* ev */,
    Fsm& /* fsm */,
    BOOST_MSM_EUML_STATE_NAME(MODULE_STATE_DISCOVERED) & /* sourceState */,
    BOOST_MSM_EUML_STATE_NAME(MODULE_STATE_INACTIVE) &
    /* targetState */) const {
    std::cout << "onModuleAllPortsDown: "
              << "Transitioning to MODULE_STATE_INACTIVE\n";
}

template <class Event, class Fsm>
void operator()(
    const Event& /* ev */,
    Fsm& /* fsm */,
    BOOST_MSM_EUML_STATE_NAME(MODULE_STATE_ACTIVE) & /* sourceState */,
    BOOST_MSM_EUML_STATE_NAME(MODULE_STATE_INACTIVE) &
    /* targetState */) const {
  std::cout << "onModuleAllPortsDown: "
            << "Transitioning to MODULE_STATE_INACTIVE\n";
}
}
;

/*
 * onAgentSyncTimeout
 * Type: Event based action
 *
 * If the Agent sync up does not happen and the port state information does
 * not come then modue state machine should transition from Discovered state
 * to Synced down state. If the module was in Ready state then it should also
 * transition to module not active state. This timeout will take care of case
 * when Agent crashes or the Agent is not running.
 */
BOOST_MSM_EUML_ACTION(
    onAgentSyncTimeout){template <class Event, class Fsm> void operator()(
    const Event& /* ev */,
    Fsm& /* fsm */,
    BOOST_MSM_EUML_STATE_NAME(MODULE_STATE_DISCOVERED) & /* sourceState */,
    BOOST_MSM_EUML_STATE_NAME(MODULE_STATE_INACTIVE) &
    /* targetState */) const {
    std::cout << "onAgentSyncTimeout: "
              << "Transitioning to MODULE_STATE_INACTIVE\n";
}
}
;

/*
 * onModuleAnyPortUp
 * Type: Event based action
 *
 * When Agent informs that some of the port in module are up then the state
 * needs to transition from Discovered to Active. The state also moves from
 * Ready or Not Active state to Active state.
 */
BOOST_MSM_EUML_ACTION(
    onModuleAnyPortUp){template <class Event, class Fsm> void operator()(
    const Event& /* ev */,
    Fsm& fsm,
    BOOST_MSM_EUML_STATE_NAME(MODULE_STATE_DISCOVERED) & /* sourceState */,
    BOOST_MSM_EUML_STATE_NAME(MODULE_STATE_ACTIVE) &
    /* targetState */) const {
    std::cout << "onModuleAnyPortUp: Transitioning to MODULE_STATE_ACTIVE\n";
// When Module state was Discovered and Agent informed that some of the
// port are up then the state need to move to Active. There is no need to
// do any bring up in this case. This will take care of warm boot case
fsm.get_attribute(moduleBringupDone) = true;
}

template <class Event, class Fsm>
void operator()(
    const Event& /* ev */,
    Fsm& fsm,
    BOOST_MSM_EUML_STATE_NAME(MODULE_STATE_INACTIVE) & /* sourceState */,
    BOOST_MSM_EUML_STATE_NAME(MODULE_STATE_ACTIVE) &
    /* targetState */) const {
  std::cout << "onModuleAnyPortUp: Transitioning to MODULE_STATE_ACTIVE\n";
  // When Agent informed that some of the port are up then the state need to
  // move to Active state.
  fsm.get_attribute(moduleBringupDone) = true;
}
}
;

/*
 * onTriggerUpgrade
 * Type: Event based action
 *
 * The firmware update can be triggered from the initialization code when
 * optics is detected or the run time when optics is plugged in and it is
 * detected by software. This is also triggered by wedge_qsfp_util cli via
 * thrift call to qsfp_service. When module state is Synced down then the
 * update trigger will move the state to Upgrading
 */
BOOST_MSM_EUML_ACTION(
    onTriggerUpgrade){template <class Event, class Fsm> void operator()(
    const Event& /* ev */,
    Fsm& /* fsm */,
    BOOST_MSM_EUML_STATE_NAME(MODULE_STATE_INACTIVE) & /* sourceState */,
    BOOST_MSM_EUML_STATE_NAME(MODULE_STATE_UPGRADING) &
    /* targetState */) const {
    std::cout << "onTriggerUpgrade: Transitioning to MODULE_STATE_UPGRADING\n";
}
}
;

/*
 * onForcedUpgrade
 * Type: Event based action
 *
 * The firmware update can be triggered from the CLI using forced upgrade.
 * In this event the module start firmware upgrade no matter it is in either
 * Active state or Inactive state. The update trigger will move the state
 * to Upgrading
 */
BOOST_MSM_EUML_ACTION(
    onForcedUpgrade){template <class Event, class Fsm> void operator()(
    const Event& /* ev */,
    Fsm& /* fsm */,
    BOOST_MSM_EUML_STATE_NAME(MODULE_STATE_INACTIVE) & /* sourceState */,
    BOOST_MSM_EUML_STATE_NAME(MODULE_STATE_UPGRADING) &
    /* targetState */) const {
    std::cout << "onForcedUpgrade: Transitioning to MODULE_STATE_UPGRADING\n";
}

template <class Event, class Fsm>
void operator()(
    const Event& /* ev */,
    Fsm& /* fsm */,
    BOOST_MSM_EUML_STATE_NAME(MODULE_STATE_ACTIVE) & /* sourceState */,
    BOOST_MSM_EUML_STATE_NAME(MODULE_STATE_UPGRADING) &
    /* targetState */) const {
  std::cout << "onForcedUpgrade: Transitioning to MODULE_STATE_UPGRADING\n";
}
}
;

/*
 * onBringupDone
 * Type: Event based action
 *
 * When optics bring up is done then irrespective of  outcome it stays in Synced
 * Not Active state. We depend on Agent to tell if the port goes up or not. If
 * the port goes up then it means bring up was successful. We set the module
 * bring up done flag to prevent it from happening next time
 */
BOOST_MSM_EUML_ACTION(
    onBringupDone){template <class Event, class Fsm> void operator()(
    const Event& /* ev */,
    Fsm& fsm,
    BOOST_MSM_EUML_STATE_NAME(MODULE_STATE_INACTIVE) & /* sourceState */,
    BOOST_MSM_EUML_STATE_NAME(MODULE_STATE_INACTIVE) &
    /* targetState */) const {
    std::cout << "onBringupDone: Stay in MODULE_STATE_INACTIVE\n";
// Module bring up has been tried, set the attribute for this in MSM
fsm.get_attribute(moduleBringupDone) = true;
}
}
;

/*
 * onRemediateDone
 * Type: Event based action
 *
 * When optics remediate is done then irrespective of  outcome it stays in
 * Synced Not Active state. We depend on Agent to tell if the port goes up or
 * not. If the port goes up then it means remediate was successful.
 */
BOOST_MSM_EUML_ACTION(
    onRemediateDone){template <class Event, class Fsm> void operator()(
    const Event& /* ev */,
    Fsm& /* fsm */,
    BOOST_MSM_EUML_STATE_NAME(MODULE_STATE_INACTIVE) & /* sourceState */,
    BOOST_MSM_EUML_STATE_NAME(MODULE_STATE_INACTIVE) &
    /* targetState */) const {
    std::cout << "onRemediateDone: Stay in MODULE_STATE_INACTIVE\n";
}
}
;

/*
 * onOpticsReset
 * Type: Event based action
 *
 * When the optics is removed from any state then the module state goes to  Not
 * Present state
 */
BOOST_MSM_EUML_ACTION(onOpticsReset){
    template <class Event, class Fsm, class Source, class Target>
    void
    operator()(
        const Event& /* ev */,
        Fsm& fsm,
        Source& /* sourceState */,
        Target& /* targetState */) const {
        std::cout
        << "onOpticsReset: Transitioning to MODULE_STATE_NOT_PRESENT\n";
std::cout << "onOpticsReset: Delete the PSM\n";
// Module has been reset so remove port state machines inside this module
fsm.get_attribute(qsfpModuleObjPtr)->eraseModulePortStateMachines();
// Make the module bring up attribute as False now
fsm.get_attribute(moduleBringupDone) = false;
}
}
;

/*
 * onOpticsRemoval
 * Type: Event based action
 *
 * When the optics is removed from any state then the module state goes to  Not
 * Present state
 */
BOOST_MSM_EUML_ACTION(onOpticsRemoval){
    template <class Event, class Fsm, class Source, class Target>
    void
    operator()(
        const Event& /* ev */,
        Fsm& fsm,
        Source& /* sourceState */,
        Target& /* targetState */) const {
        std::cout
        << "onOpticsRemoval: Transitioning to MODULE_STATE_NOT_PRESENT\n";
std::cout << "onOpticsRemoval: Delete the PSM\n";
// Module has been removed so remove port state machines inside this module
fsm.get_attribute(qsfpModuleObjPtr)->eraseModulePortStateMachines();
// Make the module bring up attribute as False now
fsm.get_attribute(moduleBringupDone) = false;
}
}
;

// Module State Machine State transition table
BOOST_MSM_EUML_TRANSITION_TABLE(
    (MODULE_STATE_NOT_PRESENT +
             MODULE_EVENT_OPTICS_DETECTED / onOpticsDetected ==
         MODULE_STATE_PRESENT,
     MODULE_STATE_PRESENT + MODULE_EVENT_EEPROM_READ / onEepromRead ==
         MODULE_STATE_DISCOVERED,
     MODULE_STATE_DISCOVERED +
             MODULE_EVENT_PSM_MODPORTS_DOWN / onModuleAllPortsDown ==
         MODULE_STATE_INACTIVE,
     MODULE_STATE_DISCOVERED +
             MODULE_EVENT_AGENT_SYNC_TIMEOUT / onAgentSyncTimeout ==
         MODULE_STATE_INACTIVE,
     MODULE_STATE_DISCOVERED +
             MODULE_EVENT_PSM_MODPORT_UP / onModuleAnyPortUp ==
         MODULE_STATE_ACTIVE,
     MODULE_STATE_INACTIVE + MODULE_EVENT_PSM_MODPORT_UP / onModuleAnyPortUp ==
         MODULE_STATE_ACTIVE,
     MODULE_STATE_INACTIVE + MODULE_EVENT_TRIGGER_UPGRADE / onTriggerUpgrade ==
         MODULE_STATE_UPGRADING,
     MODULE_STATE_INACTIVE + MODULE_EVENT_FORCED_UPGRADE / onForcedUpgrade ==
         MODULE_STATE_UPGRADING,
     MODULE_STATE_INACTIVE + MODULE_EVENT_BRINGUP_DONE / onBringupDone ==
         MODULE_STATE_INACTIVE,
     MODULE_STATE_INACTIVE + MODULE_EVENT_REMEDIATE_DONE / onRemediateDone ==
         MODULE_STATE_INACTIVE,
     MODULE_STATE_INACTIVE + MODULE_EVENT_OPTICS_RESET / onOpticsReset ==
         MODULE_STATE_NOT_PRESENT,
     MODULE_STATE_ACTIVE +
             MODULE_EVENT_PSM_MODPORTS_DOWN / onModuleAllPortsDown ==
         MODULE_STATE_INACTIVE,
     MODULE_STATE_ACTIVE + MODULE_EVENT_FORCED_UPGRADE / onForcedUpgrade ==
         MODULE_STATE_UPGRADING,
     MODULE_STATE_UPGRADING + MODULE_EVENT_OPTICS_RESET / onOpticsReset ==
         MODULE_STATE_NOT_PRESENT,
     MODULE_STATE_PRESENT + MODULE_EVENT_OPTICS_REMOVED / onOpticsRemoval ==
         MODULE_STATE_NOT_PRESENT,
     MODULE_STATE_DISCOVERED + MODULE_EVENT_OPTICS_REMOVED / onOpticsRemoval ==
         MODULE_STATE_NOT_PRESENT,
     MODULE_STATE_INACTIVE + MODULE_EVENT_OPTICS_REMOVED / onOpticsRemoval ==
         MODULE_STATE_NOT_PRESENT,
     MODULE_STATE_ACTIVE + MODULE_EVENT_OPTICS_REMOVED / onOpticsRemoval ==
         MODULE_STATE_NOT_PRESENT,
     MODULE_STATE_UPGRADING + MODULE_EVENT_OPTICS_REMOVED / onOpticsRemoval ==
         MODULE_STATE_NOT_PRESENT),
    moduleTransitionTable)

// Define a Module State Machine
BOOST_MSM_EUML_DECLARE_STATE_MACHINE(
    (moduleTransitionTable,
     init_ << MODULE_STATE_NOT_PRESENT,
     no_action,
     no_action,
     attributes_ << moduleBringupDone << qsfpModuleObjPtr),
    moduleStateMachine)

/**************************** Port State Machine *****************************/

// Port State Machine States
BOOST_MSM_EUML_STATE((), MODULE_PORT_STATE_IDLE)
BOOST_MSM_EUML_STATE((), MODULE_PORT_STATE_INITIALIZED)
BOOST_MSM_EUML_STATE((), MODULE_PORT_STATE_DOWN)
BOOST_MSM_EUML_STATE((), MODULE_PORT_STATE_UP)

// Port State Machine Events
BOOST_MSM_EUML_EVENT(MODULE_PORT_EVENT_OPTICS_INITIALIZED)
BOOST_MSM_EUML_EVENT(MODULE_PORT_EVENT_AGENT_PORT_UP)
BOOST_MSM_EUML_EVENT(MODULE_PORT_EVENT_AGENT_PORT_DOWN)

/*
 * onModulePortInitialize
 * Type: Event based action
 *
 * When the Module's port level initialization is done then the Port State
 * Machine moves to Initialized state
 */
BOOST_MSM_EUML_ACTION(
    onModulePortInitialize){template <class Event, class Fsm> void operator()(
    const Event& /* ev */,
    Fsm& /* fsm */,
    BOOST_MSM_EUML_STATE_NAME(MODULE_PORT_STATE_IDLE) & /* sourceState */,
    BOOST_MSM_EUML_STATE_NAME(MODULE_PORT_STATE_INITIALIZED) &
    /* targetState */) const {
    std::cout << "onModulePortInitialize: "
              << "Transitioning to MODULE_PORT_STATE_INITIALIZED\n";
}
}
;

/*
 * onModulePortAgentUp
 * Type: Event based action
 *
 * When the Agent sync up happens and any of the port is declared to be up
 * then the Port State Machine moves to Port Up state. This transition also
 * leads to generation of Module port up event to Module State Machine
 */
BOOST_MSM_EUML_ACTION(
    onModulePortAgentUp){template <class Event, class Fsm> void operator()(
    const Event& /* ev */,
    Fsm& fsm,
    BOOST_MSM_EUML_STATE_NAME(
        MODULE_PORT_STATE_INITIALIZED) & /* sourceState */,
    BOOST_MSM_EUML_STATE_NAME(MODULE_PORT_STATE_UP) &
    /* targetState */) const {
    std::cout << "onModulePortAgentUp: Transitioning to MODULE_PORT_STATE_UP\n";
// This transition generates Module Port Up event to Module State Machine
fsm.get_attribute(qsfpModuleObjPtr)->genMsmModPortUpEvent();
}

template <class Event, class Fsm>
void operator()(
    const Event& /* ev */,
    Fsm& fsm,
    BOOST_MSM_EUML_STATE_NAME(MODULE_PORT_STATE_DOWN) & /* sourceState */,
    BOOST_MSM_EUML_STATE_NAME(MODULE_PORT_STATE_UP) &
    /* targetState */) const {
  std::cout << "onModulePortAgentUp: Transitioning to MODULE_PORT_STATE_UP\n";
  // This transition generates Module Port Up event to Module State Machine
  fsm.get_attribute(qsfpModuleObjPtr)->genMsmModPortUpEvent();
}
}
;

/*
 * onModulePortAgentDown
 * Type: Event based action
 *
 * When the Agent sync up happens and all of the port are declared to down
 * then the Port State Machine moves to Port Down state. We check if all
 * other module port state for this module is in Down state, if it is so
 * then from here we also generate the Module port Down event to Module
 * State Machine
 */
BOOST_MSM_EUML_ACTION(
    onModulePortAgentDown){template <class Event, class Fsm> void operator()(
    const Event& /* ev */,
    Fsm& fsm,
    BOOST_MSM_EUML_STATE_NAME(
        MODULE_PORT_STATE_INITIALIZED) & /* sourceState */,
    BOOST_MSM_EUML_STATE_NAME(MODULE_PORT_STATE_DOWN) &
    /* targetState */) const {
    std::cout << "onModulePortAgentDown: "
              << "Transitioning to MODULE_PORT_STATE_DOWN\n";
// This module port has gone down, check if all other module port are
// also down If that is the case then generate the Module Port Down event
// to the Module State Machine
fsm.get_attribute(qsfpModuleObjPtr)->genMsmModPortsDownEvent();
}

template <class Event, class Fsm>
void operator()(
    const Event& /* ev */,
    Fsm& fsm,
    BOOST_MSM_EUML_STATE_NAME(MODULE_PORT_STATE_UP) & /* sourceState */,
    BOOST_MSM_EUML_STATE_NAME(MODULE_PORT_STATE_DOWN) &
    /* targetState */) const {
  std::cout << "onModulePortAgentDown: "
            << "Transitioning to MODULE_PORT_STATE_DOWN\n";
  // This module port has gone down, check if all other module port are
  // also down If that is the case then generate the Module Port Down event
  // to the Module State Machine
  fsm.get_attribute(qsfpModuleObjPtr)->genMsmModPortsDownEvent();
}
}
;

// Port State Machine state transition table
BOOST_MSM_EUML_TRANSITION_TABLE(
    (MODULE_PORT_STATE_IDLE +
             MODULE_PORT_EVENT_OPTICS_INITIALIZED / onModulePortInitialize ==
         MODULE_PORT_STATE_INITIALIZED,
     MODULE_PORT_STATE_INITIALIZED +
             MODULE_PORT_EVENT_AGENT_PORT_UP / onModulePortAgentUp ==
         MODULE_PORT_STATE_UP,
     MODULE_PORT_STATE_INITIALIZED +
             MODULE_PORT_EVENT_AGENT_PORT_DOWN / onModulePortAgentDown ==
         MODULE_PORT_STATE_DOWN,
     MODULE_PORT_STATE_DOWN +
             MODULE_PORT_EVENT_AGENT_PORT_UP / onModulePortAgentUp ==
         MODULE_PORT_STATE_UP,
     MODULE_PORT_STATE_UP +
             MODULE_PORT_EVENT_AGENT_PORT_DOWN / onModulePortAgentDown ==
         MODULE_PORT_STATE_DOWN),
    modulePortTransitionTable)

// Define Port State Machine
BOOST_MSM_EUML_DECLARE_STATE_MACHINE(
    (modulePortTransitionTable,
     init_ << MODULE_PORT_STATE_IDLE,
     no_action,
     no_action,
     attributes_ << moduleBringupDone << qsfpModuleObjPtr),
    modulePortStateMachine)
}
} // namespace facebook::fboss
