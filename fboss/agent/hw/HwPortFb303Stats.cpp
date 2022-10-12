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

std::array<folly::StringPiece, 6> HwPortFb303Stats::kQueueStatKeys() {
  return {
      kOutCongestionDiscardsBytes(),
      kOutCongestionDiscards(),
      kOutBytes(),
      kOutPkts(),
      kWredDroppedPackets(),
      kOutEcnCounter()};
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
  updateStat(timeRetrieved_, kInBytes(), *curPortStats.inBytes_());
  updateStat(timeRetrieved_, kInUnicastPkts(), *curPortStats.inUnicastPkts_());
  updateStat(
      timeRetrieved_, kInMulticastPkts(), *curPortStats.inMulticastPkts_());
  updateStat(
      timeRetrieved_, kInBroadcastPkts(), *curPortStats.inBroadcastPkts_());
  updateStat(timeRetrieved_, kInDiscardsRaw(), *curPortStats.inDiscardsRaw_());
  updateStat(timeRetrieved_, kInDiscards(), *curPortStats.inDiscards_());
  updateStat(timeRetrieved_, kInErrors(), *curPortStats.inErrors_());
  updateStat(timeRetrieved_, kInPause(), *curPortStats.inPause_());
  updateStat(
      timeRetrieved_, kInIpv4HdrErrors(), *curPortStats.inIpv4HdrErrors_());
  updateStat(
      timeRetrieved_, kInIpv6HdrErrors(), *curPortStats.inIpv6HdrErrors_());
  updateStat(
      timeRetrieved_, kInDstNullDiscards(), *curPortStats.inDstNullDiscards_());
  // Egress Stats
  updateStat(timeRetrieved_, kOutBytes(), *curPortStats.outBytes_());
  updateStat(
      timeRetrieved_, kOutUnicastPkts(), *curPortStats.outUnicastPkts_());
  updateStat(
      timeRetrieved_, kOutMulticastPkts(), *curPortStats.outMulticastPkts_());
  updateStat(
      timeRetrieved_, kOutBroadcastPkts(), *curPortStats.outBroadcastPkts_());
  updateStat(timeRetrieved_, kOutDiscards(), *curPortStats.outDiscards_());
  updateStat(timeRetrieved_, kOutErrors(), *curPortStats.outErrors_());
  updateStat(timeRetrieved_, kOutPause(), *curPortStats.outPause_());
  updateStat(
      timeRetrieved_,
      kOutCongestionDiscards(),
      *curPortStats.outCongestionDiscardPkts_());
  updateStat(
      timeRetrieved_,
      kWredDroppedPackets(),
      *curPortStats.wredDroppedPackets_());
  updateStat(timeRetrieved_, kOutEcnCounter(), *curPortStats.outEcnCounter_());
  updateStat(
      timeRetrieved_, kFecCorrectable(), *curPortStats.fecCorrectableErrors());
  updateStat(
      timeRetrieved_,
      kFecUncorrectable(),
      *curPortStats.fecUncorrectableErrors());
  updateStat(
      timeRetrieved_,
      kInLabelMissDiscards(),
      *curPortStats.inLabelMissDiscards_());

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
        *curPortStats.queueOutDiscardBytes_());
    updateQueueStat(
        kOutCongestionDiscards(),
        queueIdAndName.first,
        *curPortStats.queueOutDiscardPackets_());
    updateQueueStat(
        kOutBytes(), queueIdAndName.first, *curPortStats.queueOutBytes_());
    updateQueueStat(
        kOutPkts(), queueIdAndName.first, *curPortStats.queueOutPackets_());
    if (curPortStats.queueWredDroppedPackets_()->size()) {
      updateQueueStat(
          kWredDroppedPackets(),
          queueIdAndName.first,
          *curPortStats.queueWredDroppedPackets_());
    }
    if (curPortStats.queueEcnMarkedPackets_()->size()) {
      updateQueueStat(
          kOutEcnCounter(),
          queueIdAndName.first,
          *curPortStats.queueEcnMarkedPackets_());
    }
  }
  if (curPortStats.queueWatermarkBytes_()->size()) {
    updateQueueWatermarkStats(*curPortStats.queueWatermarkBytes_());
  }
  // Macsec stats
  if (curPortStats.macsecStats()) {
    if (!macsecStatsInited_) {
      reinitMacsecStats(std::nullopt);
    }
    auto updateMacsecPortStats = [this](auto& macsecPortStats, bool ingress) {
      updateStat(
          timeRetrieved_,
          ingress ? kInPreMacsecDropPkts() : kOutPreMacsecDropPkts(),
          *macsecPortStats.preMacsecDropPkts());
      updateStat(
          timeRetrieved_,
          ingress ? kInMacsecDataPkts() : kOutMacsecDataPkts(),
          *macsecPortStats.dataPkts());
      updateStat(
          timeRetrieved_,
          ingress ? kInMacsecControlPkts() : kOutMacsecControlPkts(),
          *macsecPortStats.controlPkts());
      updateStat(
          timeRetrieved_,
          ingress ? kInMacsecDecryptedBytes() : kOutMacsecEncryptedBytes(),
          *macsecPortStats.octetsEncrypted());
      if (ingress) {
        updateStat(
            timeRetrieved_,
            kInMacsecBadOrNoTagDroppedPkts(),
            *macsecPortStats.inBadOrNoMacsecTagDroppedPkts());
        updateStat(
            timeRetrieved_,
            kInMacsecNoSciDroppedPkts(),
            *macsecPortStats.inNoSciDroppedPkts());
        updateStat(
            timeRetrieved_,
            kInMacsecUnknownSciPkts(),
            *macsecPortStats.inUnknownSciPkts());
        updateStat(
            timeRetrieved_,
            kInMacsecOverrunDroppedPkts(),
            *macsecPortStats.inOverrunDroppedPkts());
        updateStat(
            timeRetrieved_,
            kInMacsecDelayedPkts(),
            *macsecPortStats.inDelayedPkts());
        updateStat(
            timeRetrieved_,
            kInMacsecLateDroppedPkts(),
            *macsecPortStats.inLateDroppedPkts());
        updateStat(
            timeRetrieved_,
            kInMacsecNotValidDroppedPkts(),
            *macsecPortStats.inNotValidDroppedPkts());
        updateStat(
            timeRetrieved_,
            kInMacsecInvalidPkts(),
            *macsecPortStats.inInvalidPkts());
        updateStat(
            timeRetrieved_,
            kInMacsecNoSADroppedPkts(),
            *macsecPortStats.inNoSaDroppedPkts());
        updateStat(
            timeRetrieved_,
            kInMacsecUnusedSAPkts(),
            *macsecPortStats.inUnusedSaPkts());
        updateStat(
            timeRetrieved_,
            kInMacsecUntaggedPkts(),
            *macsecPortStats.noMacsecTagPkts());
      } else {
        updateStat(
            timeRetrieved_,
            kOutMacsecUntaggedPkts(),
            *macsecPortStats.noMacsecTagPkts());
        updateStat(
            timeRetrieved_,
            kOutMacsecTooLongDroppedPkts(),
            *macsecPortStats.outTooLongDroppedPkts());
      }
    };
    updateMacsecPortStats(
        *curPortStats.macsecStats()->ingressPortStats(), true);
    updateMacsecPortStats(
        *curPortStats.macsecStats()->egressPortStats(), false);
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
