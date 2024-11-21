/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/hw/bcm/BcmTrunkStats.h"

#include "fboss/agent/hw/CounterUtils.h"
#include "fboss/agent/hw/HwTrunkCounters.h"
#include "fboss/agent/hw/bcm/BcmPortTable.h"
#include "fboss/agent/hw/bcm/BcmSwitch.h"

#include <folly/logging/xlog.h>
#include <chrono>
#include <memory>

namespace facebook::fboss {

BcmTrunkStats::BcmTrunkStats(const BcmSwitchIf* hw)
    : hw_(hw), memberPortIDs_(), counters_() {}

void BcmTrunkStats::initialize(
    AggregatePortID aggPortID,
    std::string trunkName) {
  aggregatePortID_ = aggPortID;
  trunkName_ = trunkName;
  if (!counters_) {
    counters_ = std::make_unique<utility::HwTrunkCounters>(
        aggregatePortID_, trunkName_);
  } else {
    counters_->reinitialize(aggregatePortID_, trunkName_);
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
  utility::clearHwTrunkStats(cumulativeSum);

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

      utility::accumulateHwTrunkMemberStats(cumulativeSum, memberStats);
      if (!timeRetrievedFromMemberPort) {
        timeRetrieved = memberPort->getTimeRetrieved();
        timeRetrievedFromMemberPort = true;
      }
    }
  }

  return std::make_pair(cumulativeSum, timeRetrieved);
}

void BcmTrunkStats::update() {
  HwTrunkStats stats;
  std::chrono::seconds then;

  std::tie(stats, then) = accumulateMemberStats();
  counters_->updateCounters(then, stats);
}

HwTrunkStats BcmTrunkStats::getHwTrunkStats() const {
  return counters_->getHwTrunkStats();
}

} // namespace facebook::fboss
