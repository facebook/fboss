/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/HwPortFb303Stats.h"

#include "fboss/agent/hw/CounterUtils.h"
#include "fboss/agent/hw/StatsConstants.h"

#include <folly/logging/xlog.h>

namespace facebook::fboss {

std::array<folly::StringPiece, 20> HwPortFb303Stats::kPortStatKeys() {
  return {
      kInBytes(),
      kInUnicastPkts(),
      kInMulticastPkts(),
      kInBroadcastPkts(),
      kInDiscards(),
      kInErrors(),
      kInPause(),
      kInIpv4HdrErrors(),
      kInIpv6HdrErrors(),
      kInDstNullDiscards(),
      kInDiscardsRaw(),
      kOutBytes(),
      kOutUnicastPkts(),
      kOutMulticastPkts(),
      kOutBroadcastPkts(),
      kOutDiscards(),
      kOutErrors(),
      kOutPause(),
      kOutCongestionDiscards(),
      kOutEcnCounter(),
  };
}

std::array<folly::StringPiece, 3> HwPortFb303Stats::kQueueStatKeys() {
  return {kOutCongestionDiscards(), kOutBytes(), kOutPkts()};
}

HwPortFb303Stats::~HwPortFb303Stats() {
  for (const auto& statNameAndStat : portCounters_) {
    utility::deleteCounter(statNameAndStat.second.getName());
  }
}

std::string HwPortFb303Stats::statName(
    folly::StringPiece statName,
    folly::StringPiece portName) {
  return folly::to<std::string>(portName, ".", statName);
}

std::string HwPortFb303Stats::statName(
    folly::StringPiece statName,
    folly::StringPiece portName,
    int queueId,
    folly::StringPiece queueName) {
  return folly::to<std::string>(
      portName, ".", "queue", queueId, ".", queueName, ".", statName);
}

const stats::MonotonicCounter* HwPortFb303Stats::getCounterIf(
    const std::string& statName) const {
  auto pcitr = portCounters_.find(statName);
  return pcitr != portCounters_.end() ? &pcitr->second : nullptr;
}

stats::MonotonicCounter* HwPortFb303Stats::getCounterIf(
    const std::string& statName) {
  return const_cast<stats::MonotonicCounter*>(
      const_cast<const HwPortFb303Stats*>(this)->getCounterIf(statName));
}

int64_t HwPortFb303Stats::getCounterLastIncrement(
    folly::StringPiece statKey) const {
  return getCounterIf(statKey.str())->get();
}

void HwPortFb303Stats::reinitStats(std::optional<std::string> oldPortName) {
  XLOG(DBG2) << "Reinitializing stats for " << portName_;

  for (auto statKey : kPortStatKeys()) {
    reinitStat(statKey, portName_, oldPortName);
  }
  for (auto queueIdAndName : queueId2Name_) {
    for (auto statKey : kQueueStatKeys()) {
      auto newStatName = statName(
          statKey, portName_, queueIdAndName.first, queueIdAndName.second);
      std::optional<std::string> oldStatName = oldPortName
          ? std::optional<std::string>(statName(
                statKey,
                *oldPortName,
                queueIdAndName.first,
                queueIdAndName.second))
          : std::nullopt;
      reinitStat(newStatName, oldStatName);
    }
  }
}

/*
 * Reinit port stat
 */
void HwPortFb303Stats::reinitStat(
    folly::StringPiece statKey,
    const std::string& portName,
    std::optional<std::string> oldPortName) {
  reinitStat(
      statName(statKey, portName),
      oldPortName ? std::optional<std::string>(statName(statKey, *oldPortName))
                  : std::nullopt);
}

/*
 * Reinit port queue stat
 */
void HwPortFb303Stats::reinitStat(
    folly::StringPiece statKey,
    int queueId,
    std::optional<std::string> oldQueueName) {
  reinitStat(
      statName(statKey, portName_, queueId, queueId2Name_[queueId]),
      oldQueueName ? std::optional<std::string>(
                         statName(statKey, portName_, queueId, *oldQueueName))
                   : std::nullopt);
}

/*
 * Reinit port or port queue stat
 */
void HwPortFb303Stats::reinitStat(
    const std::string& statName,
    std::optional<std::string> oldStatName) {
  if (oldStatName) {
    auto stat = getCounterIf(*oldStatName);
    stats::MonotonicCounter newStat{statName, fb303::SUM, fb303::RATE};
    stat->swap(newStat);
    utility::deleteCounter(newStat.getName());
  } else {
    portCounters_.emplace(
        statName, stats::MonotonicCounter(statName, fb303::SUM, fb303::RATE));
  }
}

void HwPortFb303Stats::addOrUpdateQueue(
    int queueId,
    const std::string& queueName) {
  auto qitr = queueId2Name_.find(queueId);
  std::optional<std::string> oldQueueName = qitr == queueId2Name_.end()
      ? std::nullopt
      : std::optional<std::string>(qitr->second);
  queueId2Name_[queueId] = queueName;
  for (auto statKey : kQueueStatKeys()) {
    reinitStat(statKey, queueId, oldQueueName);
  }
}

void HwPortFb303Stats::removeQueue(int queueId) {
  auto qitr = queueId2Name_.find(queueId);
  for (auto statKey : kQueueStatKeys()) {
    auto stat = getCounterIf(
        statName(statKey, portName_, queueId, queueId2Name_[queueId]));
    utility::deleteCounter(stat->getName());
    portCounters_.erase(stat->getName());
  }
  queueId2Name_.erase(queueId);
}

void HwPortFb303Stats::updateStat(
    const std::chrono::seconds& now,
    const std::string& statName,
    int64_t val) {
  auto stat = getCounterIf(statName);
  stat->updateValue(now, val);
}

void HwPortFb303Stats::updateStats(
    const HwPortStats& curPortStats,
    const std::chrono::seconds& retrievedAt) {
  timeRetrieved_ = retrievedAt;
  updateStat(timeRetrieved_, kInBytes(), curPortStats.inBytes_);
  updateStat(timeRetrieved_, kInUnicastPkts(), curPortStats.inUnicastPkts_);
  updateStat(timeRetrieved_, kInMulticastPkts(), curPortStats.inMulticastPkts_);
  updateStat(timeRetrieved_, kInBroadcastPkts(), curPortStats.inBroadcastPkts_);
  updateStat(timeRetrieved_, kInDiscardsRaw(), curPortStats.inDiscardsRaw_);
  updateStat(timeRetrieved_, kInDiscards(), curPortStats.inDiscards_);
  updateStat(timeRetrieved_, kInErrors(), curPortStats.inErrors_);
  updateStat(timeRetrieved_, kInPause(), curPortStats.inPause_);
  updateStat(timeRetrieved_, kInIpv4HdrErrors(), curPortStats.inIpv4HdrErrors_);
  updateStat(timeRetrieved_, kInIpv6HdrErrors(), curPortStats.inIpv6HdrErrors_);
  updateStat(
      timeRetrieved_, kInDstNullDiscards(), curPortStats.inDstNullDiscards_);
  // Egress Stats
  updateStat(timeRetrieved_, kOutBytes(), curPortStats.outBytes_);
  updateStat(timeRetrieved_, kOutUnicastPkts(), curPortStats.outUnicastPkts_);
  updateStat(
      timeRetrieved_, kOutMulticastPkts(), curPortStats.outMulticastPkts_);
  updateStat(
      timeRetrieved_, kOutBroadcastPkts(), curPortStats.outBroadcastPkts_);
  updateStat(timeRetrieved_, kOutDiscards(), curPortStats.outDiscards_);
  updateStat(timeRetrieved_, kOutErrors(), curPortStats.outErrors_);
  updateStat(timeRetrieved_, kOutPause(), curPortStats.outPause_);
  updateStat(
      timeRetrieved_,
      kOutCongestionDiscards(),
      curPortStats.outCongestionDiscardPkts_);

  updateStat(timeRetrieved_, kOutEcnCounter(), curPortStats.outEcnCounter_);
  // Update queue stats
  auto updateQueueStat = [this](
                             folly::StringPiece statKey,
                             int queueId,
                             const std::map<int16_t, int64_t>& queueStats) {
    auto qitr = queueStats.find(queueId);
    CHECK(qitr != queueStats.end())
        << "Missing stat: " << statKey
        << " for queue: :" << queueId2Name_[queueId];
    updateStat(timeRetrieved_, statKey, queueId, qitr->second);
  };
  for (const auto& queueIdAndName : queueId2Name_) {
    updateQueueStat(
        kOutCongestionDiscards(),
        queueIdAndName.first,
        curPortStats.queueOutDiscardBytes_);
    updateQueueStat(
        kOutBytes(), queueIdAndName.first, curPortStats.queueOutBytes_);
    updateQueueStat(
        kOutPkts(), queueIdAndName.first, curPortStats.queueOutPackets_);
  }
}

void HwPortFb303Stats::updateStat(
    const std::chrono::seconds& now,
    folly::StringPiece statKey,
    int queueId,
    int64_t val) {
  updateStat(
      now, statName(statKey, portName_, queueId, queueId2Name_[queueId]), val);
}

void HwPortFb303Stats::updateStat(
    const std::chrono::seconds& now,
    folly::StringPiece statKey,
    int64_t val) {
  updateStat(now, statName(statKey, portName_), val);
}
} // namespace facebook::fboss
