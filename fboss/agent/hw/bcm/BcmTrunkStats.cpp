/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "BcmTrunkStats.h"

#include "fboss/agent/hw/bcm/BcmPortTable.h"
#include "fboss/agent/hw/bcm/BcmStatsConstants.h"
#include "fboss/agent/hw/bcm/BcmSwitch.h"

#include <chrono>

namespace facebook {
namespace fboss {

BcmTrunkStats::BcmTrunkStats(const BcmSwitch* hw)
    : hw_(hw), memberPorts_(), counters_() {}

void BcmTrunkStats::initialize(
    AggregatePortID aggPortID,
    std::string trunkName) {
  aggregatePortID_ = aggPortID;
  trunkName_ = trunkName;

  initializeCounter(kInBytes());
  initializeCounter(kInUnicastPkts());
  initializeCounter(kInMulticastPkts());
  initializeCounter(kInBroadcastPkts());
  initializeCounter(kInDiscards());
  initializeCounter(kInErrors());
  initializeCounter(kInPause());
  initializeCounter(kInIpv4HdrErrors());
  initializeCounter(kInIpv6HdrErrors());
  initializeCounter(kInNonPauseDiscards());

  initializeCounter(kOutBytes());
  initializeCounter(kOutUnicastPkts());
  initializeCounter(kOutMulticastPkts());
  initializeCounter(kOutBroadcastPkts());
  initializeCounter(kOutDiscards());
  initializeCounter(kOutErrors());
  initializeCounter(kOutPause());
  initializeCounter(kOutCongestionDiscards());
}

void BcmTrunkStats::initializeCounter(folly::StringPiece counterKey) {
  auto oldCounter = getCounterIf(counterKey);

  auto externalCounterName = constructCounterName(counterKey);
  auto newCounter =
      stats::MonotonicCounter({externalCounterName, stats::SUM, stats::RATE});

  if (oldCounter) {
    oldCounter->swap(newCounter);
  } else {
    counters_.emplace(counterKey.str(), std::move(newCounter));
  }
}

void BcmTrunkStats::grantMembership(PortID memberPortID) {
  bool inserted;
  std::tie(std::ignore, inserted) = memberPorts_.wlock()->insert(memberPortID);
  if (!inserted) {
    LOG(WARNING) << "BcmTrunkStats::memberPorts_ out of sync";
  }
}

void BcmTrunkStats::revokeMembership(PortID memberPortID) {
  auto numErased = memberPorts_.wlock()->erase(memberPortID);

  if (numErased != 0) {
    LOG(WARNING) << "BcmTrunkStats::memberPorts_ out of sync";
  }
}

const HwTrunkStats BcmTrunkStats::accumulateMemberStats() const {
  auto portTable = hw_->getPortTable();

  HwTrunkStats cumulativeSum;
  clearHwTrunkStats(cumulativeSum);

  {
    auto lockedMemberPortsPtr = memberPorts_.rlock();
    for (const auto& memberPort : *lockedMemberPortsPtr) {
      auto memberStats = portTable->getBcmPortIf(memberPort)->getStats();

      cumulativeSum.inBytes_            += memberStats.inBytes_;
      cumulativeSum.inUnicastPkts_      += memberStats.inUnicastPkts_;
      cumulativeSum.inMulticastPkts_    += memberStats.inMulticastPkts_;
      cumulativeSum.inBroadcastPkts_    += memberStats.inBroadcastPkts_;
      cumulativeSum.inDiscards_         += memberStats.inDiscards_;
      cumulativeSum.inErrors_           += memberStats.inErrors_;
      cumulativeSum.inPause_            += memberStats.inPause_;
      cumulativeSum.inIpv4HdrErrors_    += memberStats.inIpv4HdrErrors_;
      cumulativeSum.inIpv6HdrErrors_    += memberStats.inIpv6HdrErrors_;
      cumulativeSum.inNonPauseDiscards_ += memberStats.inNonPauseDiscards_;

      cumulativeSum.outBytes_                 += memberStats.outBytes_;
      cumulativeSum.outUnicastPkts_           += memberStats.outUnicastPkts_;
      cumulativeSum.outMulticastPkts_         += memberStats.outMulticastPkts_;
      cumulativeSum.outBroadcastPkts_         += memberStats.outBroadcastPkts_;
      cumulativeSum.outDiscards_              += memberStats.outDiscards_;
      cumulativeSum.outErrors_                += memberStats.outErrors_;
      cumulativeSum.outPause_                 += memberStats.outPause_;
      cumulativeSum.outCongestionDiscardPkts_ +=
          memberStats.outCongestionDiscardPkts_;
    }
  }

  return cumulativeSum;
}

void BcmTrunkStats::clearHwTrunkStats(HwTrunkStats& stats) {
  stats.inBytes_            = 0;
  stats.inUnicastPkts_      = 0;
  stats.inMulticastPkts_    = 0;
  stats.inBroadcastPkts_    = 0;
  stats.inDiscards_         = 0;
  stats.inErrors_           = 0;
  stats.inPause_            = 0;
  stats.inIpv4HdrErrors_    = 0;
  stats.inIpv6HdrErrors_    = 0;
  stats.inNonPauseDiscards_ = 0;

  stats.outBytes_                 = 0;
  stats.outUnicastPkts_           = 0;
  stats.outMulticastPkts_         = 0;
  stats.outBroadcastPkts_         = 0;
  stats.outDiscards_              = 0;
  stats.outErrors_                = 0;
  stats.outPause_                 = 0;
  stats.outCongestionDiscardPkts_ = 0;
}

stats::MonotonicCounter* FOLLY_NULLABLE BcmTrunkStats::getCounterIf(
    folly::StringPiece counterKey) {
  auto it = counters_.find(counterKey.str());
  if (it == counters_.end()) {
    return nullptr;
  }

  return &(it->second);
}

void BcmTrunkStats::updateCounter(
    std::chrono::seconds now,
    folly::StringPiece counterKey,
    int64_t value) {
  auto counter = getCounterIf(counterKey);
  if (!counter) {
    return;
  }

  counter->updateValue(now, value);
}

void BcmTrunkStats::update() {
  auto stats = accumulateMemberStats();

  // TODO(samank): use time when member stats were actually retrieved
  auto now = std::chrono::duration_cast<std::chrono::seconds>(
      std::chrono::system_clock::now().time_since_epoch());

  updateCounter(now, kInBytes(), stats.inBytes_);
  updateCounter(now, kInUnicastPkts(), stats.inUnicastPkts_);
  updateCounter(now, kInMulticastPkts(), stats.inMulticastPkts_);
  updateCounter(now, kInBroadcastPkts(), stats.inBroadcastPkts_);
  updateCounter(now, kInDiscards(), stats.inDiscards_);
  updateCounter(now, kInErrors(), stats.inErrors_);
  updateCounter(now, kInPause(), stats.inPause_);
  updateCounter(now, kInIpv4HdrErrors(), stats.inIpv4HdrErrors_);
  updateCounter(now, kInIpv6HdrErrors(), stats.inIpv6HdrErrors_);
  updateCounter(now, kInNonPauseDiscards(), stats.inNonPauseDiscards_);

  updateCounter(now, kOutBytes(), stats.outBytes_);
  updateCounter(now, kOutUnicastPkts(), stats.outUnicastPkts_);
  updateCounter(now, kOutMulticastPkts(), stats.outMulticastPkts_);
  updateCounter(now, kOutBroadcastPkts(), stats.outBroadcastPkts_);
  updateCounter(now, kOutDiscards(), stats.outDiscards_);
  updateCounter(now, kOutErrors(), stats.outErrors_);
  updateCounter(now, kOutPause(), stats.outPause_);
  updateCounter(now, kOutCongestionDiscards(), stats.outCongestionDiscardPkts_);
}

std::string BcmTrunkStats::constructCounterName(
    folly::StringPiece counterKey) const {
  if (trunkName_ == "") {
    return folly::to<std::string>("po", aggregatePortID_, ".", counterKey);
  }
  return folly::to<std::string>(trunkName_, ".", counterKey);
}

} // namespace fboss
} // namespace facebook
