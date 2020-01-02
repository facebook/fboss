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
#include "fboss/agent/hw/StatsConstants.h"
#include "fboss/agent/hw/gen-cpp2/hardware_stats_types.h"
#include "fboss/agent/types.h"

#include <boost/container/flat_set.hpp>
#include <folly/CppAttributes.h>
#include <folly/Range.h>
#include <folly/Synchronized.h>

#include <map>
#include <string>
#include <utility>

namespace facebook::fboss {

class BcmSwitchIf;

class BcmTrunkStats {
 public:
  explicit BcmTrunkStats(const BcmSwitchIf* hw);

  void initialize(AggregatePortID aggPortID, std::string trunkName);
  void update();

  void grantMembership(PortID memberPortID);
  void revokeMembership(PortID memberPortID);

 private:
  BcmTrunkStats(const BcmTrunkStats&) = delete;
  BcmTrunkStats& operator=(const BcmTrunkStats&) = delete;

  // Helpers operating on an HwTrunkStats object
  static void clearHwTrunkStats(HwTrunkStats& stats);
  std::pair<HwTrunkStats, std::chrono::seconds> accumulateMemberStats() const;

  // Helpers which operate on an individual counter
  void initializeCounter(folly::StringPiece counterKey);
  stats::MonotonicCounter* FOLLY_NULLABLE
  getCounterIf(folly::StringPiece counterKey);
  void updateCounter(
      std::chrono::seconds now,
      folly::StringPiece counterKey,
      int64_t value);
  std::string constructCounterName(folly::StringPiece counterKey) const;

  const BcmSwitchIf* const hw_;
  std::string trunkName_;
  AggregatePortID aggregatePortID_{0};

  using SynchronizedPortIDs =
      folly::Synchronized<boost::container::flat_set<PortID>>;
  SynchronizedPortIDs memberPortIDs_;

  std::map<std::string, stats::MonotonicCounter> counters_;
};

} // namespace facebook::fboss
