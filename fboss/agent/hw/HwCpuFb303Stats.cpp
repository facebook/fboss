/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/HwCpuFb303Stats.h"

#include "fboss/agent/hw/StatsConstants.h"

#include <folly/logging/xlog.h>

namespace facebook::fboss {

std::array<folly::StringPiece, 2>
HwCpuFb303Stats::kQueueMonotonicCounterStatKeys() {
  return {kInPkts(), kInDroppedPkts()};
}

std::string HwCpuFb303Stats::statName(
    folly::StringPiece statName,
    int queueId,
    folly::StringPiece queueName) {
  return folly::to<std::string>(
      "cpu.queue", queueId, ".cpuQueue-", queueName, ".", statName);
}

int64_t HwCpuFb303Stats::getCounterLastIncrement(
    folly::StringPiece statKey) const {
  return queueCounters_.getCounterLastIncrement(statKey.str(), std::nullopt);
}

int64_t HwCpuFb303Stats::getCounter(const folly::StringPiece statName) const {
  return queueCounters_.getCumulativeValueIf(statName.str());
}

void HwCpuFb303Stats::setupStats() {
  XLOG(DBG2) << "Initializing CPU stats";

  for (auto queueIdAndName : queueId2Name_) {
    for (auto statKey : kQueueMonotonicCounterStatKeys()) {
      auto newStatName =
          statName(statKey, queueIdAndName.first, queueIdAndName.second);
      queueCounters_.reinitStat(newStatName, std::nullopt);
    }
  }
}

void HwCpuFb303Stats::queueChanged(int queueId, const std::string& queueName) {
  auto qitr = queueId2Name_.find(queueId);
  std::optional<std::string> oldQueueName = qitr == queueId2Name_.end()
      ? std::nullopt
      : std::optional<std::string>(qitr->second);
  queueId2Name_[queueId] = queueName;
  for (auto statKey : kQueueMonotonicCounterStatKeys()) {
    queueCounters_.reinitStat(
        statName(statKey, queueId, queueName),
        oldQueueName ? std::optional<std::string>(
                           statName(statKey, queueId, *oldQueueName))
                     : std::nullopt);
  }
}

void HwCpuFb303Stats::queueRemoved(int queueId) {
  for (auto statKey : kQueueMonotonicCounterStatKeys()) {
    queueCounters_.removeStat(
        statName(statKey, queueId, queueId2Name_[queueId]));
  }
  queueId2Name_.erase(queueId);
}

void HwCpuFb303Stats::updateStats(
    const HwPortStats& curPortStats,
    const std::chrono::seconds& retrievedAt) {
  timeRetrieved_ = retrievedAt;
  // Update queue stats
  auto updateQueueStat = [this](
                             folly::StringPiece statKey,
                             int queueId,
                             const std::map<int16_t, int64_t>& queueStats) {
    auto qitr = queueStats.find(queueId);
    if (qitr == queueStats.end()) {
      // may not update stats for every queue but only those that application
      // cares about.
      return;
    }
    queueCounters_.updateStat(
        timeRetrieved_,
        statName(statKey, queueId, queueId2Name_[queueId]),
        qitr->second);
  };
  for (const auto& queueIdAndName : queueId2Name_) {
    updateQueueStat(
        kInDroppedPkts(),
        queueIdAndName.first,
        *curPortStats.queueOutDiscardPackets_());
    updateQueueStat(
        kInPkts(), queueIdAndName.first, *curPortStats.queueOutPackets_());
  }
}

CpuPortStats HwCpuFb303Stats::getCpuPortStats() const {
  CpuPortStats cpuPortStats;
  int64_t ingressPackets = 0;
  int64_t discardPackets = 0;

  cpuPortStats.queueToName_()->insert(
      queueId2Name_.begin(), queueId2Name_.end());
  for (const auto& queueIdAndName : queueId2Name_) {
    ingressPackets = getCounter(
        statName(kInPkts(), queueIdAndName.first, queueIdAndName.second));
    cpuPortStats.queueInPackets_()->emplace(
        queueIdAndName.first, ingressPackets);

    discardPackets = getCounter(statName(
        kInDroppedPkts(), queueIdAndName.first, queueIdAndName.second));

    cpuPortStats.queueDiscardPackets_()->emplace(
        queueIdAndName.first, discardPackets);
  }
  return cpuPortStats;
}

void HwCpuFb303Stats::updateStats(const CpuPortStats& curPortStats) {
  for (const auto& [queueId, queueName] : *curPortStats.queueToName_()) {
    auto oldQueueName = queueId2Name_.find(queueId);
    if (oldQueueName == queueId2Name_.end() ||
        queueName != oldQueueName->second) {
      queueChanged(queueId, queueName);
    }
    queueCounters_.updateStat(
        timeRetrieved_,
        statName(kInDroppedPkts(), queueId, queueId2Name_[queueId]),
        curPortStats.queueDiscardPackets_()->at(queueId));
    queueCounters_.updateStat(
        timeRetrieved_,
        statName(kInPkts(), queueId, queueId2Name_[queueId]),
        curPortStats.queueInPackets_()->at(queueId));
  }
  for (auto iter = queueId2Name_.cbegin(); iter != queueId2Name_.cend();) {
    if (curPortStats.queueToName_()->find(iter->first) ==
        curPortStats.queueToName_()->end()) {
      auto queueId = iter->first;
      iter++;
      queueRemoved(queueId);
    } else {
      ++iter;
    }
  }
}

int64_t HwCpuFb303Stats::getCumulativeValueIf(
    const folly::StringPiece statKey) const {
  return queueCounters_.getCumulativeValueIf(statKey.str());
}

} // namespace facebook::fboss
