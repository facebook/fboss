/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/HwSysPortFb303Stats.h"

#include "fboss/agent/hw/StatsConstants.h"

#include <folly/logging/xlog.h>
#include <thrift/lib/cpp2/protocol/Serializer.h>

namespace facebook::fboss {

const std::vector<folly::StringPiece>&
HwSysPortFb303Stats::kPortMonotonicCounterStatKeys() const {
  // No port level stats on sys ports
  static std::vector<folly::StringPiece> kPortKeys{};
  return kPortKeys;
}

const std::vector<folly::StringPiece>&
HwSysPortFb303Stats::kPortFb303CounterStatKeys() const {
  // No port level stats on sys ports
  static std::vector<folly::StringPiece> kPortKeys{};
  return kPortKeys;
}

const std::vector<folly::StringPiece>&
HwSysPortFb303Stats::kQueueMonotonicCounterStatKeys() const {
  static std::vector<folly::StringPiece> kQueueKeys{
      kOutDiscards(),
      kOutBytes(),
      kWredDroppedPackets(),
      kCreditWatchdogDeletedPackets()};
  return kQueueKeys;
}

const std::vector<folly::StringPiece>&
HwSysPortFb303Stats::kQueueFb303CounterStatKeys() const {
  static std::vector<folly::StringPiece> kQueueKeys{kLatencyWatermarkNsec()};
  return kQueueKeys;
}

const std::vector<folly::StringPiece>&
HwSysPortFb303Stats::kInMacsecPortMonotonicCounterStatKeys() const {
  // No macsec stats on sys ports
  static std::vector<folly::StringPiece> kMacsecInKeys{};
  return kMacsecInKeys;
}

const std::vector<folly::StringPiece>&
HwSysPortFb303Stats::kOutMacsecPortMonotonicCounterStatKeys() const {
  // No macsec stats on sys ports
  static std::vector<folly::StringPiece> kMacsecOutKeys{};
  return kMacsecOutKeys;
}

const std::vector<folly::StringPiece>&
HwSysPortFb303Stats::kPfcMonotonicCounterStatKeys() const {
  // No PFC stats on sys ports
  static std::vector<folly::StringPiece> kPfcKeys{};
  return kPfcKeys;
}

const std::vector<folly::StringPiece>&
HwSysPortFb303Stats::kPfcDeadlockMonotonicCounterStatKeys() const {
  // No PFC deadlock stats on sys ports
  static std::vector<folly::StringPiece> kPfcDeadlockKeys{};
  return kPfcDeadlockKeys;
}

const std::vector<folly::StringPiece>&
HwSysPortFb303Stats::kPriorityGroupMonotonicCounterStatKeys() const {
  // No priority group stats on sys ports
  static std::vector<folly::StringPiece> kPgKeys{};
  return kPgKeys;
}

const std::vector<folly::StringPiece>&
HwSysPortFb303Stats::kPriorityGroupCounterStatKeys() const {
  // No priority group stats on sys ports
  static std::vector<folly::StringPiece> kPgKeys{};
  return kPgKeys;
}

void HwSysPortFb303Stats::updateStats(
    const HwSysPortStats& curPortStats,
    const std::chrono::seconds& retrievedAt) {
  if (!initialized_) {
    reinitStats(std::nullopt);
    initialized_ = true;
  }
  timeRetrieved_ = retrievedAt;
  auto updateQueueStat = [this](
                             folly::StringPiece statKey,
                             int queueId,
                             const std::map<int16_t, int64_t>& queueStats) {
    auto qitr = queueStats.find(queueId);
    if (qitr != queueStats.end()) {
      updateStat(timeRetrieved_, statKey, queueId, qitr->second);
    }
  };
  for (const auto& queueIdAndName : queueId2Name()) {
    updateQueueStat(
        kOutDiscards(),
        queueIdAndName.first,
        *curPortStats.queueOutDiscardBytes_());
    updateQueueStat(
        kOutBytes(), queueIdAndName.first, *curPortStats.queueOutBytes_());
    if (curPortStats.queueWredDroppedPackets_()->size()) {
      updateQueueStat(
          kWredDroppedPackets(),
          queueIdAndName.first,
          *curPortStats.queueWredDroppedPackets_());
    }
    if (curPortStats.queueCreditWatchdogDeletedPackets_()->size()) {
      updateQueueStat(
          kCreditWatchdogDeletedPackets(),
          queueIdAndName.first,
          *curPortStats.queueCreditWatchdogDeletedPackets_());
    }
    if (curPortStats.queueLatencyWatermarkNsec_()->size()) {
      auto& queueStats = *curPortStats.queueLatencyWatermarkNsec_();
      auto qitr = queueStats.find(queueIdAndName.first);
      if (qitr != queueStats.end()) {
        fb303::fbData->setCounter(
            statName(
                kLatencyWatermarkNsec(),
                portName(),
                queueIdAndName.first,
                queueIdAndName.second),
            qitr->second);
      }
    }
  }
  if (curPortStats.queueWatermarkBytes_()->size()) {
    updateQueueWatermarkStats(*curPortStats.queueWatermarkBytes_());
  }
  portStats_ = curPortStats;
}
} // namespace facebook::fboss
