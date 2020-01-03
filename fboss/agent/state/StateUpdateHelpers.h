/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
//
// This file contains helper implementations of StateUpdate.
#pragma once

#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>

#include <folly/Range.h>
#include <folly/String.h>
#include <folly/logging/xlog.h>
#include "fboss/agent/state/StateUpdate.h"

namespace facebook::fboss {

class FunctionStateUpdate : public StateUpdate {
 public:
  typedef std::function<std::shared_ptr<SwitchState>(
      const std::shared_ptr<SwitchState>&)>
      StateUpdateFn;

  FunctionStateUpdate(
      folly::StringPiece name,
      StateUpdateFn fn,
      bool allowCoalesce = true)
      : StateUpdate(name, allowCoalesce), function_(fn) {}

  std::shared_ptr<SwitchState> applyUpdate(
      const std::shared_ptr<SwitchState>& origState) override {
    return function_(origState);
  }

  void onError(const std::exception& ex) noexcept override {
    XLOG(FATAL) << "unexpected error applying state update <" << getName()
                << ">: " << folly::exceptionStr(ex);
  }

 private:
  StateUpdateFn function_;
};

/*
 * A helper class for synchronizing on the result of a blocking update.
 *
 * This is very similar to std::future/std::promise, but is lower overhead as
 * we know the lifetime and don't need to allocate a separate object for the
 * shared state.
 */
class BlockingUpdateResult {
 public:
  void wait() {
    std::unique_lock<std::mutex> guard(lock_);
    while (!done_) {
      cond_.wait(guard);
    }
    if (error_) {
      std::rethrow_exception(error_);
    }
  }

  void signalSuccess() {
    {
      std::lock_guard<std::mutex> g(lock_);
      CHECK(!done_);
      done_ = true;
    }
    cond_.notify_one();
  }

  void signalError(const std::exception_ptr& ex) noexcept {
    {
      std::lock_guard<std::mutex> g(lock_);
      CHECK(!done_);
      error_ = ex;
      done_ = true;
    }
    cond_.notify_one();
  }

 private:
  std::mutex lock_;
  std::condition_variable cond_;
  std::exception_ptr error_;
  bool done_{false};
};

class BlockingStateUpdate : public StateUpdate {
 public:
  typedef std::function<std::shared_ptr<SwitchState>(
      const std::shared_ptr<SwitchState>&)>
      StateUpdateFn;

  BlockingStateUpdate(
      folly::StringPiece name,
      StateUpdateFn fn,
      std::shared_ptr<BlockingUpdateResult> result,
      bool allowCoalesce = true)
      : StateUpdate(name, allowCoalesce), function_(fn), result_(result) {}

  std::shared_ptr<SwitchState> applyUpdate(
      const std::shared_ptr<SwitchState>& origState) override {
    return function_(origState);
  }

  void onError(const std::exception& /*ex*/) noexcept override {
    // Note that we need to use std::current_exception() here rather than
    // std::make_exception_ptr() -- make_exception_ptr will lose the original
    // exception type information.  SwSwitch guarantees that the exception
    // passed in will still be available as std::current_exception() when
    // onError() is called.
    result_->signalError(std::current_exception());
  }

  void onSuccess() override {
    result_->signalSuccess();
  }

 private:
  StateUpdateFn function_;
  std::shared_ptr<BlockingUpdateResult> result_;
};

} // namespace facebook::fboss
