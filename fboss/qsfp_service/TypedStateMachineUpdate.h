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

#include <fmt/format.h>

#include <folly/logging/xlog.h>
#include <memory>

namespace facebook::fboss {

/*
 * TypedStateMachineUpdates are simple wrappers around StateMachineEvent enums
 * that enable functionality for performing certain actions on completion of
 * state update applied.
 */

template <typename EventType>
class TypedStateMachineUpdate {
 public:
  explicit TypedStateMachineUpdate(EventType event) : event_(event) {}
  virtual ~TypedStateMachineUpdate() = default;
  TypedStateMachineUpdate(TypedStateMachineUpdate&& update) = default;
  TypedStateMachineUpdate& operator=(TypedStateMachineUpdate&& update) =
      default;

  virtual void onCompletion() {}
  EventType getEvent() const {
    return event_;
  }

 private:
  // Forbidden copy constructor and assignment operator
  TypedStateMachineUpdate(TypedStateMachineUpdate const&) = delete;
  TypedStateMachineUpdate& operator=(TypedStateMachineUpdate const&) = delete;

  const EventType event_;
};

/*
 * A helper class for synchronizing on the result of a blocking update.
 * A pretty similar class of BlockingUpdateResult from fboss/agent/state
 *
 * This is very similar to std::future/std::promise, but is lower overhead as
 * we know the lifetime and don't need to allocate a separate object for the
 * shared state.
 */

class BlockingStateMachineUpdateResult {
 public:
  BlockingStateMachineUpdateResult() = default;
  ~BlockingStateMachineUpdateResult() = default;
  BlockingStateMachineUpdateResult(BlockingStateMachineUpdateResult&& result) =
      delete;
  BlockingStateMachineUpdateResult& operator=(
      BlockingStateMachineUpdateResult&& result) = delete;

  void wait() {
    std::unique_lock<std::mutex> guard(lock_);
    cond_.wait(guard, [this] { return done_; });
  }

  void signalCompletion() {
    {
      std::unique_lock<std::mutex> guard(lock_);
      CHECK(!done_);
      done_ = true;
    }
    cond_.notify_one();
  }

 private:
  // Forbidden copy constructor and assignment operator
  BlockingStateMachineUpdateResult(BlockingStateMachineUpdateResult const&) =
      delete;
  BlockingStateMachineUpdateResult& operator=(
      BlockingStateMachineUpdateResult const&) = delete;

  std::mutex lock_;
  std::condition_variable cond_;
  bool done_{false};
};

/*
 * A blocking variant of TypedStateMachineUpdate, allowing the caller to wait
 * for the update to complete. It uses a BlockingStateMachineUpdateResult object
 * to synchronize on the result.
 */

template <typename EventType>
class BlockingStateMachineUpdate : public TypedStateMachineUpdate<EventType> {
 public:
  BlockingStateMachineUpdate(
      EventType event,
      std::shared_ptr<BlockingStateMachineUpdateResult> result)
      : TypedStateMachineUpdate<EventType>(event), result_(result) {}
  ~BlockingStateMachineUpdate() override = default;
  BlockingStateMachineUpdate(BlockingStateMachineUpdate&& update) = default;
  BlockingStateMachineUpdate& operator=(BlockingStateMachineUpdate&& update) =
      default;

  void onCompletion() override {
    result_->signalCompletion();
  }

 private:
  // Forbidden copy constructor and assignment operator
  BlockingStateMachineUpdate(BlockingStateMachineUpdate const&) = delete;
  BlockingStateMachineUpdate& operator=(BlockingStateMachineUpdate const&) =
      delete;

  std::shared_ptr<BlockingStateMachineUpdateResult> result_;
};
} // namespace facebook::fboss
