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
      isTransaction(transaction) {
  if (FLAGS_enable_state_oper_delta) {
    inDelta = delta.getOperDelta();
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
  hwSwitchManagerEvb_.runInEventBaseThreadAndWait(
      [this]() { hwSwitchManagerEvb_.terminateLoopSoon(); });
  hwSwitchManagerThread_->join();
  hwSwitchManagerThread_.reset();
}

HwSwitchHandler::~HwSwitchHandler() {
  stop();
}

folly::Future<std::shared_ptr<SwitchState>> HwSwitchHandler::stateChanged(
    HwSwitchStateUpdate update) {
  auto [promise, semiFuture] =
      folly::makePromiseContract<std::shared_ptr<SwitchState>>();

  hwSwitchManagerEvb_.runInEventBaseThread([promise = std::move(promise),
                                            update = std::move(update),
                                            this]() mutable {
    promise.setWith([update, this]() { return stateChangedImpl(update); });
  });

  auto future = std::move(semiFuture).via(&hwSwitchManagerEvb_);
  return future;
}

std::shared_ptr<SwitchState> HwSwitchHandler::stateChangedImpl(
    const HwSwitchStateUpdate& update) {
  if (!FLAGS_enable_state_oper_delta) {
    StateDelta stateDelta(update.oldState, update.newState);
    return stateChanged(stateDelta, update.isTransaction);
  }
  // filter out deltas that don't apply to this switch
  auto inDelta = operDeltaFilter_.filter(update.inDelta, 1);
  if (!inDelta) {
    // no-op
    return update.newState;
  }
  auto outDelta = stateChangedImpl(*inDelta, update.isTransaction);
  if (outDelta.changes()->empty()) {
    return update.newState;
  }
  if (*inDelta == outDelta) {
    return update.oldState;
  }
  // obtain the state that actually got programmed
  return StateDelta(update.newState, outDelta).newState();
}

fsdb::OperDelta HwSwitchHandler::stateChangedImpl(
    const fsdb::OperDelta& delta,
    bool transaction) {
  return stateChanged(delta, transaction);
}

} // namespace facebook::fboss
