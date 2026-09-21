/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/qsfp_service/PortStateMachine.h"

namespace facebook::fboss {

PortStateMachineState getPortStateByOrder(int currentStateOrder) {
  if (currentStateOrder == 0) {
    return PortStateMachineState::UNINITIALIZED;
  } else if (currentStateOrder == 1) {
    return PortStateMachineState::INITIALIZED;
  } else if (currentStateOrder == 2) {
    return PortStateMachineState::IPHY_PORTS_PROGRAMMED;
  } else if (currentStateOrder == 3) {
    return PortStateMachineState::XPHY_PORTS_PROGRAMMED;
  } else if (currentStateOrder == 4) {
    return PortStateMachineState::TRANSCEIVERS_PROGRAMMED;
  } else if (currentStateOrder == 5) {
    return PortStateMachineState::PORT_UP;
  } else if (currentStateOrder == 6) {
    return PortStateMachineState::PORT_DOWN;
  }

  throw FbossError(
      "Unsupported PortStateMachineState order: ", currentStateOrder);
}
} // namespace facebook::fboss
