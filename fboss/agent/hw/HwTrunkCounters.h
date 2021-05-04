// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#pragma once

#include "common/stats/MonotonicCounter.h"

#include "fboss/agent/hw/gen-cpp2/hardware_stats_types.h"
#include "fboss/agent/types.h"

namespace facebook::fboss::utility {

class HwTrunkCounters {
 public:
  HwTrunkCounters(AggregatePortID aggPortID, std::string trunkName);
  void updateCounters(std::chrono::seconds now, const HwTrunkStats& stats);
  void reinitialize(AggregatePortID aggPortID, std::string trunkName);

 private:
  // Helpers which operate on an individual counter
  void initializeCounter(folly::StringPiece counterKey);
  stats::MonotonicCounter* getCounterIf(folly::StringPiece counterKey);
  std::string constructCounterName(folly::StringPiece counterKey) const;
  void updateCounter(
      std::chrono::seconds now,
      folly::StringPiece counterKey,
      int64_t value);

  AggregatePortID aggregatePortID_;
  std::string trunkName_;
  std::map<std::string, stats::MonotonicCounter> counters_;
};

void clearHwTrunkStats(HwTrunkStats& stats);
} // namespace facebook::fboss::utility
