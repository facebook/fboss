/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#pragma once

#include <vector>

#include "fboss/agent/state/RouteNextHopEntry.h"

using RackId = int;
using PlaneId = int;

namespace facebook::fboss {

class RibRouteWeightNormalizer {
 public:
  RibRouteWeightNormalizer(int numRacks, int numPlanePathsPerRack, int rackId);
  virtual ~RibRouteWeightNormalizer() = default;
  int getNumPathsToPrune(int numFailures, RackId dstRack, RackId srcRack);
  RouteNextHopSet getNormalizedNexthops(RouteNextHopSet& nhop);

 protected:
  void normalizeWeightsForNexthops(std::vector<ResolvedNextHop>& nhs);

 private:
  int numRacks_;
  int numPlanePathsPerRack_;
  int rackId_;
  /* num failures to number of paths to be pruned for dest, src rack id */
  std::vector<std::vector<std::vector<int>>> pruneLookupTable_;
};

} // namespace facebook::fboss
