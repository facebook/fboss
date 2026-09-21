/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#pragma once
#include <memory>
#include <string>

#include "fboss/agent/HwAsicTable.h"
#include "fboss/agent/hw/HwCpuFb303Stats.h"
#include "fboss/agent/hw/HwSwitchFb303Stats.h"
#include "fboss/agent/types.h"

namespace facebook::fboss {
class MultiSwitchFb303Stats {
 public:
  explicit MultiSwitchFb303Stats(
      const std::map<SwitchID, const HwAsic*>& asicsMap);
  void updateStats(HwSwitchFb303GlobalStats& globalStats);
  void updateStats(CpuPortStats& cpuStats);
  const HwSwitchFb303Stats& getHwSwitchFb303GlobalStats() const {
    return hwSwitchFb303GlobalStats_;
  }

 private:
  HwSwitchFb303Stats hwSwitchFb303GlobalStats_;
  HwCpuFb303Stats hwSwitchFb303CpuStats_;
};
} // namespace facebook::fboss
