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

std::array<folly::StringPiece, 24> HwPortFb303Stats::kPortStatKeys() {
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
      kInLabelMissDiscards(),
  };
}

std::array<folly::StringPiece, 4> HwPortFb303Stats::kQueueStatKeys() {
  return {
      kOutCongestionDiscardsBytes(),
      kOutCongestionDiscards(),
      kOutBytes(),
      kOutPkts()};
}

std::array<folly::StringPiece, 15> HwPortFb303Stats::kInMacsecPortStatKeys() {
  return {
      kInPreMacsecDropPkts(),
      kInMacsecControlPkts(),
      kInMacsecDataPkts(),
      kInMacsecDecryptedBytes(),
      kInMacsecBadOrNoTagDroppedPkts(),
      kInMacsecNoSciDroppedPkts(),
      kInMacsecUnknownSciPkts(),
      kInMacsecOverrunDroppedPkts(),
      kInMacsecDelayedPkts(),
      kInMacsecLateDroppedPkts(),
      kInMacsecNotValidDroppedPkts(),
      kInMacsecInvalidPkts(),
      kInMacsecNoSADroppedPkts(),
      kInMacsecUnusedSAPkts(),
      kInMacsecUntaggedPkts(),
  };
}

std::array<folly::StringPiece, 6> HwPortFb303Stats::kOutMacsecPortStatKeys() {
  return {
      kOutPreMacsecDropPkts(),
      kOutMacsecControlPkts(),
      kOutMacsecDataPkts(),
      kOutMacsecEncryptedBytes(),
      kOutMacsecTooLongDroppedPkts(),
      kOutMacsecUntaggedPkts(),
  };
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
  if (macsecStatsInited_) {
    reinitMacsecStats(oldPortName);
  }
}

/*
 * Reinit macsec stats
 */
void HwPortFb303Stats::reinitMacsecStats(
    std::optional<std::string> oldPortName) {
  auto reinitStats = [this, &oldPortName](const auto& keys) {
    for (auto statKey : keys) {
      reinitStat(statKey, portName_, oldPortName);
    }
  };
  reinitStats(kInMacsecPortStatKeys());
  reinitStats(kOutMacsecPortStatKeys());

  macsecStatsInited_ = true;
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
  updateStat(
      timeRetrieved_,
      kInLabelMissDiscards(),
      *curPortStats.inLabelMissDiscards__ref());

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
        kOutCongestionDiscardsBytes(),
        queueIdAndName.first,
        *curPortStats.queueOutDiscardBytes__ref());
    updateQueueStat(
        kOutCongestionDiscards(),
        queueIdAndName.first,
        *curPortStats.queueOutDiscardPackets__ref());
    updateQueueStat(
        kOutBytes(), queueIdAndName.first, *curPortStats.queueOutBytes__ref());
    updateQueueStat(
        kOutPkts(), queueIdAndName.first, *curPortStats.queueOutPackets__ref());
  }
  if (curPortStats.queueWatermarkBytes__ref()->size()) {
    updateQueueWatermarkStats(*curPortStats.queueWatermarkBytes__ref());
  }
  // Macsec stats
  if (curPortStats.macsecStats_ref()) {
    if (!macsecStatsInited_) {
      reinitMacsecStats(std::nullopt);
    }
    auto updateMacsecPortStats = [this](auto& macsecPortStats, bool ingress) {
      updateStat(
          timeRetrieved_,
          ingress ? kInPreMacsecDropPkts() : kOutPreMacsecDropPkts(),
          *macsecPortStats.preMacsecDropPkts_ref());
      updateStat(
          timeRetrieved_,
          ingress ? kInMacsecDataPkts() : kOutMacsecDataPkts(),
          *macsecPortStats.dataPkts_ref());
      updateStat(
          timeRetrieved_,
          ingress ? kInMacsecControlPkts() : kOutMacsecControlPkts(),
          *macsecPortStats.controlPkts_ref());
      updateStat(
          timeRetrieved_,
          ingress ? kInMacsecDecryptedBytes() : kOutMacsecEncryptedBytes(),
          *macsecPortStats.octetsEncrypted_ref());
      if (ingress) {
        updateStat(
            timeRetrieved_,
            kInMacsecBadOrNoTagDroppedPkts(),
            *macsecPortStats.inBadOrNoMacsecTagDroppedPkts_ref());
        updateStat(
            timeRetrieved_,
            kInMacsecNoSciDroppedPkts(),
            *macsecPortStats.inNoSciDroppedPkts_ref());
        updateStat(
            timeRetrieved_,
            kInMacsecUnknownSciPkts(),
            *macsecPortStats.inUnknownSciPkts_ref());
        updateStat(
            timeRetrieved_,
            kInMacsecOverrunDroppedPkts(),
            *macsecPortStats.inOverrunDroppedPkts_ref());
        updateStat(
            timeRetrieved_,
            kInMacsecDelayedPkts(),
            *macsecPortStats.inDelayedPkts_ref());
        updateStat(
            timeRetrieved_,
            kInMacsecLateDroppedPkts(),
            *macsecPortStats.inLateDroppedPkts_ref());
        updateStat(
            timeRetrieved_,
            kInMacsecNotValidDroppedPkts(),
            *macsecPortStats.inNotValidDroppedPkts_ref());
        updateStat(
            timeRetrieved_,
            kInMacsecInvalidPkts(),
            *macsecPortStats.inInvalidPkts_ref());
        updateStat(
            timeRetrieved_,
            kInMacsecNoSADroppedPkts(),
            *macsecPortStats.inNoSaDroppedPkts_ref());
        updateStat(
            timeRetrieved_,
            kInMacsecUnusedSAPkts(),
            *macsecPortStats.inUnusedSaPkts_ref());
        updateStat(
            timeRetrieved_,
            kInMacsecUntaggedPkts(),
            *macsecPortStats.noMacsecTagPkts_ref());
      } else {
        updateStat(
            timeRetrieved_,
            kOutMacsecUntaggedPkts(),
            *macsecPortStats.noMacsecTagPkts_ref());
        updateStat(
            timeRetrieved_,
            kOutMacsecTooLongDroppedPkts(),
            *macsecPortStats.outTooLongDroppedPkts_ref());
      }
    };
    updateMacsecPortStats(
        *curPortStats.macsecStats_ref()->ingressPortStats_ref(), true);
    updateMacsecPortStats(
        *curPortStats.macsecStats_ref()->egressPortStats_ref(), false);
  }
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
