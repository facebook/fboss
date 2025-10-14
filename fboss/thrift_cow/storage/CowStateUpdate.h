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

#include <chrono>
#include <memory>

#include <fboss/thrift_cow/storage/CowStorage.h>
#include <folly/FBString.h>
#include <folly/IntrusiveList.h>

namespace facebook::fboss::fsdb {
template <typename Root>
class CowStorageMgr;
/*
 * CowStateUpdate objects are used to make changes to the CowState.
 *
 * All updates are applied in a single thread.  CowStateUpdate objects allow
 * other threads to instruct the update thread how to apply a particular update.
 *
 * Note that the update thread may choose to batch updates in some cases--if it
 * has multiple updates to apply it may run them all at once and only send a
 * single update notification  to  update subscribers.
 * Therefore the applyUpdate() may be called with an unpublished CowState in
 * some cases.
 */
template <typename Root>
class CowStateUpdate {
 public:
  enum class BehaviorFlags : int {
    NONE = 0x0,
    NON_COALESCING = 0x1,
  };
  static constexpr int kDefaultBehaviorFlags =
      static_cast<int>(BehaviorFlags::NONE);
  using CowState = typename CowStorage<Root>::StorageImpl;
  using CowStateUpdateFn = std::function<std::shared_ptr<CowState>(
      const std::shared_ptr<CowState>&)>;

  explicit CowStateUpdate(folly::StringPiece name, int behaviorFlags)
      : name_(name.str()),
        behaviorFlags_(behaviorFlags),
        queuedTimestamp_(std::chrono::steady_clock::now()) {}
  virtual ~CowStateUpdate() = default;

  const std::string& getName() const {
    return name_;
  }

  std::chrono::steady_clock::time_point getQueuedTimestamp() const {
    return queuedTimestamp_;
  }

  void setPrintUpdateDelay(bool printUpdateDelay) {
    printUpdateDelay_ = printUpdateDelay;
  }

  bool shouldPrintUpdateDelay() const {
    return printUpdateDelay_;
  }

  bool allowsCoalescing() const {
    return !isNonCoalescing();
  }
  bool isNonCoalescing() const {
    return behaviorFlags_ & static_cast<int>(BehaviorFlags::NON_COALESCING);
  }

  /*
   * Apply the update, and return a new CowState.
   *
   * This function should return null if the update does not result in any
   * changes to the state.  (This may occur in cases where the update would
   * have caused changes when it was first scheduled, but no longer results in
   * changes by the time it is actually applied.)
   */
  virtual std::shared_ptr<CowState> applyUpdate(
      const std::shared_ptr<CowState>& origState) = 0;

  /*
   * The onError() function will be called in the update thread if an error
   * occurs applying the state update.
   *
   * In case the callback wishes to save the current exception as a
   * std::exception_ptr, the exception will also be available via
   * std::current_exception() when onError() is called.
   */
  virtual void onError(const std::exception& ex) noexcept = 0;

  /*
   * The onSuccess() function will be called in the update thread
   * after the state update has been successfully applied.
   */
  virtual void onSuccess() {}

 private:
  // Forbidden copy constructor and assignment operator
  CowStateUpdate(CowStateUpdate const&) = delete;
  CowStateUpdate& operator=(CowStateUpdate const&) = delete;

  std::string name_;
  int behaviorFlags_{static_cast<int>(BehaviorFlags::NONE)};
  std::chrono::steady_clock::time_point queuedTimestamp_;
  bool printUpdateDelay_{false};

  // An intrusive list hook for maintaining the list of pending updates.
  folly::IntrusiveListHook listHook_;
  // The CowStateMgr code needs access to our listHook_ member so it can
  // maintain the update list.
  friend class CowStorageMgr<Root>;
};

template <typename Root>
class FunctionCowStateUpdate : public CowStateUpdate<Root> {
 public:
  using Base = CowStateUpdate<Root>;
  using Base::getName;
  using CowState = typename Base::CowState;
  using CowStateUpdateFn = typename Base::CowStateUpdateFn;

  FunctionCowStateUpdate(
      folly::StringPiece name,
      CowStateUpdateFn fn,
      int flags = Base::kDefaultBehaviorFlags)
      : Base(name, flags), function_(fn) {}

  std::shared_ptr<CowState> applyUpdate(
      const std::shared_ptr<CowState>& origState) override {
    return function_(origState);
  }

  void onError(const std::exception& ex) noexcept override {
    XLOG(FATAL) << "unexpected error applying state update <" << getName()
                << ">: " << folly::exceptionStr(ex);
  }

 private:
  CowStateUpdateFn function_;
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

template <typename Root>
class BlockingCowStateUpdate : public CowStateUpdate<Root> {
 public:
  using Base = CowStateUpdate<Root>;
  using CowState = typename Base::CowState;
  using CowStateUpdateFn = typename Base::CowStateUpdateFn;

  BlockingCowStateUpdate(
      folly::StringPiece name,
      CowStateUpdateFn fn,
      std::shared_ptr<BlockingUpdateResult> result,
      int flags = Base::kDefaultBehaviorFlags)
      : Base(name, flags), function_(fn), result_(result) {}

  std::shared_ptr<CowState> applyUpdate(
      const std::shared_ptr<CowState>& origState) override {
    return function_(origState);
  }

  void onError(const std::exception& /*ex*/) noexcept override {
    // Note that we need to use std::current_exception() here rather than
    // std::make_exception_ptr() -- make_exception_ptr will lose the original
    // exception type information.  Caller must ensure that the exception
    // passed in will still be available as std::current_exception() when
    // onError() is called.
    result_->signalError(std::current_exception());
  }

  void onSuccess() override {
    result_->signalSuccess();
  }

 private:
  CowStateUpdateFn function_;
  std::shared_ptr<BlockingUpdateResult> result_;
};

} // namespace facebook::fboss::fsdb
