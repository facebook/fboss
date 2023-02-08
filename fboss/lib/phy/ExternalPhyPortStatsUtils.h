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

#include "common/stats/MonotonicCounter.h"
#include "fboss/lib/phy/ExternalPhy.h"

#include <chrono>

namespace facebook::fboss {

using std::chrono::steady_clock;

class ExternalPhyPortStatsUtils {
 public:
  struct ExternalPhyLanePrbsStatsEntry {
    bool locked{false};
    int laneSpeedInMb{0};
    uint64_t accuErrorCount{0};
    double maxBer{-1.};
    uint32_t numLinkLoss{0};
    steady_clock::time_point timeLastLocked;
    steady_clock::time_point timeLastCleared;
    steady_clock::time_point timeLastCollect;

    float_t calculateBer(uint64_t numErrors, uint64_t microseconds) const;
  };

  explicit ExternalPhyPortStatsUtils(std::string prefix);
  virtual ~ExternalPhyPortStatsUtils() = default;

  virtual void updateXphyStats(
      const phy::ExternalPhyPortStats& stats,
      std::optional<std::chrono::seconds> now = std::nullopt);
  void updateXphyPrbsStats(
      const phy::ExternalPhyPortStats& stats,
      std::optional<std::chrono::seconds> now = std::nullopt);

  virtual void setupPrbsCollection(
      phy::Side side,
      const std::vector<LaneID>& lanes,
      int laneSpeedInMb);
  void disablePrbsCollection(phy::Side side);
  bool isPrbsCollectionEnabled(phy::Side side);

  virtual std::vector<phy::PrbsLaneStats> getPrbsStats(
      phy::Side side) const = 0;

  void clearPrbsStats(phy::Side side);

 protected:
  template <typename KeyT>
  std::string constructCounterName(KeyT&& counterKey) {
    return folly::to<std::string>(prefix_, ".", std::forward<KeyT>(counterKey));
  }

  std::chrono::seconds getCurrentTime(std::optional<std::chrono::seconds> now);

  std::string prefix_;
  // common xphy stats
  facebook::stats::MonotonicCounter systemFecUncorr_;
  facebook::stats::MonotonicCounter lineFecUncorr_;
  facebook::stats::MonotonicCounter systemFecCorr_;
  facebook::stats::MonotonicCounter lineFecCorr_;

  // System and Line side lanes might use the same id, hence use phy::Side
  // to saperate both sides lane to prbs stats mapping
  std::unordered_map<phy::Side, std::map<LaneID, ExternalPhyLanePrbsStatsEntry>>
      sideToLanePrbsStats_;

 private:
  virtual void updateLanePrbsStats(
      phy::Side side,
      LaneID lane,
      const phy::ExternalPhyLaneStats& laneStats) = 0;
};

} // namespace facebook::fboss
