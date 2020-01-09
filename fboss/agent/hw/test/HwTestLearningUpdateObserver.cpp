// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/test/HwTestLearningUpdateObserver.h"

#include "fboss/agent/MacTableUtils.h"

namespace facebook::fboss {

void HwTestLearningUpdateObserver::startObserving(HwSwitchEnsemble* ensemble) {
  ensemble_ = ensemble;
  ensemble_->addHwEventObserver(this);
}

void HwTestLearningUpdateObserver::stopObserving() {
  if (ensemble_) {
    ensemble_->removeHwEventObserver(this);
    ensemble_ = nullptr;
  }
}

void HwTestLearningUpdateObserver::reset() {
  std::lock_guard<std::mutex> lock(mtx_);
  data_.clear();
}

void HwTestLearningUpdateObserver::l2LearningUpdateReceived(
    L2Entry l2Entry,
    L2EntryUpdateType l2EntryUpdateType) {
  std::lock_guard<std::mutex> lock(mtx_);

  auto state1 = ensemble_->getProgrammedState();
  auto state2 =
      MacTableUtils::updateMacTable(state1, l2Entry, l2EntryUpdateType);
  ensemble_->applyNewState(state2);

  data_.push_back(std::make_pair(l2Entry, l2EntryUpdateType));

  cv_.notify_all();
}

std::vector<std::pair<L2Entry, L2EntryUpdateType>>
HwTestLearningUpdateObserver::waitForLearningUpdates(int numUpdates) {
  std::unique_lock<std::mutex> lock(mtx_);
  while (data_.size() != numUpdates) {
    cv_.wait(lock);
  }

  return data_;
}

} // namespace facebook::fboss
