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

HwCpuFb303Stats::~HwCpuFb303Stats() {
  for (const auto& statNameAndStat : queueCounters_) {
    utility::deleteCounter(statNameAndStat.second.getName());
  }
}

std::string HwCpuFb303Stats::statName(
    folly::StringPiece statName,
    int queueId,
    folly::StringPiece queueName) {
  return folly::to<std::string>(
      "cpu.queue", queueId, ".cpuQueue-", queueName, ".", statName);
}

const stats::MonotonicCounter* HwCpuFb303Stats::getCounterIf(
    const std::string& statName) const {
  auto pcitr = queueCounters_.find(statName);
  return pcitr != queueCounters_.end() ? &pcitr->second : nullptr;
}

stats::MonotonicCounter* HwCpuFb303Stats::getCounterIf(
    const std::string& statName) {
  return const_cast<stats::MonotonicCounter*>(
      const_cast<const HwCpuFb303Stats*>(this)->getCounterIf(statName));
}

int64_t HwCpuFb303Stats::getCounterLastIncrement(
    folly::StringPiece statKey) const {
  return getCounterIf(statKey.str())->get();
}

void HwCpuFb303Stats::setupStats() {
  XLOG(DBG2) << "Initializing CPU stats";

  for (auto queueIdAndName : queueId2Name_) {
    for (auto statKey : kQueueStatKeys()) {
      auto newStatName =
          statName(statKey, queueIdAndName.first, queueIdAndName.second);
      reinitStat(newStatName, std::nullopt);
    }
  }
}

/*
 * Reinit port or port queue stat
 */
void HwCpuFb303Stats::reinitStat(
    const std::string& statName,
    std::optional<std::string> oldStatName) {
  if (oldStatName) {
    if (oldStatName == statName) {
      return;
    }
    auto stat = getCounterIf(*oldStatName);
    stats::MonotonicCounter newStat{statName, fb303::SUM, fb303::RATE};
    stat->swap(newStat);
    utility::deleteCounter(newStat.getName());
    queueCounters_.insert(std::make_pair(statName, std::move(*stat)));
    queueCounters_.erase(*oldStatName);
  } else {
    queueCounters_.emplace(
        statName, stats::MonotonicCounter(statName, fb303::SUM, fb303::RATE));
  }
}

void HwCpuFb303Stats::addOrUpdateQueue(
    int queueId,
    const std::string& queueName) {
  auto qitr = queueId2Name_.find(queueId);
  std::optional<std::string> oldQueueName = qitr == queueId2Name_.end()
      ? std::nullopt
      : std::optional<std::string>(qitr->second);
  queueId2Name_[queueId] = queueName;
  for (auto statKey : kQueueStatKeys()) {
    reinitStat(
        statName(statKey, queueId, queueName),
        oldQueueName ? std::optional<std::string>(
                           statName(statKey, queueId, *oldQueueName))
                     : std::nullopt);
  }
}

void HwCpuFb303Stats::removeQueue(int queueId) {
  auto qitr = queueId2Name_.find(queueId);
  for (auto statKey : kQueueStatKeys()) {
    auto stat =
        getCounterIf(statName(statKey, queueId, queueId2Name_[queueId]));
    utility::deleteCounter(stat->getName());
    queueCounters_.erase(stat->getName());
  }
  queueId2Name_.erase(queueId);
}

void HwCpuFb303Stats::updateStat(
    const std::chrono::seconds& now,
    const std::string& statName,
    int64_t val) {
  auto stat = getCounterIf(statName);
  CHECK(stat);
  stat->updateValue(now, val);
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
    CHECK(qitr != queueStats.end())
        << "Missing stat: " << statKey
        << " for queue: :" << queueId2Name_[queueId];
    updateStat(
        timeRetrieved_,
        statName(statKey, queueId, queueId2Name_[queueId]),
        qitr->second);
  };
  for (const auto& queueIdAndName : queueId2Name_) {
    updateQueueStat(
        kInDroppedPkts(),
        queueIdAndName.first,
        curPortStats.queueOutDiscardPackets_);
    updateQueueStat(
        kInPkts(), queueIdAndName.first, curPortStats.queueOutPackets_);
  }
}

} // namespace facebook::fboss
