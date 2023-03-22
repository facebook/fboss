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

#include "fboss/agent/hw/HwFb303Stats.h"
#include "fboss/agent/hw/gen-cpp2/hardware_stats_types.h"

#include "folly/container/F14Map.h"

#include <optional>
#include <string>

namespace facebook::fboss {

class HwCpuFb303Stats {
 public:
  using QueueId2Name = folly::F14FastMap<int, std::string>;
  explicit HwCpuFb303Stats(QueueId2Name queueId2Name = {})
      : queueId2Name_(queueId2Name) {
    setupStats();
  }
  void updateStats(
      const HwPortStats& latestStats,
      const std::chrono::seconds& retrievedAt);

  void queueChanged(int queueId, const std::string& queueName);
  void queueRemoved(int queueId);
  const folly::F14FastMap<int, std::string>& getQueueId2Name() const {
    return queueId2Name_;
  }

  /*
   * Cpu queue stat name
   */
  static std::string statName(
      folly::StringPiece statName,
      int queueuId,
      folly::StringPiece queueName);

  static std::array<folly::StringPiece, 2> kQueueStatKeys();
  int64_t getCounterLastIncrement(folly::StringPiece statKey) const;
  CpuPortStats getCpuPortStats() const;

 private:
  void setupStats();

  void updateStat(
      const std::chrono::seconds& now,
      const std::string& statName,
      int64_t val);

  std::chrono::seconds timeRetrieved_{0};
  HwFb303Stats queueCounters_;
  QueueId2Name queueId2Name_;
};

} // namespace facebook::fboss
