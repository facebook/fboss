/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/EcmpGroupConsolidator.h"

#include "fboss/agent/state/StateDelta.h"

#include <gflags/gflags.h>
#include <limits>

DEFINE_bool(
    consolidate_ecmp_groups,
    false,
    "Consolidate ECMP groups when approaching HW limits");

namespace facebook::fboss {

std::shared_ptr<SwitchState> EcmpGroupConsolidator::consolidate(
    const StateDelta& delta) {
  if (!FLAGS_consolidate_ecmp_groups) {
    return delta.newState();
  }
  return delta.newState();
}

EcmpGroupConsolidator::NextHopGroupId
EcmpGroupConsolidator::findNextAvailableId() const {
  std::unordered_set<NextHopGroupId> allocatedIds;
  for (const auto& [_, id] : nextHopGroup2Id_) {
    allocatedIds.insert(id);
  }
  for (auto start = kMinNextHopGroupId;
       start < std::numeric_limits<NextHopGroupId>::max();
       ++start) {
    if (allocatedIds.find(start) == allocatedIds.end()) {
      return start;
    }
  }
  throw FbossError("Unable to find id to allocate for new next hop group");
}
} // namespace facebook::fboss
