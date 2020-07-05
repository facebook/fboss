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

#include "fboss/agent/hw/CounterUtils.h"
#include "fboss/agent/hw/StatsConstants.h"

#include <folly/logging/xlog.h>

namespace facebook::fboss {

std::array<folly::StringPiece, 2> HwCpuFb303Stats::kQueueStatKeys() {
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
  return queueCounters_.getCounterLastIncrement(statKey.str());
}

void HwCpuFb303Stats::setupStats() {
  XLOG(DBG2) << "Initializing CPU stats";

  for (auto queueIdAndName : queueId2Name_) {
    for (auto statKey : kQueueStatKeys()) {
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
  for (auto statKey : kQueueStatKeys()) {
    queueCounters_.reinitStat(
        statName(statKey, queueId, queueName),
        oldQueueName ? std::optional<std::string>(
                           statName(statKey, queueId, *oldQueueName))
                     : std::nullopt);
  }
}

void HwCpuFb303Stats::queueRemoved(int queueId) {
  auto qitr = queueId2Name_.find(queueId);
  for (auto statKey : kQueueStatKeys()) {
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
        *curPortStats.queueOutDiscardPackets__ref());
    updateQueueStat(
        kInPkts(), queueIdAndName.first, *curPortStats.queueOutPackets__ref());
  }
}

} // namespace facebook::fboss
