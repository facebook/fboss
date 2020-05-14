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

#include "fboss/agent/hw/CounterUtils.h"
#include "fboss/agent/hw/StatsConstants.h"
#include "fboss/agent/hw/bcm/BcmPortTable.h"
#include "fboss/agent/hw/bcm/BcmSwitch.h"

#include <folly/logging/xlog.h>
#include <chrono>

namespace facebook::fboss {

BcmTrunkStats::BcmTrunkStats(const BcmSwitchIf* hw)
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
  initializeCounter(kInDiscardsRaw());
  initializeCounter(kInErrors());
  initializeCounter(kInPause());
  initializeCounter(kInIpv4HdrErrors());
  initializeCounter(kInIpv6HdrErrors());
  initializeCounter(kInDstNullDiscards());

  initializeCounter(kOutBytes());
  initializeCounter(kOutUnicastPkts());
  initializeCounter(kOutMulticastPkts());
  initializeCounter(kOutBroadcastPkts());
  initializeCounter(kOutDiscards());
  initializeCounter(kOutErrors());
  initializeCounter(kOutPause());
  initializeCounter(kOutCongestionDiscards());
  initializeCounter(kOutEcnCounter());
}

void BcmTrunkStats::initializeCounter(folly::StringPiece counterKey) {
  auto oldCounter = getCounterIf(counterKey);

  auto externalCounterName = constructCounterName(counterKey);
  auto newCounter =
      stats::MonotonicCounter({externalCounterName, fb303::SUM, fb303::RATE});

  if (oldCounter) {
    oldCounter->swap(newCounter);
    utility::deleteCounter(newCounter.getName());
  } else {
    counters_.emplace(counterKey.str(), std::move(newCounter));
  }
}

void BcmTrunkStats::grantMembership(PortID memberPortID) {
  bool inserted;
  std::tie(std::ignore, inserted) =
      memberPortIDs_.wlock()->insert(memberPortID);
  if (!inserted) {
    XLOG(WARNING) << "BcmTrunkStats for AggregatePort " << aggregatePortID_
                  << " is out of sync for member " << memberPortID;
  }
}

void BcmTrunkStats::revokeMembership(PortID memberPortID) {
  auto numErased = memberPortIDs_.wlock()->erase(memberPortID);

  if (numErased != 0) {
    XLOG(WARNING) << "BcmTrunkStats for AggregatePort " << aggregatePortID_
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
        XLOG(WARNING) << "BcmTrunkStats for AggregatePort " << aggregatePortID_
                      << " is out of sync for member " << memberPort;
        continue;
      }

      auto memberStatsOptional = memberPort->getPortStats();
      if (!memberStatsOptional.has_value()) {
        XLOG(WARNING) << "BcmTrunkStats for AggregatePort " << aggregatePortID_
                      << " is out of sync for member " << memberPort;
        continue;
      }
      auto& memberStats = memberStatsOptional.value();

      *cumulativeSum.inBytes__ref() += *memberStats.inBytes__ref();
      *cumulativeSum.inUnicastPkts__ref() += *memberStats.inUnicastPkts__ref();
      *cumulativeSum.inMulticastPkts__ref() +=
          *memberStats.inMulticastPkts__ref();
      *cumulativeSum.inBroadcastPkts__ref() +=
          *memberStats.inBroadcastPkts__ref();
      *cumulativeSum.inDiscards__ref() += *memberStats.inDiscards__ref();
      *cumulativeSum.inErrors__ref() += *memberStats.inErrors__ref();
      *cumulativeSum.inPause__ref() += *memberStats.inPause__ref();
      *cumulativeSum.inIpv4HdrErrors__ref() +=
          *memberStats.inIpv4HdrErrors__ref();
      *cumulativeSum.inIpv6HdrErrors__ref() +=
          *memberStats.inIpv6HdrErrors__ref();
      *cumulativeSum.inDiscardsRaw__ref() += *memberStats.inDiscardsRaw__ref();
      *cumulativeSum.inDstNullDiscards__ref() +=
          *memberStats.inDstNullDiscards__ref();

      *cumulativeSum.outBytes__ref() += *memberStats.outBytes__ref();
      *cumulativeSum.outUnicastPkts__ref() +=
          *memberStats.outUnicastPkts__ref();
      *cumulativeSum.outMulticastPkts__ref() +=
          *memberStats.outMulticastPkts__ref();
      *cumulativeSum.outBroadcastPkts__ref() +=
          *memberStats.outBroadcastPkts__ref();
      *cumulativeSum.outDiscards__ref() += *memberStats.outDiscards__ref();
      *cumulativeSum.outErrors__ref() += *memberStats.outErrors__ref();
      *cumulativeSum.outPause__ref() += *memberStats.outPause__ref();
      *cumulativeSum.outCongestionDiscardPkts__ref() +=
          *memberStats.outCongestionDiscardPkts__ref();

      if (!timeRetrievedFromMemberPort) {
        timeRetrieved = memberPort->getTimeRetrieved();
        timeRetrievedFromMemberPort = true;
      }
    }
  }

  return std::make_pair(cumulativeSum, timeRetrieved);
}

void BcmTrunkStats::clearHwTrunkStats(HwTrunkStats& stats) {
  *stats.inBytes__ref() = 0;
  *stats.inUnicastPkts__ref() = 0;
  *stats.inMulticastPkts__ref() = 0;
  *stats.inBroadcastPkts__ref() = 0;
  *stats.inDiscards__ref() = 0;
  *stats.inErrors__ref() = 0;
  *stats.inPause__ref() = 0;
  *stats.inIpv4HdrErrors__ref() = 0;
  *stats.inIpv6HdrErrors__ref() = 0;
  *stats.inDiscardsRaw__ref() = 0;
  *stats.inDstNullDiscards__ref() = 0;

  *stats.outBytes__ref() = 0;
  *stats.outUnicastPkts__ref() = 0;
  *stats.outMulticastPkts__ref() = 0;
  *stats.outBroadcastPkts__ref() = 0;
  *stats.outDiscards__ref() = 0;
  *stats.outErrors__ref() = 0;
  *stats.outPause__ref() = 0;
  *stats.outCongestionDiscardPkts__ref() = 0;
}

stats::MonotonicCounter* FOLLY_NULLABLE
BcmTrunkStats::getCounterIf(folly::StringPiece counterKey) {
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

  updateCounter(then, kInBytes(), *stats.inBytes__ref());
  updateCounter(then, kInUnicastPkts(), *stats.inUnicastPkts__ref());
  updateCounter(then, kInMulticastPkts(), *stats.inMulticastPkts__ref());
  updateCounter(then, kInBroadcastPkts(), *stats.inBroadcastPkts__ref());
  updateCounter(then, kInDiscards(), *stats.inDiscards__ref());
  updateCounter(then, kInErrors(), *stats.inErrors__ref());
  updateCounter(then, kInPause(), *stats.inPause__ref());
  updateCounter(then, kInIpv4HdrErrors(), *stats.inIpv4HdrErrors__ref());
  updateCounter(then, kInIpv6HdrErrors(), *stats.inIpv6HdrErrors__ref());

  updateCounter(then, kOutBytes(), *stats.outBytes__ref());
  updateCounter(then, kOutUnicastPkts(), *stats.outUnicastPkts__ref());
  updateCounter(then, kOutMulticastPkts(), *stats.outMulticastPkts__ref());
  updateCounter(then, kOutBroadcastPkts(), *stats.outBroadcastPkts__ref());
  updateCounter(then, kOutDiscards(), *stats.outDiscards__ref());
  updateCounter(then, kOutErrors(), *stats.outErrors__ref());
  updateCounter(then, kOutPause(), *stats.outPause__ref());
  updateCounter(
      then, kOutCongestionDiscards(), *stats.outCongestionDiscardPkts__ref());
  updateCounter(then, kOutEcnCounter(), *stats.outEcnCounter__ref());
}

std::string BcmTrunkStats::constructCounterName(
    folly::StringPiece counterKey) const {
  if (trunkName_ == "") {
    return folly::to<std::string>("po", aggregatePortID_, ".", counterKey);
  }
  return folly::to<std::string>(trunkName_, ".", counterKey);
}

} // namespace facebook::fboss
