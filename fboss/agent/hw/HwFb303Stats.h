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

#include "folly/container/F14Map.h"

#include <optional>
#include <string>
namespace facebook::fboss {

class HwFb303Stats {
 public:
  ~HwFb303Stats();

  int64_t getCounterLastIncrement(const std::string& statName) const;

  /*
   * Reinit stat
   */
  void reinitStat(
      const std::string& statName,
      std::optional<std::string> oldStatName);
  void updateStat(
      const std::chrono::seconds& now,
      const std::string& statName,
      int64_t val);
  void removeStat(const std::string& statName);

 private:
  /*
   * Update queue stat
   */
  stats::MonotonicCounter* getCounterIf(const std::string& statName);
  const stats::MonotonicCounter* getCounterIf(
      const std::string& statName) const;

  folly::F14FastMap<std::string, stats::MonotonicCounter> counters_;
};
} // namespace facebook::fboss
