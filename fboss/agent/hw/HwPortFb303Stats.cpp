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

#include <fb303/ServiceData.h>
#include <folly/logging/xlog.h>
#include <thrift/lib/cpp2/protocol/Serializer.h>

namespace facebook::fboss {

const std::vector<folly::StringPiece>&
HwPortFb303Stats::kPortMonotonicCounterStatKeys() const {
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
      kLeakyBucketFlapCnt(),
      kInLabelMissDiscards(),
      kInCongestionDiscards(),
      kInAclDiscards(),
      kInTrapDiscards(),
      kOutForwardingDiscards(),
      kPqpErrorEgressDroppedPackets(),
      kFabricLinkDownDroppedCells(),
  };
  return kPortKeys;
}

const std::vector<folly::StringPiece>&
HwPortFb303Stats::kPortFb303CounterStatKeys() const {
  static std::vector<folly::StringPiece> kPortKeys{
      kCableLengthMeters(),
      kDataCellsFilterOn(),
  };
  return kPortKeys;
}

const std::vector<folly::StringPiece>&
HwPortFb303Stats::kQueueMonotonicCounterStatKeys() const {
  static std::vector<folly::StringPiece> kQueueKeys{
      kOutCongestionDiscardsBytes(),
      kOutCongestionDiscards(),
      kOutBytes(),
      kOutPkts(),
      kWredDroppedPackets(),
      kOutEcnCounter(),
  };
  return kQueueKeys;
}

const std::vector<folly::StringPiece>&
HwPortFb303Stats::kInMacsecPortMonotonicCounterStatKeys() const {
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
      kInMacsecCurrentXpn(),
  };
  return kMacsecInKeys;
}

const std::vector<folly::StringPiece>&
HwPortFb303Stats::kOutMacsecPortMonotonicCounterStatKeys() const {
  static std::vector<folly::StringPiece> kMacsecOutKeys{
      kOutPreMacsecDropPkts(),
      kOutMacsecControlPkts(),
      kOutMacsecDataPkts(),
      kOutMacsecEncryptedBytes(),
      kOutMacsecTooLongDroppedPkts(),
      kOutMacsecUntaggedPkts(),
      kOutMacsecCurrentXpn(),
  };
  return kMacsecOutKeys;
}

const std::vector<folly::StringPiece>&
HwPortFb303Stats::kPfcMonotonicCounterStatKeys() const {
  static std::vector<folly::StringPiece> kPfcKeys{
      kInPfc(),
      kInPfcXon(),
      kOutPfc(),
  };
  return kPfcKeys;
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
  if (curPortStats.leakyBucketFlapCount_().has_value()) {
    updateStat(
        timeRetrieved_,
        kLeakyBucketFlapCnt(),
        *curPortStats.leakyBucketFlapCount_());
  }
  updateStat(
      timeRetrieved_,
      kInLabelMissDiscards(),
      *curPortStats.inLabelMissDiscards_());
  updateStat(
      timeRetrieved_,
      kInCongestionDiscards(),
      *curPortStats.inCongestionDiscards_());
  if (curPortStats.inAclDiscards_().has_value()) {
    updateStat(
        timeRetrieved_, kInAclDiscards(), *curPortStats.inAclDiscards_());
  }
  if (curPortStats.inTrapDiscards_().has_value()) {
    updateStat(
        timeRetrieved_, kInTrapDiscards(), *curPortStats.inTrapDiscards_());
  }
  if (curPortStats.outForwardingDiscards_().has_value()) {
    updateStat(
        timeRetrieved_,
        kOutForwardingDiscards(),
        *curPortStats.outForwardingDiscards_());
  }
  if (curPortStats.pqpErrorEgressDroppedPackets_().has_value()) {
    updateStat(
        timeRetrieved_,
        kPqpErrorEgressDroppedPackets(),
        *curPortStats.pqpErrorEgressDroppedPackets_());
  }
  if (curPortStats.fabricLinkDownDroppedCells_().has_value()) {
    updateStat(
        timeRetrieved_,
        kFabricLinkDownDroppedCells(),
        *curPortStats.fabricLinkDownDroppedCells_());
  }
  // Set fb303 counter stats
  if (curPortStats.cableLengthMeters().has_value() &&
      curPortStats.cableLengthMeters() !=
          std::numeric_limits<uint32_t>::max()) {
    fb303::fbData->setCounter(
        statName(kCableLengthMeters(), portName()),
        *curPortStats.cableLengthMeters());
  }
  if (curPortStats.dataCellsFilterOn().has_value()) {
    fb303::fbData->setCounter(
        statName(kDataCellsFilterOn(), portName()),
        *curPortStats.dataCellsFilterOn() ? 1 : 0);
  }

  // Update queue stats
  auto updateQueueStat = [this](
                             folly::StringPiece statKey,
                             int queueId,
                             const std::map<int16_t, int64_t>& queueStats) {
    auto qitr = queueStats.find(queueId);
    /*
     * Not all queue stats are available on all ASICs. Hence the queue stats
     * maps are sparsely populated. So skip over keys that are not found.
     */
    if (qitr != queueStats.end()) {
      updateStat(timeRetrieved_, statKey, queueId, qitr->second);
    }
  };
  for (const auto& queueIdAndName : queueId2Name()) {
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
  CHECK(
      curPortStats.queueWatermarkBytes_()->empty() ||
      curPortStats.queueWatermarkLevel_()->empty())
      << "Expect only one of queue watermark bytes, level to be populated";
  if (curPortStats.queueWatermarkBytes_()->size()) {
    updateQueueWatermarkStats(*curPortStats.queueWatermarkBytes_());
  } else if (curPortStats.queueWatermarkLevel_()->size()) {
    updateQueueWatermarkStats(*curPortStats.queueWatermarkLevel_());
  }
  if (curPortStats.egressGvoqWatermarkBytes_()->size()) {
    updateEgressGvoqWatermarkStats(*curPortStats.egressGvoqWatermarkBytes_());
  }
  // Macsec stats
  if (curPortStats.macsecStats()) {
    if (!macsecStatsInited()) {
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
        updateStat(
            timeRetrieved_,
            kInMacsecCurrentXpn(),
            *macsecPortStats.inCurrentXpn());
      } else {
        updateStat(
            timeRetrieved_,
            kOutMacsecUntaggedPkts(),
            *macsecPortStats.noMacsecTagPkts());
        updateStat(
            timeRetrieved_,
            kOutMacsecTooLongDroppedPkts(),
            *macsecPortStats.outTooLongDroppedPkts());
        updateStat(
            timeRetrieved_,
            kOutMacsecCurrentXpn(),
            *macsecPortStats.outCurrentXpn());
      }
    };
    updateMacsecPortStats(
        *curPortStats.macsecStats()->ingressPortStats(), true);
    updateMacsecPortStats(
        *curPortStats.macsecStats()->egressPortStats(), false);
  }

  // PFC stats
  auto updatePfcStat = [this](
                           folly::StringPiece statKey,
                           PfcPriority priority,
                           const std::map<int16_t, int64_t>& pfcStats,
                           int64_t* counter) {
    auto pitr = pfcStats.find(priority);
    if (pitr != pfcStats.end()) {
      updateStat(timeRetrieved_, statKey, priority, pitr->second);
      if (counter) {
        *counter += pitr->second;
      }
    }
  };
  int64_t inPfc = 0, outPfc = 0;
  for (auto priority : getEnabledPfcPriorities()) {
    updatePfcStat(kInPfc(), priority, *curPortStats.inPfc_(), &inPfc);
    updatePfcStat(
        kInPfcXon(), priority, *curPortStats.inPfcXon_(), std::nullptr_t());
    updatePfcStat(kOutPfc(), priority, *curPortStats.outPfc_(), &outPfc);
  }
  if (getEnabledPfcPriorities().size()) {
    updateStat(timeRetrieved_, kInPfc(), inPfc);
    updateStat(timeRetrieved_, kOutPfc(), outPfc);
  }

  portStats_ = curPortStats;
}

} // namespace facebook::fboss
