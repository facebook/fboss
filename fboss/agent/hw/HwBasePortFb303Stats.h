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
#include "fboss/agent/types.h"

#include "folly/container/F14Map.h"

#include <optional>
#include <string>

namespace facebook::fboss {

class HwBasePortFb303Stats {
 public:
  using QueueId2Name = folly::F14FastMap<int, std::string>;
  explicit HwBasePortFb303Stats(
      const std::string& portName,
      QueueId2Name queueId2Name = {},
      std::vector<PfcPriority> enabledPfcPriorities = {},
      std::optional<std::string> multiSwitchStatsPrefix = std::nullopt)
      : portName_(portName),
        portCounters_(HwFb303Stats(multiSwitchStatsPrefix)),
        queueId2Name_(queueId2Name),
        enabledPfcPriorities_(enabledPfcPriorities) {}

  virtual ~HwBasePortFb303Stats() = default;

  const std::string& portName() const {
    return portName_;
  }

  virtual void portNameChanged(const std::string& newName) {
    auto oldPortName = portName_;
    portName_ = newName;
    reinitStats(oldPortName);
  }
  void queueChanged(int queueId, const std::string& queueName);
  void queueRemoved(int queueId);
  void pfcPriorityChanged(std::vector<PfcPriority> enabledPriorities);
  void updateLeakyBucketFlapCnt(int cnt);

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

  /*
   * Port PFC stat name
   */
  static std::string statName(
      folly::StringPiece statName,
      folly::StringPiece portName,
      PfcPriority priority);

  int64_t getCounterLastIncrement(
      folly::StringPiece statKey,
      std::optional<int64_t> defaultVal = std::nullopt) const;

  virtual const std::vector<folly::StringPiece>& kPortMonotonicCounterStatKeys()
      const = 0;
  virtual const std::vector<folly::StringPiece>& kPortFb303CounterStatKeys()
      const = 0;
  virtual const std::vector<folly::StringPiece>&
  kQueueMonotonicCounterStatKeys() const = 0;
  virtual const std::vector<folly::StringPiece>&
  kInMacsecPortMonotonicCounterStatKeys() const = 0;
  virtual const std::vector<folly::StringPiece>&
  kOutMacsecPortMonotonicCounterStatKeys() const = 0;
  virtual const std::vector<folly::StringPiece>& kPfcMonotonicCounterStatKeys()
      const = 0;

 protected:
  void reinitStats(std::optional<std::string> oldPortName);
  void reinitMacsecStats(std::optional<std::string> oldPortName);
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
   * update port PFC stat
   */
  void updateStat(
      const std::chrono::seconds& now,
      folly::StringPiece statKey,
      PfcPriority priority,
      int64_t val);

  void updateQueueWatermarkStats(
      const std::map<int16_t, int64_t>& queueWatermarkBytes) const;

  void updateEgressGvoqWatermarkStats(
      const std::map<int16_t, int64_t>& gvoqWatermarks) const;

  bool macsecStatsInited() const {
    return macsecStatsInited_;
  }
  const QueueId2Name& queueId2Name() const {
    return queueId2Name_;
  }
  const std::vector<PfcPriority> getEnabledPfcPriorities() const {
    return enabledPfcPriorities_;
  }

 private:
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

  std::string portName_;
  HwFb303Stats portCounters_;
  QueueId2Name queueId2Name_;
  bool macsecStatsInited_{false};
  std::vector<PfcPriority> enabledPfcPriorities_{};
};

} // namespace facebook::fboss
