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

  std::vector<phy::PrbsLaneStats> getPrbsStats(
      phy::Side /* side */) const override {
    return {};
  }

  void updateXphyStats(
      const phy::ExternalPhyPortStats& /* stats */,
      std::optional<std::chrono::seconds> /* now */) override {}

 private:
  void updateLanePrbsStats(
      phy::Side /* side */,
      LaneID /* lane */,
      const phy::ExternalPhyLaneStats& /* laneStats */) override {}
};

} // namespace facebook::fboss
