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

#include "fboss/lib/phy/ExternalPhyPortStatsUtils.h"

#include "common/stats/MonotonicCounter.h"
#include "fboss/agent/if/gen-cpp2/FbossCtrl.h"
#include "fboss/lib/phy/ExternalPhy.h"

#include <chrono>

namespace facebook::fboss {

// Adding this for now just so that we can make ElbertPort/FujiPort extend from
// ExternalPhyPort despite them not having their own portStats objects yet
// TODO (ccpowers): remove this once all platforms have their own PortStats type
class NullPortStats : public ExternalPhyPortStatsUtils {
 public:
  explicit NullPortStats(std::string prefix)
      : ExternalPhyPortStatsUtils(prefix) {}

  void clearPrbsStats(
      const phy::PhyPortConfig& /* phyPortConfig */,
      phy::Side /* side */) override {}

  std::vector<PrbsLaneStats> getPrbsStats(
      const phy::PhyPortConfig& /* phyPortConfig */,
      phy::Side /* side */) const override {
    return {};
  }

  void updateXphyStats(
      const phy::ExternalPhyPortStats& /* stats */,
      std::optional<std::chrono::seconds> /* now */) override {}

  void updateXphyPrbsStats(
      const phy::ExternalPhyPortStats& /* stats */,
      std::optional<std::chrono::seconds> /* now */) override {}
};

} // namespace facebook::fboss
