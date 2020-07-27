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

#include "fboss/agent/hw/gen-cpp2/hardware_stats_types.h"

#include "fboss/agent/types.h"

#include <map>

namespace facebook::fboss {

class HwSwitch;

void updateHwSwitchStats(HwSwitch* hw);
uint64_t getPortOutPkts(const HwPortStats& portStats);
uint64_t getPortOutPkts(const std::map<PortID, HwPortStats>& port2Stats);
uint64_t getPortInPkts(const HwPortStats& portStats);
uint64_t getPortInPkts(const std::map<PortID, HwPortStats>& port2Stats);

} // namespace facebook::fboss
