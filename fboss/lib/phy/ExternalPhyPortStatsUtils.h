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
#include "fboss/agent/if/gen-cpp2/FbossCtrl.h"
#include "fboss/lib/phy/ExternalPhy.h"

#include <chrono>

namespace facebook::fboss {

using std::chrono::steady_clock;

class ExternalPhyPortStatsUtils {
 public:
  struct ExternalPhyLanePrbsStatsEntry {
    bool locked{false};
    double laneSpeed{0.};
    int64_t accuErrorCount{0};
    double maxBer{-1.};
    int32_t numLinkLoss{0};
    steady_clock::time_point timeLastLocked;
    steady_clock::time_point timeLastCleared;
    steady_clock::time_point timeLastCollect;
  };

  explicit ExternalPhyPortStatsUtils(std::string prefix);
  virtual ~ExternalPhyPortStatsUtils() = default;

  virtual void updateXphyStats(
      const phy::ExternalPhyPortStats& stats,
      std::optional<std::chrono::seconds> now = std::nullopt);
  virtual void updateXphyPrbsStats(
      const phy::ExternalPhyPortStats& stats,
      std::optional<std::chrono::seconds> now = std::nullopt) = 0;

  virtual void setupPrbsCollection(
      const phy::PhyPortConfig& phyPortConfig,
      phy::Side side,
      float_t laneSpeed);

  virtual std::vector<PrbsLaneStats> getPrbsStats(
      const phy::PhyPortConfig& phyPortConfig,
      phy::Side side) const = 0;

  virtual void clearPrbsStats(
      const phy::PhyPortConfig& phyPortConfig,
      phy::Side side) = 0;

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

  folly::Synchronized<std::map<int32_t, ExternalPhyLanePrbsStatsEntry>>
      lanePrbsStatsMap_;
};

} // namespace facebook::fboss
