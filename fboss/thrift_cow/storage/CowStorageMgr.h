// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#pragma once

#include <fb303/ExportType.h>
#include <fb303/ThreadCachedServiceData.h>
#include <fb303/detail/QuantileStatWrappers.h>
#include <fboss/thrift_cow/storage/CowStateUpdate.h>
#include <fboss/thrift_cow/storage/CowStorage.h>
#include <fmt/format.h>
#include <folly/Synchronized.h>
#include <folly/io/async/ScopedEventBaseThread.h>
#include <folly/logging/xlog.h>
#include <optional>

namespace facebook::fboss::fsdb {

template <typename Root>
class CowStorageMgr {
 public:
  using CowStateUpdateFn = typename CowStateUpdate<Root>::CowStateUpdateFn;
  using CowState = typename CowStateUpdate<Root>::CowState;
  using CowStateUpdateList = folly::
      IntrusiveList<CowStateUpdate<Root>, &CowStateUpdate<Root>::listHook_>;
  using CowStateUpdateCb = std::function<
      void(const std::shared_ptr<CowState>&, const std::shared_ptr<CowState>&)>;

  explicit CowStorageMgr(
      CowStorage<Root>&& storage,
      CowStateUpdateCb cb = [](const auto& /*oldState*/,
                               const auto& /*newState*/) {},
      std::optional<std::string> clientCounterPrefix = std::nullopt)
      : storage_(std::make_unique<CowStorage<Root>>(std::move(storage))),
        stateUpdateCb_(cb),
        clientCounterPrefix_(clientCounterPrefix) {
    initCowStateUpdateCounter();
  }

  // Create blank published state
  explicit CowStorageMgr(
      CowStateUpdateCb cb = [](const auto& /*oldState*/,
                               const auto& /*newState*/) {},
      std::optional<std::string> clientCounterPrefix = std::nullopt)
      : CowStorageMgr(
            CowStorage<Root>(Root()),
            std::move(cb),
            clientCounterPrefix) {
    (*storage_.wlock())->publish();
  }

  uint64_t getPendingUpdatesQueueLength() const {
    return pendingUpdates_.rlock()->size();
  }

  /**
   * Schedule an update to the CowState.
   *
   *  @param  update
   *          The update to be enqueued
   *  @return bool whether the update was queued or not
   * This schedules the specified CowStateUpdate to be invoked in the update
   * thread in order to update the CowState
   *
   */
  void updateState(
      std::unique_ptr<CowStateUpdate<Root>> update,
      bool printUpdateDelay = false) {
    // Set the print update delay flag on the update
    update->setPrintUpdateDelay(printUpdateDelay);

    pendingUpdates_.wlock()->push_back(*update.release());
    XLOG(DBG2) << "[FSDB] # Pending updates "
               << pendingUpdates_.rlock()->size();
    updateEvbThread_.getEventBase()->runInEventBaseThread(
        handlePendingUpdatesHelper, this);
  }
  /**
   * Schedule an update to the CowState.
   *
   * @param name  A name to identify the source of this update.  This is
   *              primarily used for logging and debugging purposes.
   * @param fn    The function that will prepare the new CowState.
   * @return bool whether the update was queued or not
   * The CowStateUpdateFn takes a single argument -- the current CowState
   * object to modify.  It should return a new CowState object, or null if
   * it decides that no update needs to be performed.
   *
   * Note that the update function will not be called immediately--it will be
   * invoked later from the update thread.  Therefore if you supply a lambda
   * with bound arguments, make sure that any bound arguments will still be
   * valid later when the function is invoked.  (e.g., Don't capture local
   * variables from your current call frame by reference.)
   *
   * The CowStateUpdateFn must not throw any exceptions.
   *
   * The update thread may choose to batch updates in some cases--if it has
   * multiple update functions to run it may run them all at once and only
   * send a single update notification to the HwSwitch and other update
   * subscribers.  Therefore the CowStateUpdateFn may be called with an
   * unpublished CowState in some cases.
   */
  void updateState(
      folly::StringPiece name,
      CowStateUpdateFn fn,
      bool printUpdateDelay = false) {
    auto update =
        std::make_unique<FunctionCowStateUpdate<Root>>(name, std::move(fn));
    updateState(std::move(update), printUpdateDelay);
  }
  /**
   * Schedule an update to the CowState.
   *
   * @param name  A name to identify the source of this update.  This is
   *              primarily used for logging and debugging purposes.
   * @param fn    The function that will prepare the new CowState.
   *
   * A version of updateState that prevents the update handling queue from
   * coalescing this update with later updates. This should rarely be used,
   * but can be used when there is an update that MUST be seen by the hw
   * implementation, even if the inverse update is immediately applied.
   */
  void updateStateNoCoalescing(folly::StringPiece name, CowStateUpdateFn fn) {
    auto update = std::make_unique<FunctionCowStateUpdate<Root>>(
        name,
        std::move(fn),
        static_cast<int>(CowStateUpdate<Root>::BehaviorFlags::NON_COALESCING));
    updateState(std::move(update));
  }

  /*
   * A version of updateState() that doesn't return until the update has been
   * applied.
   *
   * This should only be called in situations where it is safe to block the
   * current thread until the operation completes.
   *
   */
  void updateStateBlocking(folly::StringPiece name, CowStateUpdateFn fn) {
    auto result = std::make_shared<BlockingUpdateResult>();
    auto update = std::make_unique<BlockingCowStateUpdate<Root>>(
        name, std::move(fn), result);
    updateState(std::move(update));
    result->wait();
  }
  const std::shared_ptr<CowState> getState() const {
    return (*storage_.rlock())->root();
  }

  folly::EventBase* getEventBase() {
    return updateEvbThread_.getEventBase();
  }

 private:
  void initCowStateUpdateCounter() {
    // Only create the quantile stat if a client counter prefix is provided
    if (clientCounterPrefix_.has_value()) {
      std::string clientCounterName =
          fmt::format("{}.cow_state_update.us", *clientCounterPrefix_);

      quantileStat_.emplace(
          clientCounterName,
          fb303::ExportTypeConsts::kAvg,
          fb303::QuantileConsts::kP95_P99_P100);
    }
  }

  void markCowStateUpdateCounter(int64_t delayUs) {
    /*
     * Counter would have been created only if the clientCounterPrefix is passed
     */
    if (quantileStat_.has_value()) {
      quantileStat_->addValue(delayUs);
    }
  }

  static void handlePendingUpdatesHelper(CowStorageMgr<Root>* mgr) {
    mgr->handlePendingUpdates();
  }
  void handlePendingUpdates() {
    CowStateUpdateList updates;
    // Dequeue all queued updates
    pendingUpdates_.withWLock([&updates](auto& pendingUpdates) {
      auto iter = pendingUpdates.begin();
      while (iter != pendingUpdates.end()) {
        auto* update = &(*iter);
        if (update->isNonCoalescing()) {
          if (iter == pendingUpdates.begin()) {
            // First update is non coalescing, splice it onto the updates list
            // and apply transaction by itself
            ++iter;
            break;
          } else {
            // Splice all updates upto this non coalescing update, we will
            // get the non coalescing update in the next round
            break;
          }
        }
        ++iter;
      }
      updates.splice(
          updates.begin(), pendingUpdates, pendingUpdates.begin(), iter);
    });
    // handlePendingUpdates() is invoked once for each update, but a previous
    // call might have already processed everything.  If we don't have anything
    // to do just return early.
    if (updates.empty()) {
      return;
    }
    auto oldAppliedState = getState();
    auto newDesiredState = oldAppliedState;
    auto iter = updates.begin();
    while (iter != updates.end()) {
      auto* update = &(*iter);
      ++iter;

      std::shared_ptr<CowState> intermediateState;

      try {
        intermediateState = update->applyUpdate(newDesiredState);
      } catch (const std::exception& ex) {
        // Call the update's onError() function, and then immediately delete
        // it (therefore removing it from the intrusive list).  This way we
        // won't call it's onSuccess() function later.
        update->onError(ex);
        delete update;
      }
      // We have applied the update to software switch state, so call success
      // on the update.
      if (intermediateState) {
        // Call publish after applying each StateUpdate.  This guarantees that
        // the next StateUpdate function will have clone the SwitchState before
        // making any changes.  This ensures that if a StateUpdate function
        // ever fails partway through it can't have partially modified our
        // existing state, leaving it in an invalid state.
        intermediateState->publish();
        newDesiredState = intermediateState;
      }

      // Calculate processing delay in microseconds
      auto now = std::chrono::steady_clock::now();
      auto delayUs = std::chrono::duration_cast<std::chrono::microseconds>(
                         now - update->getQueuedTimestamp())
                         .count();

      markCowStateUpdateCounter(delayUs);

      if (update->shouldPrintUpdateDelay()) {
        XLOG(DBG2) << "Applied state update " << update->getName()
                   << " (processing delay: " << delayUs << " μs)";
      } else {
        XLOG(DBG3) << "Applied state update " << update->getName()
                   << " (processing delay: " << delayUs << " μs)";
      }
    }
    // Invoke callback on change
    if (newDesiredState != oldAppliedState) {
      storage_.wlock()->reset(new CowStorage<Root>(newDesiredState));
      stateUpdateCb_(oldAppliedState, newDesiredState);
      newDesiredState.reset();
    }
    // Notify all of the updates of success and delete them.
    while (!updates.empty()) {
      std::unique_ptr<CowStateUpdate<Root>> update(&updates.front());
      updates.pop_front();
      update->onSuccess();
    }
  }
  folly::Synchronized<std::unique_ptr<CowStorage<Root>>> storage_;
  CowStateUpdateCb stateUpdateCb_;
  /*
   * A list of pending state updates to be applied.
   */
  folly::Synchronized<CowStateUpdateList> pendingUpdates_;
  folly::ScopedEventBaseThread updateEvbThread_{"CowStorageMgrUpdateThread"};
  std::optional<fb303::detail::QuantileStatWrapper> quantileStat_;
  std::optional<std::string> clientCounterPrefix_;
};

} // namespace facebook::fboss::fsdb
