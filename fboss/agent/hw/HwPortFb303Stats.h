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

#include <optional>
#include <string>

namespace facebook::fboss {

class HwPortFb303Stats {
 public:
  using QueueId2Name = folly::F14FastMap<int, std::string>;
  explicit HwPortFb303Stats(
      const std::string& portName,
      QueueId2Name queueId2Name = {})
      : portName_(portName), queueId2Name_(queueId2Name) {
    reinitStats(std::nullopt);
  }
  ~HwPortFb303Stats();

  void updateStats(
      const HwPortStats& latestStats,
      const std::chrono::seconds& retrievedAt);

  void portNameChanged(const std::string& newName) {
    auto oldPortName = portName_;
    portName_ = newName;
    reinitStats(oldPortName);
  }
  void addOrUpdateQueue(int queueId, const std::string& queueName);
  void removeQueue(int queueId);

  /*
   * Port stat name
   */
  static std::string statName(
      folly::StringPiece statName,
      folly::StringPiece portName);

  /*
   * Port queue stat name
   */
  static std::string statName(
      folly::StringPiece statName,
      folly::StringPiece portName,
      int queueId,
      folly::StringPiece queueName);

  static std::array<folly::StringPiece, 20> kPortStatKeys();
  static std::array<folly::StringPiece, 3> kQueueStatKeys();
  int64_t getCounterLastIncrement(folly::StringPiece statKey) const;

 private:
  void reinitStats(std::optional<std::string> oldPortName);
  int getQueueId(const std::string& queueName) const;
  /*
   * Reinit port stat
   */
  void reinitStat(
      folly::StringPiece statKey,
      const std::string& portName,
      std::optional<std::string> oldPortName);
  /*
   * Reinit port queue stat
   */
  void reinitStat(
      folly::StringPiece statKey,
      int queueId,
      std::optional<std::string> oldQueueName);

  /*
   * Reinit port or port queue stat
   */
  void reinitStat(
      const std::string& statName,
      std::optional<std::string> oldStatName);
  /*
   * update port stat
   */
  void updateStat(
      const std::chrono::seconds& now,
      folly::StringPiece statKey,
      int64_t val);
  /*
   * update port queue stat
   */
  void updateStat(
      const std::chrono::seconds& now,
      folly::StringPiece statKey,
      int queueId,
      int64_t val);
  /*
   * Update port or port queue stat
   */
  void updateStat(
      const std::chrono::seconds& now,
      const std::string& statName,
      int64_t val);
  stats::MonotonicCounter* getCounterIf(const std::string& statName);
  const stats::MonotonicCounter* getCounterIf(
      const std::string& statName) const;

  std::chrono::seconds timeRetrieved_{0};
  std::string portName_;
  folly::F14FastMap<std::string, stats::MonotonicCounter> portCounters_;
  QueueId2Name queueId2Name_;
};

} // namespace facebook::fboss
