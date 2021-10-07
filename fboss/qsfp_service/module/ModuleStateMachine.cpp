/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/qsfp_service/module/ModuleStateMachine.h"

#include "fboss/agent/FbossError.h"

DEFINE_bool(use_new_state_machine, false, "Use the new state machine logic");

namespace facebook::fboss {

std::string getModuleStateMachineEventName(ModuleStateMachineEvent event) {
  switch (event) {
    case ModuleStateMachineEvent::OPTICS_DETECTED:
      return "OPTICS_DETECTED";
    case ModuleStateMachineEvent::OPTICS_REMOVED:
      return "OPTICS_REMOVED";
    case ModuleStateMachineEvent::OPTICS_RESET:
      return "OPTICS_RESET";
    case ModuleStateMachineEvent::EEPROM_READ:
      return "EEPROM_READ";
    case ModuleStateMachineEvent::ALL_PORTS_DOWN:
      return "ALL_PORTS_DOWN";
    case ModuleStateMachineEvent::PORT_UP:
      return "PORT_UP";
    case ModuleStateMachineEvent::TRIGGER_UPGRADE:
      return "TRIGGER_UPGRADE";
    case ModuleStateMachineEvent::FORCED_UPGRADE:
      return "FORCED_UPGRADE";
    case ModuleStateMachineEvent::AGENT_SYNC_TIMEOUT:
      return "AGENT_SYNC_TIMEOUT";
    case ModuleStateMachineEvent::BRINGUP_DONE:
      return "BRINGUP_DONE";
    case ModuleStateMachineEvent::REMEDIATE_DONE:
      return "REMEDIATE_DONE";
  }
  throw FbossError("Unsupported ModuleStateMachineEvent");
}
} // namespace facebook::fboss
