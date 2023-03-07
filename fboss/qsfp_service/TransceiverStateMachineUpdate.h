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
#include "fboss/qsfp_service/TransceiverStateMachine.h"

#include <folly/IntrusiveList.h>
#include <memory>

namespace facebook::fboss {

/*
 * TransceiverStateMachineUpdate objects are used to make changes to the
 * transceiver state machine which will eventually change the iphy/xphy ports
 * and transceiver. Similar to fboss/agent/state/StateUpdate.h.
 */
class TransceiverStateMachineUpdate {
 public:
  explicit TransceiverStateMachineUpdate(
      TransceiverID id,
      TransceiverStateMachineEvent event);
  virtual ~TransceiverStateMachineUpdate() {}

  TransceiverID getTransceiverID() const {
    return id_;
  }
  TransceiverStateMachineEvent getEvent() const {
    return event_;
  }
  std::string getName() const {
    return name_;
  }
  static std::string getEventName(TransceiverStateMachineEvent event);

  virtual void applyUpdate(state_machine<TransceiverStateMachine>& curState);

  virtual void onError(const std::exception& ex) noexcept;

  virtual void onSuccess();

 private:
  // Forbidden copy constructor and assignment operator
  TransceiverStateMachineUpdate(TransceiverStateMachineUpdate const&) = delete;
  TransceiverStateMachineUpdate& operator=(
      TransceiverStateMachineUpdate const&) = delete;

  const TransceiverID id_;
  const TransceiverStateMachineEvent event_;
  const std::string name_;

  // An intrusive list hook for maintaining the list of pending updates.
  folly::IntrusiveListHook listHook_;

  // Keep track of previous and new state so that we can use it to tell whether
  // there is state changed after the update
  std::optional<TransceiverStateMachineState> preState_;
  std::optional<TransceiverStateMachineState> newState_;

  // TransceiverManager code needs access to our listHook_ member so it can
  // maintain the update list.
  friend class TransceiverManager;
};

/*
 * A helper class for synchronizing on the result of a blocking update.
 * A pretty similar class of BlockingUpdateResult from fboss/agent/state
 *
 * This is very similar to std::future/std::promise, but is lower overhead as
 * we know the lifetime and don't need to allocate a separate object for the
 * shared state.
 */
class BlockingTransceiverStateMachineUpdateResult {
 public:
  BlockingTransceiverStateMachineUpdateResult() = default;

  void wait();

  void signalSuccess();

  void signalError(const std::exception_ptr& ex) noexcept;

 private:
  // Forbidden copy constructor and assignment operator
  BlockingTransceiverStateMachineUpdateResult(
      BlockingTransceiverStateMachineUpdateResult const&) = delete;
  BlockingTransceiverStateMachineUpdateResult& operator=(
      BlockingTransceiverStateMachineUpdateResult const&) = delete;

  std::mutex lock_;
  std::condition_variable cond_;
  std::exception_ptr error_;
  bool done_{false};
};

class BlockingTransceiverStateMachineUpdate
    : public TransceiverStateMachineUpdate {
 public:
  BlockingTransceiverStateMachineUpdate(
      TransceiverID id,
      TransceiverStateMachineEvent event,
      std::shared_ptr<BlockingTransceiverStateMachineUpdateResult> result)
      : TransceiverStateMachineUpdate(id, event), result_(result) {}

  void onSuccess() override;

  void onError(const std::exception& ex) noexcept override;

 private:
  // Forbidden copy constructor and assignment operator
  BlockingTransceiverStateMachineUpdate(
      BlockingTransceiverStateMachineUpdate const&) = delete;
  BlockingTransceiverStateMachineUpdate& operator=(
      BlockingTransceiverStateMachineUpdate const&) = delete;

  std::shared_ptr<BlockingTransceiverStateMachineUpdateResult> result_;
};
} // namespace facebook::fboss
