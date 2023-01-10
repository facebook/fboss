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

namespace facebook::fboss {

const std::vector<folly::StringPiece>& HwSysPortFb303Stats::kPortStatKeys()
    const {
  // No port level stats on sys ports
  static std::vector<folly::StringPiece> kPortKeys{};
  return kPortKeys;
}

const std::vector<folly::StringPiece>& HwSysPortFb303Stats::kQueueStatKeys()
    const {
  static std::vector<folly::StringPiece> kQueueKeys{
      kOutBytes(), kOutDiscards()};
  return kQueueKeys;
}

const std::vector<folly::StringPiece>&
HwSysPortFb303Stats::kInMacsecPortStatKeys() const {
  // No macsec stats on sys ports
  static std::vector<folly::StringPiece> kMacsecInKeys{};
  return kMacsecInKeys;
}

const std::vector<folly::StringPiece>&
HwSysPortFb303Stats::kOutMacsecPortStatKeys() const {
  // No macsec stats on sys ports
  static std::vector<folly::StringPiece> kMacsecOutKeys{};
  return kMacsecOutKeys;
}

void HwSysPortFb303Stats::updateStats(
    const HwSysPortStats& curPortStats,
    const std::chrono::seconds& retrievedAt) {
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
  }
  if (curPortStats.queueWatermarkBytes_()->size()) {
    updateQueueWatermarkStats(*curPortStats.queueWatermarkBytes_());
  }
  portStats_ = curPortStats;
}
} // namespace facebook::fboss
