/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/qsfp_service/module/ModuleStateMachineUpdate.h"

#include "fboss/agent/FbossError.h"

namespace facebook::fboss {
void ModuleStateMachineUpdate::applyUpdate(
    msm::back::state_machine<NewModuleStateMachine>* curState) {
  switch (moduleEvent_) {
    case ModuleStateMachineEvent::OPTICS_DETECTED:
      curState->process_event(NEW_MODULE_EVENT_OPTICS_DETECTED);
      break;
    default:
      throw FbossError(
          "Unsupported ModuleStateMachineEvent: ",
          getModuleStateMachineEventName(moduleEvent_));
  }
}

} // namespace facebook::fboss
