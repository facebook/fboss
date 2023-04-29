// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/HwSwitchSyncer.h"

#include "fboss/agent/HwSwitch.h"
#include "fboss/agent/Utils.h"
#include "fboss/agent/state/StateDelta.h"
#include "folly/futures/Future.h"
#include "folly/futures/Promise.h"

namespace facebook::fboss {

HwSwitchSyncer::HwSwitchStateUpdate::HwSwitchStateUpdate(
    const StateDelta& delta,
    bool transaction)
    : oldState(delta.oldState()),
      newState(delta.newState()),
      inDelta(delta.getOperDelta()),
      isTransaction(transaction) {}

HwSwitchSyncer::HwSwitchSyncer(
    HwSwitch* hwSwitch,
    const SwitchID& switchId,
    const cfg::SwitchInfo& info)
    : hwSwitch_(hwSwitch), switchId_(switchId), info_(info) {}

void HwSwitchSyncer::start() {
  hwSwitchManagerThread_.reset(new std::thread([this]() { run(); }));
}

void HwSwitchSyncer::run() {
  initThread(folly::to<std::string>("HwSwitchSyncer-", switchId_));
  hwSwitchManagerEvb_.loopForever();
}

void HwSwitchSyncer::stop() {
  if (!hwSwitchManagerThread_) {
    return;
  }
  hwSwitchManagerEvb_.runInEventBaseThreadAndWait(
      [this]() { hwSwitchManagerEvb_.terminateLoopSoon(); });
  hwSwitchManagerThread_->join();
  hwSwitchManagerThread_.reset();
}

HwSwitchSyncer::~HwSwitchSyncer() {
  stop();
}

std::shared_ptr<SwitchState> HwSwitchSyncer::stateChanged(
    const StateDelta& delta,
    bool transaction) {
  HwSwitchStateUpdate update(delta, transaction);
  auto [promise, semiFuture] =
      folly::makePromiseContract<std::shared_ptr<SwitchState>>();

  hwSwitchManagerEvb_.runInEventBaseThread([promise = std::move(promise),
                                            update = std::move(update),
                                            this]() mutable {
    promise.setWith([update, this]() { return stateChangedImpl(update); });
  });

  auto result = std::move(semiFuture).via(&hwSwitchManagerEvb_).getTry();
  if (result.hasException()) {
    // TODO: handle exception
  }
  return result.value();
}

std::shared_ptr<SwitchState> HwSwitchSyncer::stateChangedImpl(
    const HwSwitchStateUpdate& update) {
  if (!FLAGS_enable_state_oper_delta) {
    StateDelta stateDelta(update.oldState, update.newState);
    return update.isTransaction ? hwSwitch_->stateChangedTransaction(stateDelta)
                                : hwSwitch_->stateChanged(stateDelta);
  }
  auto outDelta = stateChangedImpl(update.inDelta, update.isTransaction);
  if (outDelta.changes()->empty()) {
    return update.newState;
  }
  if (update.inDelta == outDelta) {
    return update.oldState;
  }
  // obtain the state that actually got programmed
  return StateDelta(update.newState, outDelta).newState();
}

fsdb::OperDelta HwSwitchSyncer::stateChangedImpl(
    const fsdb::OperDelta& delta,
    bool transaction) {
  return transaction ? hwSwitch_->stateChangedTransaction(delta)
                     : hwSwitch_->stateChanged(delta);
}

} // namespace facebook::fboss
