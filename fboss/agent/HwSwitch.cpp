/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/HwSwitch.h"

#include "fboss/agent/FbossError.h"
#include "fboss/agent/HwSwitchRouteUpdateWrapper.h"
#include "fboss/agent/Utils.h"
#include "fboss/agent/hw/HwSwitchFb303Stats.h"
#include "fboss/agent/hw/HwSwitchWarmBootHelper.h"
#include "fboss/agent/hw/switch_asics/HwAsic.h"
#include "fboss/agent/normalization/Normalizer.h"
#include "fboss/agent/rib/ForwardingInformationBaseUpdater.h"
#include "fboss/agent/state/StateDelta.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/state/TransceiverMap.h"
#include "fboss/lib/HwWriteBehavior.h"

#include <fb303/ThreadCachedServiceData.h>
#include <folly/FileUtil.h>
#include <folly/experimental/TestUtil.h>
#include <folly/logging/xlog.h>

DEFINE_bool(flexports, false, "Load the agent with flexport support enabled");
DEFINE_int32(
    update_watermark_stats_interval_s,
    60,
    "Update watermark stats interval in seconds");

DEFINE_bool(
    flowletSwitchingEnable,
    false,
    "Flag to turn on flowlet switching for DLB");

namespace facebook::fboss {

std::string HwSwitch::getDebugDump() const {
  folly::test::TemporaryDirectory tmpDir;
  auto fname = tmpDir.path().string() + "hw_debug_dump";
  dumpDebugState(fname);
  std::string out;
  if (!folly::readFile(fname.c_str(), out)) {
    throw FbossError("Unable get debug dump");
  }
  return out;
}

HwSwitchFb303Stats* HwSwitch::getSwitchStats() const {
  if (!hwSwitchStats_) {
    hwSwitchStats_.reset(new HwSwitchFb303Stats(
        fb303::ThreadCachedServiceData::get()->getThreadStats(),
        getPlatform()->getAsic()->getVendor()));
  }
  return hwSwitchStats_.get();
}

void HwSwitch::switchRunStateChanged(SwitchRunState newState) {
  if (runState_ != newState) {
    switchRunStateChangedImpl(newState);
    runState_ = newState;
    XLOG(DBG2) << " HwSwitch run state changed to: "
               << switchRunStateStr(runState_);
  }
}

void HwSwitch::updateStats(SwitchStats* switchStats) {
  updateStatsImpl(switchStats);
  // send to normalizer
  auto normalizer = Normalizer::getInstance();
  if (normalizer) {
    normalizer->processStats(getPortStats());
  }
}

uint32_t HwSwitch::generateDeterministicSeed(LoadBalancerID loadBalancerID) {
  return generateDeterministicSeed(
      loadBalancerID, getPlatform()->getLocalMac());
}

void HwSwitch::gracefulExit(const state::WarmbootState& thriftSwitchState) {
  if (getPlatform()->getAsic()->isSupported(HwAsic::Feature::WARMBOOT)) {
    gracefulExitImpl();
  }
}

std::shared_ptr<SwitchState> HwSwitch::stateChangedTransaction(
    const StateDelta& delta,
    const HwWriteBehaviorRAII& behavior) {
  if (!FLAGS_enable_state_oper_delta) {
    // failback to move away from oper delta
    if (!transactionsSupported()) {
      throw FbossError("Transactions not supported on this switch");
    }
    try {
      setProgrammedState(stateChanged(delta));
    } catch (const FbossError& e) {
      XLOG(WARNING) << " Transaction failed with error : " << *e.message()
                    << " attempting rollback";
      this->rollback(delta.oldState());
      setProgrammedState(delta.oldState());
    }
    return getProgrammedState();
  }
  auto result = stateChangedTransaction(delta.getOperDelta(), behavior);
  if (!result.changes()->empty()) {
    // changes have been rolled back to last good known state
    return delta.oldState();
  }
  return delta.newState();
}

void HwSwitch::rollback(
    const std::shared_ptr<SwitchState>& /*knownGoodState*/) noexcept {
  XLOG(FATAL)
      << "Transactions is supported but rollback is implemented on this switch";
}

std::shared_ptr<SwitchState> HwSwitch::getProgrammedState() const {
  auto programmedState = programmedState_.rlock();
  return *programmedState;
}

void HwSwitch::setProgrammedState(const std::shared_ptr<SwitchState>& state) {
  auto programmedState = programmedState_.wlock();
  *programmedState = state;
  (*programmedState)->publish();
}

fsdb::OperDelta HwSwitch::stateChanged(
    const fsdb::OperDelta& delta,
    const HwWriteBehaviorRAII& /*behavior*/) {
  auto stateDelta = StateDelta(getProgrammedState(), delta);
  auto state = stateChangedImpl(stateDelta);
  setProgrammedState(state);
  if (getProgrammedState() == stateDelta.newState()) {
    return fsdb::OperDelta{};
  }
  // return the delta between expected applied state and actually applied state
  // caller can then can construct actually applied state from its expected new
  // state from returning oper delta, and also know what was not applied from
  // the state delta between expected applied state and applied state.
  return StateDelta(stateDelta.newState(), getProgrammedState()).getOperDelta();
}

fsdb::OperDelta HwSwitch::stateChangedTransaction(
    const fsdb::OperDelta& delta,
    const HwWriteBehaviorRAII& /*behavior*/) {
  if (!transactionsSupported()) {
    throw FbossError("Transactions not supported on this switch");
  }
  auto goodKnownState = getProgrammedState();
  fsdb::OperDelta result{};
  try {
    result = stateChanged(delta);
  } catch (const FbossError& e) {
    XLOG(WARNING) << " Transaction failed with error : " << *e.message()
                  << " attempting rollback";
    this->rollback(goodKnownState);
    setProgrammedState(goodKnownState);
    return delta;
  }
  return result;
}

bool HwSwitch::isFullyConfigured() const {
  auto state = getRunState();
  return state >= SwitchRunState::CONFIGURED &&
      state != SwitchRunState::EXITING;
}

void HwSwitch::ensureConfigured(folly::StringPiece function) const {
  if (isFullyConfigured()) {
    return;
  }

  if (!function.empty()) {
    XLOG(DBG1) << "failing thrift prior to switch configuration: " << function;
  }
  throw FbossError(
      "switch is still initializing or is exiting and is not "
      "fully configured yet");
}

std::shared_ptr<SwitchState> HwSwitch::getMinAlpmState(
    RoutingInformationBase* rib,
    const std::shared_ptr<SwitchState>& state) {
  CHECK(getRunState() < SwitchRunState::INITIALIZED)
      << "Invalid run state for programming ALPM state";
  CHECK(getBootType() == BootType::COLD_BOOT)
      << "ALPM minimum state can only be programmed on cold boot";
  CHECK(
      getPlatform()->getAsic()->isSupported(HwAsic::Feature::ROUTE_PROGRAMMING))
      << "Route programming is not supported";
  CHECK(
      getSwitchType() == cfg::SwitchType::VOQ ||
      getSwitchType() == cfg::SwitchType::NPU)
      << "ALPM minimum state can only be programmed on support VOQ or NPU switch";

  std::shared_ptr<SwitchState> minAlpmState{};
  HwSwitchRouteUpdateWrapper routeUpdater(
      this, rib, [&minAlpmState](const StateDelta& delta) {
        minAlpmState = delta.newState();
        return minAlpmState;
      });
  routeUpdater.programMinAlpmState();
  return minAlpmState;
}

std::shared_ptr<SwitchState> HwSwitch::programMinAlpmState(
    RoutingInformationBase* rib) {
  return programMinAlpmState(
      rib, [=](const StateDelta& delta) { return stateChanged(delta); });
}

std::shared_ptr<SwitchState> HwSwitch::programMinAlpmState(
    RoutingInformationBase* rib,
    StateChangedFn func) {
  auto state = getProgrammedState();
  auto minAlpmState = getMinAlpmState(rib, state);
  return func(StateDelta(state, minAlpmState));
}

HwInitResult HwSwitch::init(Callback* callback, bool failHwCallsOnWarmboot) {
  using std::chrono::duration;
  using std::chrono::duration_cast;
  using std::chrono::steady_clock;

  steady_clock::time_point begin = steady_clock::now();
  HwInitResult ret{};
  ret.initializedTime =
      duration_cast<duration<float>>(steady_clock::now() - begin).count();
  ret.bootType = initLight(callback, failHwCallsOnWarmboot);
  if (ret.bootType == BootType::WARM_BOOT) {
    auto wbState =
        getPlatform()->getWarmBootHelper()->getSwSwitchWarmBootState();
    ret.switchState = SwitchState::fromThrift(*(wbState.swSwitchState()));
    const auto& routeTables = *(wbState.routeTables());
    ret.rib = RoutingInformationBase::fromThrift(
        routeTables,
        ret.switchState->getFibs(),
        ret.switchState->getLabelForwardingInformationBase());
  } else {
    // cold boot state is already programmed during initLight
    ret.switchState = getProgrammedState();
    ret.rib = std::make_unique<RoutingInformationBase>();
  }
  if (ret.bootType == BootType::WARM_BOOT) {
    // apply state only for warm boot. cold boot state is already applied.
    auto writeBehavior = getWarmBootWriteBehavior(failHwCallsOnWarmboot);
    setProgrammedState(std::make_shared<SwitchState>());
    ret.switchState = stateChanged(
        StateDelta(getProgrammedState(), ret.switchState), writeBehavior);
    setProgrammedState(ret.switchState);
  }
  ret.bootTime =
      duration_cast<duration<float>>(steady_clock::now() - begin).count();
  initialStateApplied();
  return ret;
}

HwWriteBehaviorRAII HwSwitch::getWarmBootWriteBehavior(
    bool failHwCallsOnWarmboot) const {
  if (failHwCallsOnWarmboot &&
      getPlatform()->getAsic()->isSupported(
          HwAsic::Feature::ZERO_SDK_WRITE_WARMBOOT)) {
    return HwWriteBehaviorRAII(HwWriteBehavior::FAIL);
  }
  return HwWriteBehaviorRAII(HwWriteBehavior::WRITE);
}

BootType HwSwitch::initLight(Callback* callback, bool failHwCallsOnWarmboot) {
  switchType_ = getPlatform()->getAsic()->getSwitchType();
  switchId_ = getPlatform()->getAsic()->getSwitchId();
  BootType bootType = BootType::COLD_BOOT;
  if (getPlatform()->getAsic()->getAsicType() !=
      cfg::AsicType::ASIC_TYPE_MOCK) {
    if (auto warmBootHelper = getPlatform()->getWarmBootHelper()) {
      bootType = warmBootHelper->canWarmBoot() ? BootType::WARM_BOOT
                                               : BootType::COLD_BOOT;
    }
  }
  // initialize hardware switch
  initImpl(callback, bootType, failHwCallsOnWarmboot);
  if (bootType == BootType::WARM_BOOT) {
    // on warm boot, no need to apply minimum alpm state
    return bootType;
  }
  if (switchType_ != cfg::SwitchType::NPU &&
      switchType_ != cfg::SwitchType::VOQ) {
    // no route programming, no need to apply minimum alpm state
    return bootType;
  }

  // program min alpm state for npu and voq only on cold boot
  std::map<int32_t, state::RouteTableFields> routeTables{};
  routeTables.emplace(kDefaultVrf, state::RouteTableFields{});
  auto rib = RoutingInformationBase::fromThrift(routeTables);
  programMinAlpmState(rib.get(), [this](const StateDelta& delta) {
    return stateChanged(delta);
  });
  return bootType;
}
} // namespace facebook::fboss
