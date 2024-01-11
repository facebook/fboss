// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/HwSwitchHandler.h"
#include "fboss/agent/Utils.h"
#include "fboss/agent/state/StateDelta.h"
#include "folly/futures/Promise.h"

namespace facebook::fboss {

HwSwitchStateUpdate::HwSwitchStateUpdate(
    const StateDelta& delta,
    bool transaction)
    : oldState(delta.oldState()),
      newState(delta.newState()),
      inDelta(delta.getOperDelta()),
      isTransaction(transaction) {}

HwSwitchHandler::HwSwitchHandler(
    const SwitchID& switchId,
    const cfg::SwitchInfo& info)
    : switchId_(switchId), info_(info), operDeltaFilter_(switchId) {}

void HwSwitchHandler::start() {
  hwSwitchManagerThread_.reset(new std::thread([this]() { run(); }));
}

void HwSwitchHandler::run() {
  initThread(folly::to<std::string>("HwSwitchHandler-", switchId_));
  hwSwitchManagerEvb_.loopForever();
}

void HwSwitchHandler::stop() {
  if (!hwSwitchManagerThread_) {
    return;
  }
  cancelOperDeltaSync();
  hwSwitchManagerEvb_.runInEventBaseThreadAndWait(
      [this]() { hwSwitchManagerEvb_.terminateLoopSoon(); });
  hwSwitchManagerThread_->join();
  hwSwitchManagerThread_.reset();
}

HwSwitchHandler::~HwSwitchHandler() {
  stop();
}

folly::Future<HwSwitchStateUpdateResult> HwSwitchHandler::stateChanged(
    HwSwitchStateUpdate update) {
  auto [promise, semiFuture] =
      folly::makePromiseContract<HwSwitchStateUpdateResult>();

  hwSwitchManagerEvb_.runInEventBaseThread([promise = std::move(promise),
                                            update = std::move(update),
                                            this]() mutable {
    promise.setWith([update, this]() { return stateChangedImpl(update); });
  });

  auto future = std::move(semiFuture).via(&hwSwitchManagerEvb_);
  return future;
}

HwSwitchStateUpdateResult HwSwitchHandler::stateChangedImpl(
    const HwSwitchStateUpdate& update) {
  auto operDeltaSyncState = getHwSwitchOperDeltaSyncState();

  if (operDeltaSyncState == HwSwitchOperDeltaSyncState::DISCONNECTED ||
      operDeltaSyncState == HwSwitchOperDeltaSyncState::CANCELLED) {
    return {
        update.oldState,
        HwSwitchStateUpdateStatus::HWSWITCH_STATE_UPDATE_CANCELLED};
  }

  std::optional<fsdb::OperDelta> inDelta;
  if (operDeltaSyncState == HwSwitchOperDeltaSyncState::WAITING_INITIAL_SYNC) {
    auto initialUpdate = HwSwitchStateUpdate(
        StateDelta(std::make_shared<SwitchState>(), update.newState),
        update.isTransaction);
    // filter out deltas that don't apply to this switch from initial update
    inDelta = operDeltaFilter_.filter(initialUpdate.inDelta, 1);
  } else {
    // filter out deltas that don't apply to this switch from incremental update
    inDelta = operDeltaFilter_.filter(update.inDelta, 1);
  }

  if (!inDelta) {
    // no-op
    return {
        update.newState,
        HwSwitchStateUpdateStatus::HWSWITCH_STATE_UPDATE_SUCCEEDED};
  }
  auto stateUpdateResult = stateChangedImpl(*inDelta, update.isTransaction);
  auto outDelta = stateUpdateResult.first;
  if (outDelta.changes()->empty()) {
    return {update.newState, stateUpdateResult.second};
  }
  if (*inDelta == outDelta) {
    return {update.oldState, stateUpdateResult.second};
  }
  // obtain the state that actually got programmed
  return {
      StateDelta(update.newState, outDelta).newState(),
      stateUpdateResult.second};
}

HwSwitchStateOperUpdateResult HwSwitchHandler::stateChangedImpl(
    const fsdb::OperDelta& delta,
    bool transaction) {
  return stateChanged(delta, transaction);
}

} // namespace facebook::fboss
