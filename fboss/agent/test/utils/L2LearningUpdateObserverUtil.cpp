// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/test/utils/L2LearningUpdateObserverUtil.h"

#include "fboss/agent/MacTableUtils.h"

#include <chrono>

namespace facebook::fboss {

void L2LearningUpdateObserverUtil::startObserving(AgentEnsemble* ensemble) {
  std::lock_guard<std::mutex> lock(mtx_);
  ensemble_ = ensemble;
  ensemble_->getSw()->getL2LearnEventObservers()->registerL2LearnEventObserver(
      this, "L2LearningUpdateObserverUtil");
}

void L2LearningUpdateObserverUtil::stopObserving() {
  std::lock_guard<std::mutex> lock(mtx_);
  ensemble_->getSw()
      ->getL2LearnEventObservers()
      ->unregisterL2LearnEventObserver(this, "L2LearningUpdateObserverUtil");
  ensemble_ = nullptr;
}

void L2LearningUpdateObserverUtil::reset() {
  std::lock_guard<std::mutex> lock(mtx_);
  data_.clear();
}

void L2LearningUpdateObserverUtil::l2LearningUpdateReceived(
    const L2Entry& l2Entry,
    const L2EntryUpdateType& l2EntryUpdateType) noexcept {
  std::lock_guard<std::mutex> lock(mtx_);
  data_.push_back(std::make_pair(l2Entry, l2EntryUpdateType));
  cv_.notify_all();
}

std::vector<std::pair<L2Entry, L2EntryUpdateType>>
L2LearningUpdateObserverUtil::waitForLearningUpdates(
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
} // namespace facebook::fboss
