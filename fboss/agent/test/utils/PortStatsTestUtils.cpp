/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/test/utils/PortStatsTestUtils.h"

#include "fboss/agent/SwitchStats.h"

#include <folly/logging/xlog.h>

#include <thrift/lib/cpp2/protocol/Serializer.h>
#include <numeric>

namespace facebook::fboss::utility {

namespace {
template <typename Fn>
uint64_t accumalatePortStats(
    const std::map<PortID, HwPortStats>& port2Stats,
    const Fn& fn) {
  return std::accumulate(port2Stats.begin(), port2Stats.end(), 0UL, fn);
}
} // namespace

uint64_t getPortOutPkts(const HwPortStats& portStats) {
  return *portStats.outUnicastPkts_() + *portStats.outMulticastPkts_() +
      *portStats.outBroadcastPkts_();
}

uint64_t getPortOutPkts(const std::map<PortID, HwPortStats>& port2Stats) {
  return accumalatePortStats(
      port2Stats, [](auto sum, const auto& portIdAndStats) {
        return sum + getPortOutPkts(portIdAndStats.second);
      });
}

uint64_t getPortInPkts(const HwPortStats& portStats) {
  return *portStats.inUnicastPkts_() + *portStats.inMulticastPkts_() +
      *portStats.inBroadcastPkts_();
}

uint64_t getPortInPkts(const std::map<PortID, HwPortStats>& port2Stats) {
  return accumalatePortStats(
      port2Stats, [](auto sum, const auto& portIdAndStats) {
        return sum + getPortInPkts(portIdAndStats.second);
      });
}

void printPortStats(const std::map<PortID, HwPortStats>& port2Stats) {
  for (auto& [port, stats] : port2Stats) {
    XLOG(DBG1) << "Stats for port: " << port;
    XLOG(DBG1) << apache::thrift::SimpleJSONSerializer::serialize<std::string>(
        stats);
  }
}

} // namespace facebook::fboss::utility
