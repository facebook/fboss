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

#include <folly/SpinLock.h>
#include <folly/Synchronized.h>
#include "fboss/agent/FbossError.h"
#include "fboss/qsfp_service/TypedStateMachineUpdate.h"

#include <boost/msm/back/state_machine.hpp>
#include <boost/msm/front/euml/euml.hpp>
#include <boost/msm/front/euml/state_grammar.hpp>

namespace facebook::fboss {

using boost::msm::back::state_machine;
using boost::msm::front::euml::init_;

/*
 * StateMachineController is a templated wrapper around a state machine and a
 * list of state machine updates. This class provides an interface to enqueue
 * and execute updates associated with this specific state machine as well.
 */

template <
    typename IdType,
    typename EventType,
    typename StateType,
    typename StateMachineType>
class StateMachineController {
  using StateMachineUpdate = TypedStateMachineUpdate<EventType>;

 public:
  explicit StateMachineController(IdType idVal) : id_(idVal) {
    setStateMachineAttributes();
  }

  folly::Synchronized<state_machine<StateMachineType>>& getStateMachine() {
    // Grant managers access to the state machine for configuring
    // manager-specific attributes.
    return stateMachine_;
  }

  void blockNewUpdates() {
    // Lock acquisition is needed here to ensure updates cannot be queued after
    // blocking new updates.
    pendingUpdates_.withWLock(
        [&](auto& /* pendingUpdatesList */) { blockNewUpdates_ = true; });
  }

  void enqueueUpdate(std::unique_ptr<StateMachineUpdate> update) {
    if (update == nullptr) {
      XLOG(ERR)
          << "[SM] Enqueued an empty state machine update to State Machine: "
          << static_cast<int32_t>(id_);
    }
    pendingUpdates_.withWLock([&](auto& pendingUpdatesList) {
      if (blockNewUpdates_) {
        XLOG(ERR) << "[SM] Ignoring state machine update for: "
                  << getUpdateString(update->getEvent())
                  << ". State machine controller is in shutdown mode.";
        return;
      }
      pendingUpdatesList.push_back(std::move(update));
    });
  }

  void executeSingleUpdate() {
    std::unique_ptr<StateMachineUpdate> update;
    {
      auto pendingUpdatesLocked = pendingUpdates_.wlock();
      if (pendingUpdatesLocked->empty()) {
        return;
      }
      update = std::move(pendingUpdatesLocked->front());
      pendingUpdatesLocked->pop_front();
    }

    if (update == nullptr) {
      XLOG(ERR) << "[SM] State machine update is empty for State Machine: "
                << static_cast<int32_t>(id_);
      return;
    }

    try {
      applyUpdate(update->getEvent());
    } catch (const std::exception& ex) {
      XLOG(ERR) << "Exception while applying update: " << ex.what();
    }

    update->onCompletion();
  }

  StateType getCurrentState() {
    auto lockedStateMachine = stateMachine_.rlock();
    auto curStateOrder = *lockedStateMachine->current_state();
    auto curState = getStateByOrder(curStateOrder);

    logCurrentState(curStateOrder, curState);
    return curState;
  }

  bool arePendingUpdates() {
    return !pendingUpdates_.rlock()->empty();
  }

 private:
  StateType getStateByOrder(int currentStateOrder) const;
  std::string getUpdateString(EventType event) const;
  void logCurrentState(int curStateOrder, StateType state) const;
  void applyUpdate(EventType event);
  void setStateMachineAttributes();

  bool blockNewUpdates_{false};
  IdType id_;
  folly::Synchronized<state_machine<StateMachineType>> stateMachine_;
  folly::Synchronized<std::list<std::unique_ptr<StateMachineUpdate>>>
      pendingUpdates_;
};
} // namespace facebook::fboss
