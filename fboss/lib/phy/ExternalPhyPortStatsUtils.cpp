/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/lib/phy/ExternalPhyPortStatsUtils.h"

#include <folly/logging/xlog.h>
#include <thrift/lib/cpp/util/EnumUtils.h>

#include <fb303/ServiceData.h>

#include <chrono>

namespace facebook::fboss {

float_t ExternalPhyPortStatsUtils::ExternalPhyLanePrbsStatsEntry::calculateBer(
    uint64_t numErrors,
    uint64_t microseconds) const {
  // The time duration in the following equation is in microseconds so that
  // balances the 10^6 for Megabits to bits conversion
  float_t numBitsInSec = laneSpeedInMb * microseconds;
  return static_cast<float_t>(numErrors) / numBitsInSec;
}

ExternalPhyPortStatsUtils::ExternalPhyPortStatsUtils(std::string prefix)
    : prefix_(std::move(prefix)),
      systemFecUncorr_(
          {constructCounterName("xphy.system.fec_uncorrectable"),
           fb303::SUM,
           fb303::RATE}),
      lineFecUncorr_(
          {constructCounterName("xphy.line.fec_uncorrectable"),
           fb303::SUM,
           fb303::RATE}),
      systemFecCorr_(
          {constructCounterName("xphy.system.fec_correctable"),
           fb303::SUM,
           fb303::RATE}),
      lineFecCorr_(
          {constructCounterName("xphy.line.fec_correctable"),
           fb303::SUM,
           fb303::RATE}) {}

std::chrono::seconds ExternalPhyPortStatsUtils::getCurrentTime(
    std::optional<std::chrono::seconds> now) {
  if (now) {
    return *now;
  } else {
    return std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch());
  }
}

void ExternalPhyPortStatsUtils::updateXphyStats(
    const phy::ExternalPhyPortStats& stats,
    std::optional<std::chrono::seconds> now) {
  std::chrono::seconds cur = getCurrentTime(now);
  // update common xphy stats
  systemFecUncorr_.updateValue(cur, stats.system.fecUncorrectableErrors);
  lineFecUncorr_.updateValue(cur, stats.line.fecUncorrectableErrors);
  systemFecCorr_.updateValue(cur, stats.system.fecCorrectableErrors);
  lineFecCorr_.updateValue(cur, stats.line.fecCorrectableErrors);

  auto updateSideLaneStats = [this](auto& sideStr, auto& sideStats) {
    auto updateStat = [this, &sideStr](
                          auto lane, auto& counterStr, auto& diagValue) {
      auto counterKey = this->constructCounterName(folly::to<std::string>(
          "xphy.", sideStr, ".lane", lane, ".", counterStr));
      fb303::fbData->setCounter(counterKey, diagValue);
    };
    for (const auto& laneStats : sideStats.lanes) {
      updateStat(
          laneStats.first, "signal_detect", laneStats.second.signalDetect);
      updateStat(laneStats.first, "cdr_lock", laneStats.second.cdrLock);
    }
  };

  updateSideLaneStats("system", stats.system);
  updateSideLaneStats("line", stats.line);
}

void ExternalPhyPortStatsUtils::setupPrbsCollection(
    phy::Side side,
    const std::vector<LaneID>& lanes,
    int laneSpeedInMb) {
  auto now = steady_clock::now();
  std::map<LaneID, ExternalPhyLanePrbsStatsEntry> newSideLanePrbsStatsMap;
  for (auto lane : lanes) {
    ExternalPhyLanePrbsStatsEntry externalPhyLanePrbsStatsEntry;
    externalPhyLanePrbsStatsEntry.laneSpeedInMb = laneSpeedInMb;
    externalPhyLanePrbsStatsEntry.timeLastCleared = now;
    externalPhyLanePrbsStatsEntry.timeLastCollect = now;
    newSideLanePrbsStatsMap[lane] = externalPhyLanePrbsStatsEntry;
    XLOG(DBG2) << "Setup externalPhyLanePrbsStatsEntry for " << prefix_
               << ", side=" << apache::thrift::util::enumNameSafe(side)
               << ", lane " << static_cast<int>(lane) << " at " << prefix_;
  }
  // Always replace existing prbs stats entry
  sideToLanePrbsStats_[side] = std::move(newSideLanePrbsStatsMap);
}

void ExternalPhyPortStatsUtils::clearPrbsStats(phy::Side side) {
  auto sideLanesStats = sideToLanePrbsStats_.find(side);
  if (sideLanesStats == sideToLanePrbsStats_.end()) {
    XLOG(WARN) << "LanePrbsStats wasn't setup for side "
               << apache::thrift::util::enumNameSafe(side) << " at " << prefix_;
    return;
  }

  for (auto& statsIter : sideLanesStats->second) {
    statsIter.second.accuErrorCount = 0;
    statsIter.second.numLinkLoss = 0;
    statsIter.second.maxBer = -1.;
    statsIter.second.timeLastLocked = statsIter.second.locked
        ? statsIter.second.timeLastCollect
        : steady_clock::time_point();
    statsIter.second.timeLastCleared = steady_clock::now();
  }
}

void ExternalPhyPortStatsUtils::updateXphyPrbsStats(
    const phy::ExternalPhyPortStats& stats,
    std::optional<std::chrono::seconds> /* now */) {
  if (sideToLanePrbsStats_.find(phy::Side::SYSTEM) !=
      sideToLanePrbsStats_.end()) {
    for (const auto& kv : stats.system.lanes) {
      if (kv.second.prbsErrorCounts) {
        updateLanePrbsStats(phy::Side::SYSTEM, LaneID(kv.first), kv.second);
      }
    }
  }
  if (sideToLanePrbsStats_.find(phy::Side::LINE) !=
      sideToLanePrbsStats_.end()) {
    for (const auto& kv : stats.line.lanes) {
      if (kv.second.prbsErrorCounts) {
        updateLanePrbsStats(phy::Side::LINE, LaneID(kv.first), kv.second);
      }
    }
  }
}

void ExternalPhyPortStatsUtils::disablePrbsCollection(phy::Side side) {
  if (auto it = sideToLanePrbsStats_.find(side);
      it != sideToLanePrbsStats_.end()) {
    sideToLanePrbsStats_.erase(it);
    XLOG(DBG2) << "Disabled " << prefix_
               << ", side=" << apache::thrift::util::enumNameSafe(side)
               << " prbs collection.";
  } else {
    XLOG(WARN)
        << prefix_ << ", side=" << apache::thrift::util::enumNameSafe(side)
        << " hasn't set up externalPhyLanePrbsStatsEntry yet. Skip disabling";
  }
}

bool ExternalPhyPortStatsUtils::isPrbsCollectionEnabled(phy::Side side) {
  return sideToLanePrbsStats_.find(side) != sideToLanePrbsStats_.end();
}

} // namespace facebook::fboss
