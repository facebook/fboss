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

#include <fb303/ServiceData.h>

#include <chrono>

namespace facebook::fboss {

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
    const phy::PhyPortConfig& phyPortConfig,
    phy::Side side,
    float_t laneSpeed) {
  auto now = steady_clock::now();
  const auto& lanes = (side == phy::Side::SYSTEM)
      ? phyPortConfig.config.system.lanes
      : phyPortConfig.config.line.lanes;
  auto lockedlanePrbsStatsMap = lanePrbsStatsMap_.wlock();
  for (const auto& it : lanes) {
    auto lane = it.first;
    auto statIter = lockedlanePrbsStatsMap->find(lane);
    if (statIter != lockedlanePrbsStatsMap->end()) {
      XLOG(INFO) << "ExternalPhyLanePrbsStats already exists for lane " << lane;
      lockedlanePrbsStatsMap->erase(lane);
    }
    ExternalPhyLanePrbsStatsEntry externalPhyLanePrbsStatsEntry;
    externalPhyLanePrbsStatsEntry.laneSpeed = laneSpeed;
    externalPhyLanePrbsStatsEntry.timeLastCleared = now;
    externalPhyLanePrbsStatsEntry.timeLastCollect = now;
    lockedlanePrbsStatsMap->emplace(
        lane, std::move(externalPhyLanePrbsStatsEntry));
    XLOG(INFO) << "Emplaced externalPhyLanePrbsStatsEntry for lane " << lane
               << " at " << prefix_;
  }
}

} // namespace facebook::fboss
