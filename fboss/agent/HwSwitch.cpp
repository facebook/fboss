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
#include "fboss/agent/Utils.h"
#include "fboss/agent/hw/HwSwitchStats.h"
#include "fboss/agent/hw/switch_asics/HwAsic.h"
#include "fboss/agent/normalization/Normalizer.h"
#include "fboss/agent/state/StateDelta.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/state/TransceiverMap.h"

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

HwSwitchStats* HwSwitch::getSwitchStats() const {
  if (!hwSwitchStats_) {
    hwSwitchStats_.reset(new HwSwitchStats(
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

void HwSwitch::gracefulExit(
    folly::dynamic& follySwitchState,
    state::WarmbootState& thriftSwitchState) {
  if (getPlatform()->getAsic()->isSupported(HwAsic::Feature::WARMBOOT)) {
    gracefulExitImpl(follySwitchState, thriftSwitchState);
  }
}

std::shared_ptr<SwitchState> HwSwitch::stateChangedTransaction(
    const StateDelta& delta) {
  if (!transactionsSupported()) {
    throw FbossError("Transactions not supported on this switch");
  }
  try {
    return stateChanged(delta);
  } catch (const FbossError& e) {
    XLOG(WARNING) << " Transaction failed with error : " << *e.message()
                  << " attempting rollback";
    this->rollback(delta.oldState());
  }
  return delta.oldState();
}

void HwSwitch::rollback(
    const std::shared_ptr<SwitchState>& /*knownGoodState*/) noexcept {
  XLOG(FATAL)
      << "Transactions is supported but rollback is implemented on this switch";
}
} // namespace facebook::fboss
