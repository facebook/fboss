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

#include "fboss/agent/hw/StatsConstants.h"

#include <folly/logging/xlog.h>

namespace facebook::fboss {

std::array<folly::StringPiece, 23> HwPortFb303Stats::kPortStatKeys() {
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
      kWredDroppedPackets(),
      kOutEcnCounter(),
      kFecCorrectable(),
      kFecUncorrectable(),
  };
}

std::array<folly::StringPiece, 3> HwPortFb303Stats::kQueueStatKeys() {
  return {kOutCongestionDiscards(), kOutBytes(), kOutPkts()};
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

int64_t HwPortFb303Stats::getCounterLastIncrement(
    folly::StringPiece statKey) const {
  return portCounters_.getCounterLastIncrement(statKey.str());
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
      portCounters_.reinitStat(newStatName, oldStatName);
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
  portCounters_.reinitStat(
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
  portCounters_.reinitStat(
      statName(statKey, portName_, queueId, queueId2Name_[queueId]),
      oldQueueName ? std::optional<std::string>(
                         statName(statKey, portName_, queueId, *oldQueueName))
                   : std::nullopt);
}

void HwPortFb303Stats::queueChanged(int queueId, const std::string& queueName) {
  auto qitr = queueId2Name_.find(queueId);
  std::optional<std::string> oldQueueName = qitr == queueId2Name_.end()
      ? std::nullopt
      : std::optional<std::string>(qitr->second);
  queueId2Name_[queueId] = queueName;
  for (auto statKey : kQueueStatKeys()) {
    reinitStat(statKey, queueId, oldQueueName);
  }
}

void HwPortFb303Stats::queueRemoved(int queueId) {
  auto qitr = queueId2Name_.find(queueId);
  for (auto statKey : kQueueStatKeys()) {
    portCounters_.removeStat(
        statName(statKey, portName_, queueId, queueId2Name_[queueId]));
  }
  queueId2Name_.erase(queueId);
}

void HwPortFb303Stats::updateStats(
    const HwPortStats& curPortStats,
    const std::chrono::seconds& retrievedAt) {
  timeRetrieved_ = retrievedAt;
  updateStat(timeRetrieved_, kInBytes(), *curPortStats.inBytes__ref());
  updateStat(
      timeRetrieved_, kInUnicastPkts(), *curPortStats.inUnicastPkts__ref());
  updateStat(
      timeRetrieved_, kInMulticastPkts(), *curPortStats.inMulticastPkts__ref());
  updateStat(
      timeRetrieved_, kInBroadcastPkts(), *curPortStats.inBroadcastPkts__ref());
  updateStat(
      timeRetrieved_, kInDiscardsRaw(), *curPortStats.inDiscardsRaw__ref());
  updateStat(timeRetrieved_, kInDiscards(), *curPortStats.inDiscards__ref());
  updateStat(timeRetrieved_, kInErrors(), *curPortStats.inErrors__ref());
  updateStat(timeRetrieved_, kInPause(), *curPortStats.inPause__ref());
  updateStat(
      timeRetrieved_, kInIpv4HdrErrors(), *curPortStats.inIpv4HdrErrors__ref());
  updateStat(
      timeRetrieved_, kInIpv6HdrErrors(), *curPortStats.inIpv6HdrErrors__ref());
  updateStat(
      timeRetrieved_,
      kInDstNullDiscards(),
      *curPortStats.inDstNullDiscards__ref());
  // Egress Stats
  updateStat(timeRetrieved_, kOutBytes(), *curPortStats.outBytes__ref());
  updateStat(
      timeRetrieved_, kOutUnicastPkts(), *curPortStats.outUnicastPkts__ref());
  updateStat(
      timeRetrieved_,
      kOutMulticastPkts(),
      *curPortStats.outMulticastPkts__ref());
  updateStat(
      timeRetrieved_,
      kOutBroadcastPkts(),
      *curPortStats.outBroadcastPkts__ref());
  updateStat(timeRetrieved_, kOutDiscards(), *curPortStats.outDiscards__ref());
  updateStat(timeRetrieved_, kOutErrors(), *curPortStats.outErrors__ref());
  updateStat(timeRetrieved_, kOutPause(), *curPortStats.outPause__ref());
  updateStat(
      timeRetrieved_,
      kOutCongestionDiscards(),
      *curPortStats.outCongestionDiscardPkts__ref());
  updateStat(
      timeRetrieved_,
      kWredDroppedPackets(),
      *curPortStats.wredDroppedPackets__ref());
  updateStat(
      timeRetrieved_, kOutEcnCounter(), *curPortStats.outEcnCounter__ref());
  updateStat(
      timeRetrieved_,
      kFecCorrectable(),
      *curPortStats.fecCorrectableErrors_ref());
  updateStat(
      timeRetrieved_,
      kFecUncorrectable(),
      *curPortStats.fecUncorrectableErrors_ref());

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
        *curPortStats.queueOutDiscardBytes__ref());
    updateQueueStat(
        kOutBytes(), queueIdAndName.first, *curPortStats.queueOutBytes__ref());
    updateQueueStat(
        kOutPkts(), queueIdAndName.first, *curPortStats.queueOutPackets__ref());
  }
  updateQueueWatermarkStats(*curPortStats.queueWatermarkBytes__ref());
  portStats_ = curPortStats;
}

void HwPortFb303Stats::updateStat(
    const std::chrono::seconds& now,
    folly::StringPiece statKey,
    int queueId,
    int64_t val) {
  portCounters_.updateStat(
      now, statName(statKey, portName_, queueId, queueId2Name_[queueId]), val);
}

void HwPortFb303Stats::updateStat(
    const std::chrono::seconds& now,
    folly::StringPiece statKey,
    int64_t val) {
  portCounters_.updateStat(now, statName(statKey, portName_), val);
}
} // namespace facebook::fboss
