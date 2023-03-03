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

DEFINE_bool(use_new_state_machine, true, "Use the new state machine logic");

namespace facebook::fboss {

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
    return TransceiverStateMachineState::TRANSCEIVER_READY;
  } else if (currentStateOrder == 6) {
    return TransceiverStateMachineState::TRANSCEIVER_PROGRAMMED;
  } else if (currentStateOrder == 7) {
    return TransceiverStateMachineState::ACTIVE;
  } else if (currentStateOrder == 8) {
    return TransceiverStateMachineState::INACTIVE;
  }
  // TODO(joseph5wu) Need to support other states
  throw FbossError(
      "Unsupported TransceiverStateMachineState order: ", currentStateOrder);
}
} // namespace facebook::fboss
