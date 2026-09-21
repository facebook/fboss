/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/MultiSwitchFb303Stats.h"
#include <chrono>

#include "fboss/agent/FbossError.h"

using facebook::fboss::FbossError;
using facebook::fboss::HwAsic;
using facebook::fboss::SwitchID;

namespace {
std::string getAsicVendor(const std::map<SwitchID, const HwAsic*>& asicsMap) {
  CHECK(!asicsMap.empty());
  std::optional<std::string> vendor;
  for (auto& [switchId, asic] : asicsMap) {
    if (!vendor) {
      vendor = asic->getVendor();
    } else if (*vendor != asic->getVendor()) {
      throw FbossError("Mismatched ASIC vendors");
    }
  }
  return *vendor;
}
} // namespace

namespace facebook::fboss {
MultiSwitchFb303Stats::MultiSwitchFb303Stats(
    const std::map<SwitchID, const HwAsic*>& asicsMap)
    : hwSwitchFb303GlobalStats_(
          fb303::ThreadCachedServiceData::get()->getThreadStats(),
          getAsicVendor(asicsMap)),
      hwSwitchFb303CpuStats_({}, std::nullopt) {}

void MultiSwitchFb303Stats::updateStats(HwSwitchFb303GlobalStats& globalStats) {
  hwSwitchFb303GlobalStats_.updateStats(globalStats);
}

void MultiSwitchFb303Stats::updateStats(CpuPortStats& cpuPortStats) {
  hwSwitchFb303CpuStats_.updateStats(cpuPortStats);
}

} // namespace facebook::fboss
