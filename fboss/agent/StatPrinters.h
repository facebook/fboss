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

#include <ostream>
#include <string>
#include "fboss/agent/hw/gen-cpp2/hardware_stats_types.h"
#include "fboss/lib/phy/gen-cpp2/phy_types.h"

namespace facebook::fboss {

void toAppend(const HwPortStats& stats, folly::fbstring* result);
void toAppend(const HwSysPortStats& stats, folly::fbstring* result);
void toAppend(const HwTrunkStats& stats, folly::fbstring* result);
void toAppend(const HwResourceStats& stats, folly::fbstring* result);
void toAppend(const HwAsicErrors& stats, folly::fbstring* result);
void toAppend(const HwTeFlowStats& stats, folly::fbstring* result);
void toAppend(const TeFlowStats& stats, folly::fbstring* result);
void toAppend(const FabricReachabilityStats& stats, folly::fbstring* result);
void toAppend(const HwBufferPoolStats& stats, folly::fbstring* result);
void toAppend(const CpuPortStats& stats, folly::fbstring* result);
void toAppend(const HwSwitchDropStats& stats, folly::fbstring* result);
void toAppend(const HwSwitchDramStats& stats, folly::fbstring* result);
void toAppend(const HwSwitchFb303GlobalStats& stats, folly::fbstring* result);
void toAppend(const HwFlowletStats& stats, folly::fbstring* result);
void toAppend(const phy::PhyInfo& phy, folly::fbstring* result);
void toAppend(const AclStats& stats, folly::fbstring* result);
void toAppend(const HwSwitchWatermarkStats& stats, folly::fbstring* result);

std::ostream& operator<<(std::ostream& os, const HwPortStats& stats);
std::ostream& operator<<(std::ostream& os, const HwSysPortStats& stats);
std::ostream& operator<<(std::ostream& os, const HwTrunkStats& stats);
std::ostream& operator<<(std::ostream& os, const HwResourceStats& stats);
std::ostream& operator<<(std::ostream& os, const HwAsicErrors& stats);
std::ostream& operator<<(std::ostream& os, const HwTeFlowStats& stats);
std::ostream& operator<<(std::ostream& os, const TeFlowStats& stats);
std::ostream& operator<<(
    std::ostream& os,
    const FabricReachabilityStats& stats);
std::ostream& operator<<(std::ostream& os, const HwBufferPoolStats& stats);
std::ostream& operator<<(std::ostream& os, const CpuPortStats& stats);
std::ostream& operator<<(std::ostream& os, const HwSwitchDropStats& stats);
std::ostream& operator<<(std::ostream& os, const HwSwitchDramStats& stats);
std::ostream& operator<<(
    std::ostream& os,
    const HwSwitchFb303GlobalStats& stats);
std::ostream& operator<<(std::ostream& os, const HwFlowletStats& stats);
std::ostream& operator<<(std::ostream& os, const phy::PhyInfo& phy);
std::ostream& operator<<(std::ostream& os, const AclStats& stats);
std::ostream& operator<<(std::ostream& os, const HwSwitchWatermarkStats& stats);
} // namespace facebook::fboss
