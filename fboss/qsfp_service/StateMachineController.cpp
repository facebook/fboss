// Copyright 2021-present Facebook. All Rights Reserved.

#include "fboss/qsfp_service/StateMachineController.h"
#include <fmt/format.h>
#include "fboss/agent/types.h"
#include "fboss/qsfp_service/TransceiverManager.h"
#include "fboss/qsfp_service/TransceiverStateMachine.h"

namespace facebook::fboss {
using TransceiverStateMachineController = StateMachineController<
    TransceiverID,
    TransceiverStateMachineEvent,
    TransceiverStateMachineState,
    TransceiverStateMachine>;

template <>
TransceiverStateMachineState TransceiverStateMachineController::getStateByOrder(
    int currentStateOrder) const {
  switch (currentStateOrder) {
    case 0:
      return TransceiverStateMachineState::NOT_PRESENT;
    case 1:
      return TransceiverStateMachineState::PRESENT;
    case 2:
      return TransceiverStateMachineState::DISCOVERED;
    case 3:
      return TransceiverStateMachineState::IPHY_PORTS_PROGRAMMED;
    case 4:
      return TransceiverStateMachineState::XPHY_PORTS_PROGRAMMED;
    case 5:
      return TransceiverStateMachineState::TRANSCEIVER_READY;
    case 6:
      return TransceiverStateMachineState::TRANSCEIVER_PROGRAMMED;
    case 7:
      return TransceiverStateMachineState::ACTIVE;
    case 8:
      return TransceiverStateMachineState::INACTIVE;
    case 9:
      return TransceiverStateMachineState::UPGRADING;
    default:
      throw FbossError(
          "Unsupported TransceiverStateMachineState order: ",
          currentStateOrder);
  }
}

template <>
std::string TransceiverStateMachineController::getUpdateString(
    TransceiverStateMachineEvent event) const {
  return fmt::format(
      "[Transceiver: {}, Event:{}]",
      id_,
      apache::thrift::util::enumNameSafe(event));
};

template <>
void TransceiverStateMachineController::applyUpdate(
    TransceiverStateMachineEvent event) {
  XLOG(INFO) << "Applying TransceiverStateMachine Update for "
             << getUpdateString(event);
  auto lockedStateMachine = stateMachine_.wlock();
  auto preState = *lockedStateMachine->current_state();

  switch (event) {
    case TransceiverStateMachineEvent::TCVR_EV_EVENT_DETECT_TRANSCEIVER:
      lockedStateMachine->process_event(DETECT_TRANSCEIVER);
      break;
    case TransceiverStateMachineEvent::TCVR_EV_READ_EEPROM:
      lockedStateMachine->process_event(READ_EEPROM);
      break;
    case TransceiverStateMachineEvent::TCVR_EV_PROGRAM_IPHY:
      lockedStateMachine->process_event(PROGRAM_IPHY);
      break;
    case TransceiverStateMachineEvent::TCVR_EV_PROGRAM_XPHY:
      lockedStateMachine->process_event(PROGRAM_XPHY);
      break;
    case TransceiverStateMachineEvent::TCVR_EV_PREPARE_TRANSCEIVER:
      lockedStateMachine->process_event(PREPARE_TRANSCEIVER);
      break;
    case TransceiverStateMachineEvent::TCVR_EV_PROGRAM_TRANSCEIVER:
      lockedStateMachine->process_event(PROGRAM_TRANSCEIVER);
      break;
    case TransceiverStateMachineEvent::TCVR_EV_ALL_PORTS_DOWN:
      lockedStateMachine->process_event(ALL_PORTS_DOWN);
      break;
    case TransceiverStateMachineEvent::TCVR_EV_PORT_UP:
      lockedStateMachine->process_event(PORT_UP);
      break;
    case TransceiverStateMachineEvent::TCVR_EV_RESET_TO_NOT_PRESENT:
      lockedStateMachine->process_event(RESET_TO_NOT_PRESENT);
      break;
    case TransceiverStateMachineEvent::TCVR_EV_RESET_TO_DISCOVERED:
      lockedStateMachine->process_event(RESET_TO_DISCOVERED);
      break;
    case TransceiverStateMachineEvent::TCVR_EV_REMOVE_TRANSCEIVER:
      lockedStateMachine->process_event(REMOVE_TRANSCEIVER);
      break;
    case TransceiverStateMachineEvent::TCVR_EV_REMEDIATE_TRANSCEIVER:
      lockedStateMachine->process_event(REMEDIATE_TRANSCEIVER);
      break;
    case TransceiverStateMachineEvent::TCVR_EV_UPGRADE_FIRMWARE:
      lockedStateMachine->process_event(UPGRADE_FIRMWARE);
      break;
    default:
      XLOG(ERR) << "Unsupported TransceiverStateMachineEvent for "
                << getUpdateString(event);
  }

  auto postState = *lockedStateMachine->current_state();
  if (preState != postState) {
    // We only log successful state changes.
    XLOG_IF(INFO, preState && postState)
        << "Successfully applied state update for " << getUpdateString(event)
        << " from "
        << apache::thrift::util::enumNameSafe(getStateByOrder(preState))
        << " to "
        << apache::thrift::util::enumNameSafe(getStateByOrder(postState));
  }
};

template <>
void TransceiverStateMachineController::logCurrentState(
    int curStateOrder,
    TransceiverStateMachineState state) const {
  XLOG(DBG4) << "Current transceiver:" << static_cast<int32_t>(id_)
             << ", state order:" << curStateOrder
             << ", state:" << apache::thrift::util::enumNameSafe(state);
}

template <>
void TransceiverStateMachineController::setStateMachineAttributes() {
  stateMachine_.withWLock([&](auto& stateMachine) {
    stateMachine.get_attribute(transceiverID) = id_;
    stateMachine.get_attribute(needResetDataPath) = false;
  });
}

} // namespace facebook::fboss
