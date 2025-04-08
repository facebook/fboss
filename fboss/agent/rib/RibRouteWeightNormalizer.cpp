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
    int numPlanePathsPerRack)
    : numRacks_(numRacks),
      numPlanePathsPerRack_(numPlanePathsPerRack),
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
} // namespace facebook::fboss
