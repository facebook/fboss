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
    : hw_(hw), memberPortIDs_(), counters_() {}

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
  std::tie(std::ignore, inserted) =
      memberPortIDs_.wlock()->insert(memberPortID);
  if (!inserted) {
    LOG(WARNING) << "BcmTrunkStats for AggregatePort " << aggregatePortID_
                 << " is out of sync for member " << memberPortID;
  }
}

void BcmTrunkStats::revokeMembership(PortID memberPortID) {
  auto numErased = memberPortIDs_.wlock()->erase(memberPortID);

  if (numErased != 0) {
    LOG(WARNING) << "BcmTrunkStats for AggregatePort " << aggregatePortID_
                 << " is out of sync for member " << memberPortID;
  }
}

std::pair<HwTrunkStats, std::chrono::seconds>
BcmTrunkStats::accumulateMemberStats() const {
  auto portTable = hw_->getPortTable();

  auto timeRetrieved = std::chrono::duration_cast<std::chrono::seconds>(
      std::chrono::system_clock::now().time_since_epoch());
  bool timeRetrievedFromMemberPort = false;

  HwTrunkStats cumulativeSum;
  clearHwTrunkStats(cumulativeSum);

  {
    auto lockedMemberPortIDsPtr = memberPortIDs_.rlock();
    for (const auto& memberPortID : *lockedMemberPortIDsPtr) {
      /* There are two data-structures below that need to be thread-safe:
       *
       * 1. BcmPort::portStats_ (returned by getPortStats()) because it is read
       * from and written to in this thread (updateStatsThread) every second AND
       * may be written to by the update thread (if BcmPort::updateName() is
       * invoked).
       *
       * BcmPort::portStats_ is protected with a read-write lock.
       *
       * 2. BcmPortTable::bcmPhysicalPorts_ (over which getBcmPortIf() is
       * iterating) because it is being read in this thread (updateStatsThread)
       * every second AND may be written to in the update thread.
       *
       * BcmPortTable::bcmPhysicalPorts_ is already accessed concurrently from
       * updateStatsThread and the update thread in
       * BcmPortTable::updatePortStats(). Although this is unsound, it turns out
       * to be safe at run-time because bcmPhysicalPorts_ is not modified after
       * the application of the SwitchState derived from the Platform.
       * Because protecting bcmPhysicalPorts_ would be a departure from our
       * current concurrency model in BcmSwitch, and the data structure is
       * already being accessed concurrently elsewhere in precisely the same way
       *  as below, I have foregone locking it below.
       * In the future, it seems a combination of BcmPortStats and thread-local
       * storage could be the solution to this ailement.
       *
       */
      auto memberPort = portTable->getBcmPortIf(memberPortID);
      if (!memberPort) {
        LOG(WARNING) << "BcmTrunkStats for AggregatePort " << aggregatePortID_
                     << " is out of sync for member " << memberPort;
        continue;
      }

      auto memberStats = memberPort->getPortStats();
      cumulativeSum.inBytes_ += memberStats.inBytes_;
      cumulativeSum.inUnicastPkts_ += memberStats.inUnicastPkts_;
      cumulativeSum.inMulticastPkts_ += memberStats.inMulticastPkts_;
      cumulativeSum.inBroadcastPkts_ += memberStats.inBroadcastPkts_;
      cumulativeSum.inDiscards_ += memberStats.inDiscards_;
      cumulativeSum.inErrors_ += memberStats.inErrors_;
      cumulativeSum.inPause_ += memberStats.inPause_;
      cumulativeSum.inIpv4HdrErrors_ += memberStats.inIpv4HdrErrors_;
      cumulativeSum.inIpv6HdrErrors_ += memberStats.inIpv6HdrErrors_;
      cumulativeSum.inNonPauseDiscards_ += memberStats.inNonPauseDiscards_;

      cumulativeSum.outBytes_ += memberStats.outBytes_;
      cumulativeSum.outUnicastPkts_ += memberStats.outUnicastPkts_;
      cumulativeSum.outMulticastPkts_ += memberStats.outMulticastPkts_;
      cumulativeSum.outBroadcastPkts_ += memberStats.outBroadcastPkts_;
      cumulativeSum.outDiscards_ += memberStats.outDiscards_;
      cumulativeSum.outErrors_ += memberStats.outErrors_;
      cumulativeSum.outPause_ += memberStats.outPause_;
      cumulativeSum.outCongestionDiscardPkts_ +=
          memberStats.outCongestionDiscardPkts_;

      if (!timeRetrievedFromMemberPort) {
        timeRetrieved = memberPort->getTimeRetrieved();
        timeRetrievedFromMemberPort = true;
      }
    }
  }

  return std::make_pair(cumulativeSum, timeRetrieved);
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
  HwTrunkStats stats;
  std::chrono::seconds then;

  std::tie(stats, then) = accumulateMemberStats();

  updateCounter(then, kInBytes(), stats.inBytes_);
  updateCounter(then, kInUnicastPkts(), stats.inUnicastPkts_);
  updateCounter(then, kInMulticastPkts(), stats.inMulticastPkts_);
  updateCounter(then, kInBroadcastPkts(), stats.inBroadcastPkts_);
  updateCounter(then, kInDiscards(), stats.inDiscards_);
  updateCounter(then, kInErrors(), stats.inErrors_);
  updateCounter(then, kInPause(), stats.inPause_);
  updateCounter(then, kInIpv4HdrErrors(), stats.inIpv4HdrErrors_);
  updateCounter(then, kInIpv6HdrErrors(), stats.inIpv6HdrErrors_);
  updateCounter(then, kInNonPauseDiscards(), stats.inNonPauseDiscards_);

  updateCounter(then, kOutBytes(), stats.outBytes_);
  updateCounter(then, kOutUnicastPkts(), stats.outUnicastPkts_);
  updateCounter(then, kOutMulticastPkts(), stats.outMulticastPkts_);
  updateCounter(then, kOutBroadcastPkts(), stats.outBroadcastPkts_);
  updateCounter(then, kOutDiscards(), stats.outDiscards_);
  updateCounter(then, kOutErrors(), stats.outErrors_);
  updateCounter(then, kOutPause(), stats.outPause_);
  updateCounter(
      then, kOutCongestionDiscards(), stats.outCongestionDiscardPkts_);
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
