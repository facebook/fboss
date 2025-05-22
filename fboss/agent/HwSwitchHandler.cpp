// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/HwSwitchHandler.h"
#include "fboss/agent/Utils.h"
#include "fboss/agent/state/StateDelta.h"
#include "folly/futures/Promise.h"

namespace facebook::fboss {

HwSwitchStateUpdate::HwSwitchStateUpdate(
    const std::vector<StateDelta>& deltas,
    bool transaction)
    : oldState(deltas.front().oldState()),
      newState(deltas.back().newState()),
      isTransaction(transaction) {
  for (const auto& delta : deltas) {
    operDeltas.push_back(delta.getOperDelta());
  }
}

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
  hwSwitchManagerEvb_.runInFbossEventBaseThreadAndWait(
      [this]() { hwSwitchManagerEvb_.terminateLoopSoon(); });
  hwSwitchManagerThread_->join();
  hwSwitchManagerThread_.reset();
}

HwSwitchHandler::~HwSwitchHandler() {
  stop();
}

folly::Future<HwSwitchStateUpdateResult> HwSwitchHandler::stateChanged(
    HwSwitchStateUpdate update,
    const HwWriteBehavior& hwWriteBehavior) {
  auto [promise, semiFuture] =
      folly::makePromiseContract<HwSwitchStateUpdateResult>();

  hwSwitchManagerEvb_.runInFbossEventBaseThread(
      [promise = std::move(promise),
       update = std::move(update),
       hwWriteBehavior = hwWriteBehavior,
       this]() mutable {
        promise.setWith([update, hwWriteBehavior, this]() {
          return stateChangedImpl(update, hwWriteBehavior);
        });
      });

  auto future = std::move(semiFuture).via(&hwSwitchManagerEvb_);
  return future;
}

HwSwitchStateUpdateResult HwSwitchHandler::stateChangedImpl(
    const HwSwitchStateUpdate& update,
    const HwWriteBehavior& hwWriteBehavior) {
  std::vector<fsdb::OperDelta> inDeltas;
  for (const auto& operDelta : update.operDeltas) {
    auto inDelta = operDeltaFilter_.filterWithSwitchStateRootPath(operDelta);
    if (inDelta.has_value()) {
      inDeltas.push_back(inDelta.value());
    }
  }
  if (inDeltas.size() == 0) {
    // no-op
    return {
        update.newState,
        HwSwitchStateUpdateStatus::HWSWITCH_STATE_UPDATE_SUCCEEDED};
  }
  auto stateUpdateResult = stateChangedImpl(
      inDeltas, update.isTransaction, update.newState, hwWriteBehavior);
  auto outDelta = stateUpdateResult.first;
  if (outDelta.changes()->empty()) {
    return {update.newState, stateUpdateResult.second};
  }
  // outDelta would be combined delta if update fails at first delta
  // return the old state
  auto inDelta = operDeltaFilter_.filterWithSwitchStateRootPath(
      StateDelta(update.oldState, update.newState).getOperDelta());
  if (inDelta && *inDelta == outDelta) {
    return {update.oldState, stateUpdateResult.second};
  }
  // obtain the state that actually got programmed
  return {
      StateDelta(update.newState, outDelta).newState(),
      stateUpdateResult.second};
}

HwSwitchStateOperUpdateResult HwSwitchHandler::stateChangedImpl(
    const std::vector<fsdb::OperDelta>& deltas,
    bool transaction,
    const std::shared_ptr<SwitchState>& newState,
    const HwWriteBehavior& hwWriteBehavior) {
  return stateChanged(deltas, transaction, newState, hwWriteBehavior);
}

fsdb::OperDelta HwSwitchHandler::getFullSyncOperDelta(
    const std::shared_ptr<SwitchState>& state) const {
  auto delta = StateDelta(std::make_shared<SwitchState>(), state);
  auto filteredOper =
      operDeltaFilter_.filterWithSwitchStateRootPath(delta.getOperDelta());
  CHECK(filteredOper.has_value());
  return filteredOper.value();
}

} // namespace facebook::fboss
