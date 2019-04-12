/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/AggregatePortStats.h"
#include "common/stats/ServiceData.h"
#include "fboss/agent/SwSwitch.h"
#include "fboss/agent/SwitchStats.h"
#include "fboss/agent/state/AggregatePort.h"

namespace facebook {
namespace fboss {

AggregatePortStats::AggregatePortStats(
    AggregatePortID aggregatePortID,
    std::string aggregatePortName)
    : aggregatePortID_(aggregatePortID),
      aggregatePortName_(aggregatePortName),
      flaps_(
          stats::ThreadCachedServiceData::get()->getThreadStats(),
          constructCounterName(aggregatePortName_, "flaps"),
          stats::SUM) {}

void AggregatePortStats::flapped() {
  flaps_.addValue(1);
}

std::string AggregatePortStats::constructCounterName(
    const std::string& aggregatePortName,
    const std::string& counterName) const {
  return folly::to<std::string>(aggregatePortName, ".", counterName);
}

void AggregatePortStats::recordStatistics(
    SwSwitch* sw,
    const std::shared_ptr<AggregatePort>& oldAggPort,
    const std::shared_ptr<AggregatePort>& newAggPort) {
  bool wasUpPreviously = oldAggPort->isUp();
  bool isUpNow = newAggPort->isUp();

  if (wasUpPreviously == isUpNow) {
    return;
  }

  auto aggregatePortID = newAggPort->getID();

  auto aggregatePortStats = sw->stats()->aggregatePort(aggregatePortID);
  if (!aggregatePortStats) {
    // Because AggregatePorts are only created by configuration, we can be
    // sure that config application has already been completed at this
    // point. It follows that the first time this code is executed,
    // aggregatePortStats will be constructed with the desired name- that
    // specified in the config.
    aggregatePortStats = sw->stats()->createAggregatePortStats(
        aggregatePortID, newAggPort->getName());
  }

  aggregatePortStats->flapped();
}

} // namespace fboss
} // namespace facebook
