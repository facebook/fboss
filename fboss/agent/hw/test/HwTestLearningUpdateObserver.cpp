// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/test/HwTestLearningUpdateObserver.h"

namespace facebook {
namespace fboss {

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
  data_.reset();
}

void HwTestLearningUpdateObserver::l2LearningUpdateReceived(
    L2Entry l2Entry,
    L2EntryUpdateType l2EntryUpdateType) {
  std::lock_guard<std::mutex> lock(mtx_);

  data_ = std::make_unique<std::pair<L2Entry, L2EntryUpdateType>>(
      std::make_pair(l2Entry, l2EntryUpdateType));

  cv_.notify_all();
}

std::pair<L2Entry, L2EntryUpdateType>*
HwTestLearningUpdateObserver::waitForLearningUpdate() {
  std::unique_lock<std::mutex> lock(mtx_);
  while (!data_) {
    cv_.wait(lock);
  }

  return data_.get();
}

} // namespace fboss
} // namespace facebook
