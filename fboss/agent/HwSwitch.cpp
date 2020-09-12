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
#include "fboss/agent/hw/HwSwitchStats.h"
#include "fboss/agent/hw/switch_asics/HwAsic.h"
#include "fboss/agent/normalization/Normalizer.h"

#include "fboss/agent/FbossError.h"
#include "fboss/agent/Utils.h"

#include <fb303/ThreadCachedServiceData.h>
#include <folly/FileUtil.h>
#include <folly/experimental/TestUtil.h>
#include <folly/logging/xlog.h>

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
  static HwSwitchStats hwSwitchStats(
      fb303::ThreadCachedServiceData::get()->getThreadStats(),
      getPlatform()->getAsic()->getVendor());
  return &hwSwitchStats;
}

void HwSwitch::switchRunStateChanged(SwitchRunState newState) {
  if (runState_ != newState) {
    switchRunStateChangedImpl(newState);
    runState_ = newState;
    XLOG(INFO) << " HwSwitch run state changed to: "
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
} // namespace facebook::fboss
