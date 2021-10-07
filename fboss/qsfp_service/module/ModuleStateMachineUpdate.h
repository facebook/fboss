/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#pragma once

#include "fboss/agent/types.h"
#include "fboss/qsfp_service/module/ModuleStateMachine.h"

#include <folly/IntrusiveList.h>

namespace facebook::fboss {

/*
 * ModuleStateMachineUpdate objects are used to make changes to the module
 * state machine which will eventually change the iphy/xphy ports and
 * transceiver. Similar to fboss/agent/state/StateUpdate.h.
 */
class ModuleStateMachineUpdate {
 public:
  explicit ModuleStateMachineUpdate(
      TransceiverID id,
      ModuleStateMachineEvent moduleEvent)
      : id_(id), moduleEvent_(moduleEvent) {}

  TransceiverID getTransceiverID() const {
    return id_;
  }

  void applyUpdate(msm::back::state_machine<NewModuleStateMachine>* curState);

 private:
  // Forbidden copy constructor and assignment operator
  ModuleStateMachineUpdate(ModuleStateMachineUpdate const&) = delete;
  ModuleStateMachineUpdate& operator=(ModuleStateMachineUpdate const&) = delete;

  const TransceiverID id_;
  const ModuleStateMachineEvent moduleEvent_;

  // An intrusive list hook for maintaining the list of pending updates.
  folly::IntrusiveListHook listHook_;
  // TransceiverManager code needs access to our listHook_ member so it can
  // maintain the update list.
  friend class TransceiverManager;
};
} // namespace facebook::fboss
