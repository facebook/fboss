/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/HwPortFb303Stats.h"

#include "fboss/agent/hw/CounterUtils.h"
#include "fboss/agent/hw/StatsConstants.h"

#include <folly/logging/xlog.h>

namespace facebook::fboss {

HwPortFb303Stats::~HwPortFb303Stats() {
  for (const auto& statName : statNames()) {
    utility::deleteCounter(statName);
  }
}
std::string HwPortFb303Stats::statName(
    folly::StringPiece statName,
    folly::StringPiece portName) {
  return folly::to<std::string>(portName, ".", statName);
}

std::vector<std::string> HwPortFb303Stats::statNames() const {
  std::vector<std::string> stats{portCounters_.size()};
  auto idx = 0;
  for (const auto& statKeyAndStat : portCounters_) {
    stats[idx++] = statKeyAndStat.second.getName();
  }
  return stats;
}

void HwPortFb303Stats::reinitPortStat(
    folly::StringPiece statKey,
    const std::string& portName) {
  auto stat = getPortCounterIf(statKey);

  if (!stat) {
    portCounters_.emplace(
        statKey.str(),
        stats::MonotonicCounter(
            statName(statKey, portName), fb303::SUM, fb303::RATE));
  } else if (stat->getName() != statName(statKey, portName)) {
    stats::MonotonicCounter newStat{
        statName(statKey, portName), fb303::SUM, fb303::RATE};
    stat->swap(newStat);
    utility::deleteCounter(newStat.getName());
  }
}

const stats::MonotonicCounter* HwPortFb303Stats::getPortCounterIf(
    folly::StringPiece statKey) const {
  auto pcitr = portCounters_.find(statKey.str());
  return pcitr != portCounters_.end() ? &pcitr->second : nullptr;
}

stats::MonotonicCounter* HwPortFb303Stats::getPortCounterIf(
    folly::StringPiece statKey) {
  return const_cast<stats::MonotonicCounter*>(
      const_cast<const HwPortFb303Stats*>(this)->getPortCounterIf(statKey));
}
int64_t HwPortFb303Stats::getPortCounterLastIncrement(
    folly::StringPiece statKey) const {
  return getPortCounterIf(statKey)->get();
}

void HwPortFb303Stats::reinitPortStats(const std::string& portName) {
  XLOG(DBG2) << "Reinitializing stats for " << portName;

  reinitPortStat(kInBytes(), portName);
  reinitPortStat(kInUnicastPkts(), portName);
  reinitPortStat(kInMulticastPkts(), portName);
  reinitPortStat(kInBroadcastPkts(), portName);
  reinitPortStat(kInDiscards(), portName);
  reinitPortStat(kInErrors(), portName);
  reinitPortStat(kInPause(), portName);
  reinitPortStat(kInIpv4HdrErrors(), portName);
  reinitPortStat(kInIpv6HdrErrors(), portName);
  reinitPortStat(kInDstNullDiscards(), portName);
  reinitPortStat(kInDiscardsRaw(), portName);

  reinitPortStat(kOutBytes(), portName);
  reinitPortStat(kOutUnicastPkts(), portName);
  reinitPortStat(kOutMulticastPkts(), portName);
  reinitPortStat(kOutBroadcastPkts(), portName);
  reinitPortStat(kOutDiscards(), portName);
  reinitPortStat(kOutErrors(), portName);
  reinitPortStat(kOutPause(), portName);
  reinitPortStat(kOutCongestionDiscards(), portName);
  reinitPortStat(kOutEcnCounter(), portName);
}

void HwPortFb303Stats::updateStat(
    const std::chrono::seconds& now,
    folly::StringPiece statKey,
    int64_t val) {
  auto stat = getPortCounterIf(statKey);
  stat->updateValue(now, val);
}

void HwPortFb303Stats::updateStats(
    const HwPortStats& curPortStats,
    const std::chrono::seconds& retrievedAt) {
  timeRetrieved_ = retrievedAt;
  updateStat(timeRetrieved_, kInBytes(), curPortStats.inBytes_);
  updateStat(timeRetrieved_, kInUnicastPkts(), curPortStats.inUnicastPkts_);
  updateStat(timeRetrieved_, kInMulticastPkts(), curPortStats.inMulticastPkts_);
  updateStat(timeRetrieved_, kInBroadcastPkts(), curPortStats.inBroadcastPkts_);
  updateStat(timeRetrieved_, kInDiscardsRaw(), curPortStats.inDiscardsRaw_);
  updateStat(timeRetrieved_, kInDiscards(), curPortStats.inDiscards_);
  updateStat(timeRetrieved_, kInErrors(), curPortStats.inErrors_);
  updateStat(timeRetrieved_, kInPause(), curPortStats.inPause_);
  updateStat(timeRetrieved_, kInIpv4HdrErrors(), curPortStats.inIpv4HdrErrors_);
  updateStat(timeRetrieved_, kInIpv6HdrErrors(), curPortStats.inIpv6HdrErrors_);
  updateStat(
      timeRetrieved_, kInDstNullDiscards(), curPortStats.inDstNullDiscards_);
  // Egress Stats
  updateStat(timeRetrieved_, kOutBytes(), curPortStats.outBytes_);
  updateStat(timeRetrieved_, kOutUnicastPkts(), curPortStats.outUnicastPkts_);
  updateStat(
      timeRetrieved_, kOutMulticastPkts(), curPortStats.outMulticastPkts_);
  updateStat(
      timeRetrieved_, kOutBroadcastPkts(), curPortStats.outBroadcastPkts_);
  updateStat(timeRetrieved_, kOutDiscards(), curPortStats.outDiscards_);
  updateStat(timeRetrieved_, kOutErrors(), curPortStats.outErrors_);
  updateStat(timeRetrieved_, kOutPause(), curPortStats.outPause_);
  updateStat(
      timeRetrieved_,
      kOutCongestionDiscards(),
      curPortStats.outCongestionDiscardPkts_);
  updateStat(timeRetrieved_, kOutEcnCounter(), curPortStats.outEcnCounter_);
}
} // namespace facebook::fboss
