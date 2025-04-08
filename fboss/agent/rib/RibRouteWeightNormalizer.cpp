/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/rib/RibRouteWeightNormalizer.h"

#include <folly/logging/xlog.h>

namespace facebook::fboss {

RibRouteWeightNormalizer::RibRouteWeightNormalizer(
    int numRacks,
    int numPlanePathsPerRack,
    int rackId)
    : numRacks_(numRacks),
      numPlanePathsPerRack_(numPlanePathsPerRack),
      rackId_(rackId),
      pruneLookupTable_(
          numRacks * numPlanePathsPerRack + 1,
          std::vector<std::vector<int>>(
              numRacks_,
              std::vector<int>(numRacks_, 0))) {
  /*
    initialize a lookup table to determine how many paths to prune for each
    dst/src rack for N spine plane failures. The lookup table is a 3D array
    with the first dimension being the number of failures. The prunes are
    distributed evenly across the destination racks as illustrated in the below
    table

  Pruning at pod 2 rtsws for  1 spine plane failure
  +-----------+-----------+-----------+-----------+-----------+----------+
  |  rtsw1.p2 | rtsw2.p2  | rtsw3.p2  | rtsw4.p2  | rtsw5.p2  | rtsw6.p2 |
  +-----------+-----------+-----------+-----------+-----------+----------+
  |rtsw1.p1 1 |rtsw1.p1 0 |rtsw1.p1 0 |rtsw1.p1 0 |rtsw1.p1 0 |rtsw1.p1 0|
  |rtsw2.p1 0 |rtsw2.p1 1 |rtsw2.p1 0 |rtsw2.p1 0 |rtsw2.p1 0 |rtsw2.p1 0|
  |rtsw3.p1 0 |rtsw3.p1 0 |rtsw3.p1 1 |rtsw3.p1 0 |rtsw3.p1 0 |rtsw3.p1 0|
  |rtsw4.p1 0 |rtsw4.p1 0 |rtsw4.p1 0 |rtsw4.p1 1 |rtsw4.p1 0 |rtsw4.p1 0|
  |rtsw5.p1 0 |rtsw5.p1 0 |rtsw5.p1 0 |rtsw5.p1 0 |rtsw5.p1 1 |rtsw5.p1 0|
  |rtsw6.p1 0 |rtsw6.p1 0 |rtsw6.p1 0 |rtsw6.p1 0 |rtsw6.p1 0 |rtsw6.p1 1|
  +-----------+-----------+-----------+-----------+-----------+----------+

  Pruning at pod 2 rtsws for  2 spine plane failures
  +-----------+-----------+-----------+-----------+-----------+----------+
  |  rtsw1.p2 | rtsw2.p2  | rtsw3.p2  | rtsw4.p2  | rtsw5.p2  | rtsw6.p2 |
  +-----------+-----------+-----------+-----------+-----------+----------+
  |rtsw1.p1 1 |rtsw1.p1 0 |rtsw1.p1 0 |rtsw1.p1 0 |rtsw1.p1 0 |rtsw1.p1 1|
  |rtsw2.p1 1 |rtsw2.p1 1 |rtsw2.p1 0 |rtsw2.p1 0 |rtsw2.p1 0 |rtsw2.p1 0|
  |rtsw3.p1 0 |rtsw3.p1 1 |rtsw3.p1 1 |rtsw3.p1 0 |rtsw3.p1 0 |rtsw3.p1 0|
  |rtsw4.p1 0 |rtsw4.p1 0 |rtsw4.p1 1 |rtsw4.p1 1 |rtsw4.p1 0 |rtsw4.p1 0|
  |rtsw5.p1 0 |rtsw5.p1 0 |rtsw5.p1 0 |rtsw5.p1 1 |rtsw5.p1 1 |rtsw5.p1 0|
  |rtsw6.p1 0 |rtsw6.p1 0 |rtsw6.p1 0 |rtsw6.p1 0 |rtsw6.p1 1 |rtsw6.p1 1|
  +-----------+-----------+-----------+-----------+-----------+----------+

  Pruning at pod 2 rtsws for  7 spine plane failures
  +-----------+-----------+-----------+-----------+-----------+----------+
  |  rtsw1.p2 | rtsw2.p2  | rtsw3.p2  | rtsw4.p2  | rtsw5.p2  | rtsw6.p2 |
  +-----------+-----------+-----------+-----------+-----------+----------+
  |rtsw1.p1 2 |rtsw1.p1 1 |rtsw1.p1 1 |rtsw1.p1 1 |rtsw1.p1 1 |rtsw1.p1 1|
  |rtsw2.p1 1 |rtsw2.p1 2 |rtsw2.p1 1 |rtsw2.p1 1 |rtsw2.p1 1 |rtsw2.p1 1|
  |rtsw3.p1 1 |rtsw3.p1 1 |rtsw3.p1 2 |rtsw3.p1 1 |rtsw3.p1 1 |rtsw3.p1 1|
  |rtsw4.p1 1 |rtsw4.p1 1 |rtsw4.p1 1 |rtsw4.p1 2 |rtsw4.p1 1 |rtsw4.p1 1|
  |rtsw5.p1 1 |rtsw5.p1 1 |rtsw5.p1 1 |rtsw5.p1 1 |rtsw5.p1 2 |rtsw5.p1 1|
  |rtsw6.p1 1 |rtsw6.p1 1 |rtsw6.p1 1 |rtsw6.p1 1 |rtsw6.p1 1 |rtsw6.p1 2|
  +-----------+-----------+-----------+-----------+-----------+----------+
  */
  int numPathsToPrune = 1;
  for (int failureCount = 0; failureCount <= numRacks * numPlanePathsPerRack;
       failureCount++) {
    if (failureCount == 0) {
      // no paths to prune for zero failures
      continue;
    }
    pruneLookupTable_[failureCount] = pruneLookupTable_[failureCount - 1];
    // add one more destination for each source rack id to prune for the new
    // failure
    for (int srcRackId = 0; srcRackId < numRacks; srcRackId++) {
      int impactedDestRackId = (srcRackId + failureCount - 1) % numRacks;
      pruneLookupTable_[failureCount][impactedDestRackId][srcRackId] =
          numPathsToPrune;
    }
    if (failureCount % numRacks == 0) {
      numPathsToPrune++;
    }
  }
}

int RibRouteWeightNormalizer::getNumPathsToPrune(
    int numFailures,
    RackId dstRack,
    RackId srcRack) {
  return pruneLookupTable_[numFailures][dstRack][srcRack];
}

RouteNextHopSet RibRouteWeightNormalizer::getNormalizedNexthops(
    RouteNextHopSet& nhops) {
  RouteNextHopSet normalizedNexthops;
  std::vector<ResolvedNextHop> resolvedNexthops;
  for (const auto& nhop : nhops) {
    resolvedNexthops.emplace_back(
        nhop.addr(),
        nhop.intf(),
        nhop.weight(),
        nhop.labelForwardingAction(),
        nhop.disableTTLDecrement(),
        nhop.planeId(),
        nhop.remotePodCapacity(),
        nhop.spineCapacity(),
        nhop.rackCapacity(),
        nhop.rackId(),
        nhop.adjustedWeight());
  }
  normalizeWeightsForNexthops(resolvedNexthops);
  for (auto& nhop : resolvedNexthops) {
    normalizedNexthops.insert(std::move(nhop));
  }
  return normalizedNexthops;
}

void RibRouteWeightNormalizer::normalizeWeightsForNexthops(
    std::vector<ResolvedNextHop>& nhs) {
  RackId dstRack;
  std::unordered_map<PlaneId, int> localPlaneCapacity;
  // plane id to rack and spine failures
  std::unordered_map<PlaneId, std::pair<int, int>> planeIdToFailures;
  bool hasFailure = false;
  // walk through the nexthops and collect the connectivity information
  for (const auto& nh : nhs) {
    // ignore prefixes that not originated by racks
    if (!nh.rackId().has_value()) {
      return;
    }
    dstRack = nh.rackId().value();
    int numRackFailures = 0;
    // For remote pod prefixes, check rack failures
    if (nh.remotePodCapacity() && *nh.remotePodCapacity()) {
      numRackFailures = numPlanePathsPerRack_ - nh.remotePodCapacity().value();
    } else if (nh.rackCapacity() && *nh.rackCapacity()) {
      // rack failures for local pod prefixes are handled same as remote pod
      // rack failures
      numRackFailures = numPlanePathsPerRack_ - nh.rackCapacity().value();
    }
    int numSpineFailures = 0;
    if (nh.spineCapacity() && *nh.spineCapacity()) {
      // total spine capaity and rack capacity in pod should be matching
      numSpineFailures =
          numRacks_ * numPlanePathsPerRack_ - nh.spineCapacity().value();
    }
    if (numRackFailures || numSpineFailures) {
      hasFailure = true;
    }
    CHECK(nh.planeId().has_value());
    auto planeId = PlaneId(*nh.planeId());
    planeIdToFailures.insert(std::make_pair(
        planeId, std::make_pair(numRackFailures, numSpineFailures)));

    // update local capacity information for each plane
    auto localPlaneInfo = localPlaneCapacity.find(planeId);
    if (localPlaneInfo == localPlaneCapacity.end()) {
      localPlaneCapacity.insert(std::make_pair(planeId, 1));
    } else {
      localPlaneInfo->second++;
    }
  }

  if (!hasFailure) {
    return;
  }
  // walk through the nexthops and normalize the weights
  for (const auto& planeFailures : planeIdToFailures) {
    auto planeId = planeFailures.first;
    auto rackFailures = planeFailures.second.first;
    auto spineFailures = planeFailures.second.second;
    auto localPlaneInfo = localPlaneCapacity.find(planeId);
    CHECK(localPlaneInfo != localPlaneCapacity.end());
    auto numLocalFailures = numPlanePathsPerRack_ - localPlaneInfo->second;

    // Determine how many paths to prune for spine failures for the src/dst pair
    auto numSpineFailuresToAct =
        getNumPathsToPrune(spineFailures, dstRack, rackId_);

    // Total prunes needed is the max of rack and spine failures to act on
    // after discounting for the  local failures
    auto numPrunesNeeded = std::max(
        std::max(rackFailures, numSpineFailuresToAct) - numLocalFailures, 0);
    XLOG(DBG4) << "Pruning " << numPrunesNeeded << " paths for plane "
               << planeId << " dst rack " << dstRack << " src rack " << rackId_
               << " remote rack failures " << rackFailures << " spine failures "
               << spineFailures << " local failures " << numLocalFailures;
    if (numPrunesNeeded) {
      for (auto& nh : nhs) {
        if (numPrunesNeeded && nh.planeId() && *nh.planeId() == planeId) {
          numPrunesNeeded--;
          // set adjusted weight to 0 to indicate that path is pruned
          nh.setAdjustedWeight(0);
        }
      }
    }
    CHECK(!numPrunesNeeded);
  }
}
} // namespace facebook::fboss
