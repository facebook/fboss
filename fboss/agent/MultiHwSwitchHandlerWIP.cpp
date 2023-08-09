// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/MultiHwSwitchHandlerWIP.h"
#include "fboss/agent/HwSwitchHandlerWIP.h"

namespace facebook::fboss {

MultiHwSwitchHandlerWIP::MultiHwSwitchHandlerWIP(
    HwSwitchHandlerDeprecated* hwSwitchHandler,
    const std::map<SwitchID, cfg::SwitchInfo>& switchInfoMap,
    HwSwitchHandlerInitFn /*hwSwitchHandlerInitFn*/) {
  for (auto entry : switchInfoMap) {
    hwSwitchSyncers_.emplace(
        entry.first,
        std::make_unique<HwSwitchHandlerWIP>(
            hwSwitchHandler, entry.first, entry.second));
  }
}

MultiHwSwitchHandlerWIP::~MultiHwSwitchHandlerWIP() {
  stop();
}

void MultiHwSwitchHandlerWIP::start() {
  if (!stopped_.load()) {
    return;
  }
  for (auto& entry : hwSwitchSyncers_) {
    entry.second->start();
  }
  stopped_.store(false);
}

void MultiHwSwitchHandlerWIP::stop() {
  if (stopped_.load()) {
    return;
  }
  for (auto& entry : hwSwitchSyncers_) {
    entry.second.reset();
  }
  stopped_.store(true);
}

std::shared_ptr<SwitchState> MultiHwSwitchHandlerWIP::stateChanged(
    const StateDelta& delta,
    bool transaction) {
  if (stopped_.load()) {
    throw FbossError("multi hw switch syncer not started");
  }
  // TODO: support more than one switches
  CHECK_EQ(hwSwitchSyncers_.size(), 1);
  auto iter = hwSwitchSyncers_.begin();
  auto switchId = iter->first;
  auto update = HwSwitchStateUpdate(delta, transaction);
  auto future = stateChanged(switchId, update);
  return getStateUpdateResult(switchId, std::move(future));
}

folly::Future<std::shared_ptr<SwitchState>>
MultiHwSwitchHandlerWIP::stateChanged(
    SwitchID switchId,
    const HwSwitchStateUpdate& update) {
  auto iter = hwSwitchSyncers_.find(switchId);
  if (iter == hwSwitchSyncers_.end()) {
    throw FbossError("hw switch syncer for switch id ", switchId, " not found");
  }
  return iter->second->stateChanged(update);
}

std::shared_ptr<SwitchState> MultiHwSwitchHandlerWIP::getStateUpdateResult(
    SwitchID switchId,
    folly::Future<std::shared_ptr<SwitchState>>&& future) {
  auto result = std::move(future).getTry();
  if (result.hasException()) {
    XLOG(ERR) << "Failed to get state update result for switch id " << switchId
              << ":" << result.exception().what();
    result.exception().throw_exception();
  }
  CHECK(result.hasValue());
  return result.value();
}
} // namespace facebook::fboss
