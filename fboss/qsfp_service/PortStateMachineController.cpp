// Copyright 2021-present Facebook. All Rights Reserved.

#include <fmt/format.h>
#include "fboss/agent/types.h"
#include "fboss/qsfp_service/PortManager.h"
#include "fboss/qsfp_service/PortStateMachine.h"
#include "fboss/qsfp_service/StateMachineController.h"

namespace facebook::fboss {
using PortStateMachineController = StateMachineController<
    PortID,
    PortStateMachineEvent,
    PortStateMachineState,
    PortStateMachine>;

template <>
PortStateMachineState PortStateMachineController::getStateByOrder(
    int currentStateOrder) const {
  switch (currentStateOrder) {
    case 0:
      return PortStateMachineState::UNINITIALIZED;
    case 1:
      return PortStateMachineState::INITIALIZED;
    case 2:
      return PortStateMachineState::IPHY_PORTS_PROGRAMMED;
    case 3:
      return PortStateMachineState::XPHY_PORTS_PROGRAMMED;
    case 4:
      return PortStateMachineState::TRANSCEIVERS_PROGRAMMED;
    case 5:
      return PortStateMachineState::PORT_UP;
    case 6:
      return PortStateMachineState::PORT_DOWN;
    default:
      throw FbossError(
          "Unsupported PortStateMachineState order: ", currentStateOrder);
  }
}

template <>
std::string PortStateMachineController::getUpdateString(
    PortStateMachineEvent event) const {
  return fmt::format(
      "[Port: {}, Event:{}]", id_, apache::thrift::util::enumNameSafe(event));
};

template <>
void PortStateMachineController::applyUpdate(PortStateMachineEvent event) {
  XLOG(INFO) << "Applying PortStateMachine Update for "
             << getUpdateString(event);
  auto lockedStateMachine = stateMachine_.wlock();
  auto preState = *lockedStateMachine->current_state();

  switch (event) {
    case PortStateMachineEvent::PORT_EV_INITIALIZE_PORT:
      lockedStateMachine->process_event(INITIALIZE_PORT);
      break;
    case PortStateMachineEvent::PORT_EV_PROGRAM_IPHY:
      lockedStateMachine->process_event(PORT_PROGRAM_IPHY);
      break;
    case PortStateMachineEvent::PORT_EV_PROGRAM_XPHY:
      lockedStateMachine->process_event(PORT_PROGRAM_XPHY);
      break;
    case PortStateMachineEvent::PORT_EV_CHECK_TCVRS_PROGRAMMED:
      lockedStateMachine->process_event(CHECK_TCVRS_PROGRAMMED);
      break;
    case PortStateMachineEvent::PORT_EV_SET_PORT_UP:
      lockedStateMachine->process_event(SET_PORT_UP);
      break;
    case PortStateMachineEvent::PORT_EV_SET_PORT_DOWN:
      lockedStateMachine->process_event(SET_PORT_DOWN);
      break;
    case PortStateMachineEvent::PORT_EV_RESET_TO_UNINITIALIZED:
      lockedStateMachine->process_event(RESET_TO_UNINITIALIZED);
      break;
    case PortStateMachineEvent::PORT_EV_RESET_TO_INITIALIZED:
      lockedStateMachine->process_event(RESET_TO_INITIALIZED);
      break;
    default:
      XLOG(ERR) << "Unsupported PortStateMachineEvent for "
                << getUpdateString(event);
  }

  auto postState = *lockedStateMachine->current_state();
  if (preState != postState) {
    // We only log successful state changes.
    XLOG(INFO) << "Successfully applied state update for "
               << getUpdateString(event) << " from "
               << apache::thrift::util::enumNameSafe(getStateByOrder(preState))
               << " to "
               << apache::thrift::util::enumNameSafe(
                      getStateByOrder(postState));
  } else {
    XLOG(ERR) << "Failed to apply PortStateMachineUpdate "
              << getUpdateString(event);
  }
};

template <>
void PortStateMachineController::logCurrentState(
    int curStateOrder,
    PortStateMachineState state) const {
  XLOG(DBG4) << "Current Port:" << static_cast<int32_t>(id_)
             << ", state order:" << curStateOrder
             << ", state:" << apache::thrift::util::enumNameSafe(state);
}

template <>
void PortStateMachineController::setStateMachineAttributes() {
  stateMachine_.withWLock([&](auto& stateMachine) {
    stateMachine.get_attribute(portID) = id_;
    stateMachine.get_attribute(xphyNeedResetDataPath) = false;
  });
}

} // namespace facebook::fboss
