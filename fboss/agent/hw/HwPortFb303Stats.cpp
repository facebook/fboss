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

const std::vector<folly::StringPiece>& HwPortFb303Stats::kPortStatKeys() const {
  static std::vector<folly::StringPiece> kPortKeys{
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
  return kPortKeys;
}

const std::vector<folly::StringPiece>& HwPortFb303Stats::kQueueStatKeys()
    const {
  static std::vector<folly::StringPiece> kQueueKeys{
      kOutCongestionDiscardsBytes(),
      kOutCongestionDiscards(),
      kOutBytes(),
      kOutPkts(),
      kWredDroppedPackets(),
      kOutEcnCounter()};
  return kQueueKeys;
}

const std::vector<folly::StringPiece>& HwPortFb303Stats::kInMacsecPortStatKeys()
    const {
  static std::vector<folly::StringPiece> kMacsecInKeys{
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
  return kMacsecInKeys;
}

const std::vector<folly::StringPiece>&
HwPortFb303Stats::kOutMacsecPortStatKeys() const {
  static std::vector<folly::StringPiece> kMacsecOutKeys{
      kOutPreMacsecDropPkts(),
      kOutMacsecControlPkts(),
      kOutMacsecDataPkts(),
      kOutMacsecEncryptedBytes(),
      kOutMacsecTooLongDroppedPkts(),
      kOutMacsecUntaggedPkts(),
  };
  return kMacsecOutKeys;
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
    /*
     * TODO(skhare) Some ASICs don't yet support querying queue stats, so skip
     * updateStat. Once that support is added, ASSERT for queue stat to be
     * present.
     */
    if (qitr != queueStats.end()) {
      updateStat(timeRetrieved_, statKey, queueId, qitr->second);
    }
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

} // namespace facebook::fboss
