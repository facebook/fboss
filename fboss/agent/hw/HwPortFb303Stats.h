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

#include "fboss/agent/hw/gen-cpp2/hardware_stats_types.h"

#include "common/stats/MonotonicCounter.h"

#include "folly/container/F14Map.h"

#include <string>

namespace facebook::fboss {

class HwPortFb303Stats {
 public:
  explicit HwPortFb303Stats(const std::string& portName) {
    reinitPortStats(portName);
  }
  ~HwPortFb303Stats();

  void updateStats(
      const HwPortStats& latestStats,
      const std::chrono::seconds& retrievedAt);
  void reinitPortStats(const std::string& portName);

  std::vector<std::string> statNames() const;

  static std::string statName(
      folly::StringPiece statName,
      folly::StringPiece portName);

  int64_t getPortCounterLastIncrement(folly::StringPiece statKey) const;

 private:
  void updateStat(
      const std::chrono::seconds& now,
      folly::StringPiece statKey,
      int64_t val);
  stats::MonotonicCounter* getPortCounterIf(folly::StringPiece statKey);
  const stats::MonotonicCounter* getPortCounterIf(
      folly::StringPiece statKey) const;
  void reinitPortStat(folly::StringPiece statKey, const std::string& portName);
  folly::F14FastMap<std::string, stats::MonotonicCounter> portCounters_;
  std::chrono::seconds timeRetrieved_{0};
};

} // namespace facebook::fboss
