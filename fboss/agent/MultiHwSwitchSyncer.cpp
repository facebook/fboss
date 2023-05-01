// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/MultiHwSwitchSyncer.h"
#include "fboss/agent/HwSwitchSyncer.h"

namespace facebook::fboss {

MultiHwSwitchSyncer::MultiHwSwitchSyncer(
    HwSwitch* hwSwitch,
    const std::map<SwitchID, cfg::SwitchInfo>& switchInfoMap) {
  for (auto entry : switchInfoMap) {
    hwSwitchSyncers_.emplace(
        entry.first,
        std::make_unique<HwSwitchSyncer>(hwSwitch, entry.first, entry.second));
  }
}

MultiHwSwitchSyncer::~MultiHwSwitchSyncer() {
  stop();
}

void MultiHwSwitchSyncer::start() {
  if (!stopped_.load()) {
    return;
  }
  for (auto& entry : hwSwitchSyncers_) {
    entry.second->start();
  }
  stopped_.store(false);
}

void MultiHwSwitchSyncer::stop() {
  if (stopped_.load()) {
    return;
  }
  for (auto& entry : hwSwitchSyncers_) {
    entry.second.reset();
  }
  stopped_.store(true);
}

std::shared_ptr<SwitchState> MultiHwSwitchSyncer::stateChanged(
    const StateDelta& delta,
    bool transaction) {
  if (stopped_.load()) {
    throw FbossError("multi hw switch syncer not started");
  }
  // TODO: support more than one switches
  CHECK_EQ(hwSwitchSyncers_.size(), 1);
  auto iter = hwSwitchSyncers_.begin();
  return iter->second->stateChanged(delta, transaction);
}
} // namespace facebook::fboss
