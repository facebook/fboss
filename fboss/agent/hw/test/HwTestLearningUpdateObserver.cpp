// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/test/HwTestLearningUpdateObserver.h"

#include "fboss/agent/MacTableUtils.h"

#include <chrono>

namespace facebook::fboss {

void HwTestLearningUpdateObserver::startObserving(HwSwitchEnsemble* ensemble) {
  std::lock_guard<std::mutex> lock(mtx_);
  ensemble_ = ensemble;

  applyStateUpdateThread_ = std::make_unique<std::thread>([=, this]() {
    initThread("apply state");
    applyStateUpdateEventBase_.loopForever();
  });

  ensemble_->addHwEventObserver(this);
}

void HwTestLearningUpdateObserver::stopObserving() {
  std::lock_guard<std::mutex> lock(mtx_);
  HwSwitchEnsemble::HwSwitchEventObserverIf::stopObserving();

  if (applyStateUpdateThread_) {
    applyStateUpdateEventBase_.runInFbossEventBaseThreadAndWait(
        [this] { applyStateUpdateEventBase_.terminateLoopSoon(); });
    applyStateUpdateThread_->join();
    applyStateUpdateThread_.reset();
  }
  ensemble_ = nullptr;
}

void HwTestLearningUpdateObserver::reset() {
  std::lock_guard<std::mutex> lock(mtx_);
  data_.clear();
}

void HwTestLearningUpdateObserver::l2LearningUpdateReceived(
    L2Entry l2Entry,
    L2EntryUpdateType l2EntryUpdateType) {
  std::lock_guard<std::mutex> lock(mtx_);

  /*
   * The state delta processing of modified L2Learning mode causes the BCM
   * layer to traverse L2 table and generate callbacks. Thus, this code may
   * execute in the context of state delta processing. We cannot schedule
   * another state update or else we will try to acquire same lock and hang.
   * To avoid, schedule state delta processing in a different context.
   */
  applyStateUpdateEventBase_.runInFbossEventBaseThread(
      [this, l2Entry, l2EntryUpdateType]() {
        this->applyStateUpdateHelper(l2Entry, l2EntryUpdateType);
      });

  data_.emplace_back(l2Entry, l2EntryUpdateType);

  cv_.notify_all();
}

void HwTestLearningUpdateObserver::applyStateUpdateHelper(
    L2Entry l2Entry,
    L2EntryUpdateType l2EntryUpdateType) {
  CHECK(applyStateUpdateEventBase_.inRunningEventBaseThread());

  auto state1 = ensemble_->getProgrammedState();
  auto state2 =
      MacTableUtils::updateMacTable(state1, l2Entry, l2EntryUpdateType);
  ensemble_->applyNewState(state2);
}

std::vector<std::pair<L2Entry, L2EntryUpdateType>>
HwTestLearningUpdateObserver::waitForLearningUpdates(
    int numUpdates,
    std::optional<int> secondsToWait) {
  std::unique_lock<std::mutex> lock(mtx_);
  // wait for at least numUpdates, call reset before waiting for accurate
  // waiting
  auto predicate = [this, numUpdates] { return data_.size() >= numUpdates; };
  if (predicate()) {
    return data_;
  }
  if (secondsToWait) {
    cv_.wait_for(lock, std::chrono::seconds(secondsToWait.value()), predicate);
  } else {
    cv_.wait(lock, predicate);
  }
  return data_;
}

void HwTestLearningUpdateObserver::waitForStateUpdate() {
  applyStateUpdateEventBase_.runInFbossEventBaseThreadAndWait([]() { return; });
}

HwTestLearningUpdateAutoObserver::HwTestLearningUpdateAutoObserver(
    HwSwitchEnsemble* ensemble)
    : ensemble_(ensemble) {
  reset();
  startObserving(ensemble_);
}

HwTestLearningUpdateAutoObserver::~HwTestLearningUpdateAutoObserver() {
  stopObserving();
  ensemble_->removeHwEventObserver(this);
}

} // namespace facebook::fboss
