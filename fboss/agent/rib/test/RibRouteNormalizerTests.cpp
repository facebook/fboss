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

#include <gtest/gtest.h>

#include "fboss/agent/test/TestUtils.h"

using namespace facebook::fboss;

class RibRouteWeightNormalizerTest : public RibRouteWeightNormalizer {
 public:
  RibRouteWeightNormalizerTest(
      int numRacks,
      int numPlanePathsPerRack,
      int rackId = 1,
      int numSpineFailuresToSkip = 0,
      int spinePruneStepCount = 1)
      : RibRouteWeightNormalizer(
            numRacks,
            numPlanePathsPerRack,
            rackId,
            numSpineFailuresToSkip,
            spinePruneStepCount) {}

  static void runNextHopPruningTest(
      std::map<int, int>& planeIdToRemoteCapacity,
      std::map<int, int>& planeIdToSpineCapacity,
      std::map<int, int>& planeIdToLocalPathCount,
      std::map<int, int>& planeIdToLocalRackCapacity,
      std::map<int, int>& planeIdToExpectedPrunes) {
    int numRacks = 6;
    int numPlanes = 8;
    int numPlanePathsPerRack = 6;
    RibRouteWeightNormalizerTest normalizer(numRacks, numPlanePathsPerRack);
    std::vector<ResolvedNextHop> nhs;
    int nhid = 0;
    for (int planeId = 0; planeId < numPlanes; planeId++) {
      for (int localPath = 0; localPath < planeIdToLocalPathCount[planeId];
           localPath++) {
        int remoteCapacity = planeIdToRemoteCapacity[planeId];
        int spineCapacity = planeIdToSpineCapacity[planeId];
        int loalRackCapacity = planeIdToLocalRackCapacity[planeId];
        NetworkTopologyInformation topologyInfo;
        topologyInfo.rack_id() = 1;
        topologyInfo.plane_id() = planeId;
        topologyInfo.remote_rack_capacity() = remoteCapacity;
        topologyInfo.spine_capacity() = spineCapacity;
        topologyInfo.local_rack_capacity() = loalRackCapacity;
        nhs.emplace_back(makeResolvedNextHop(
            InterfaceID(nhid), "fe80::1", 1 /*weight*/, topologyInfo));
        nhid++;
      }
    }
    normalizer.normalizeWeightsForNexthops(nhs);
    auto numPrunedPaths = [](const auto& nhs, const auto planeId) {
      int numPrunedPaths = 0;
      for (const auto& nh : nhs) {
        if (*nh.topologyInfo()->plane_id() == planeId &&
            nh.adjustedWeight().has_value() && nh.adjustedWeight() == 0) {
          numPrunedPaths++;
        }
      }
      return numPrunedPaths;
    };
    for (int planeId = 0; planeId < numPlanes; planeId++) {
      XLOG(DBG4) << "Checking for planeId: " << planeId
                 << " expected prunes: " << planeIdToExpectedPrunes[planeId];
      EXPECT_EQ(numPrunedPaths(nhs, planeId), planeIdToExpectedPrunes[planeId]);
    }
  }
};

TEST(RibRouteWeightNormalizerTest, GetNumPathsToPruneForZeroFailures) {
  RibRouteWeightNormalizerTest normalizer(6, 6);
  int numFailures = 0;
  RackId dstRack = 1;
  RackId srcRack = 1;
  normalizer.getNumPathsToPrune(numFailures, dstRack, srcRack);
  EXPECT_EQ(normalizer.getNumPathsToPrune(numFailures, dstRack, srcRack), 0);
}

TEST(RibRouteWeightNormalizerTest, GetNumPathsToPruneForNonZeroFailures) {
  RibRouteWeightNormalizerTest normalizer(6, 6);
  int numFailures = 1;
  RackId dstRack;
  RackId srcRack;
  for (dstRack = 1; dstRack <= 6; dstRack++) {
    for (srcRack = 1; srcRack <= 6; srcRack++) {
      int expectedPrunes = srcRack == dstRack ? 1 : 0;
      EXPECT_EQ(
          expectedPrunes,
          normalizer.getNumPathsToPrune(numFailures, dstRack, srcRack));
    }
  }
  numFailures = 7;
  for (dstRack = 1; dstRack <= 6; dstRack++) {
    for (srcRack = 1; srcRack <= 6; srcRack++) {
      int expectedPrunes = srcRack == dstRack ? 2 : 1;
      EXPECT_EQ(
          expectedPrunes,
          normalizer.getNumPathsToPrune(numFailures, dstRack, srcRack));
    }
  }
  numFailures = 36;
  for (dstRack = 1; dstRack <= 6; dstRack++) {
    for (srcRack = 1; srcRack <= 6; srcRack++) {
      int expectedPrunes = 6;
      EXPECT_EQ(
          expectedPrunes,
          normalizer.getNumPathsToPrune(numFailures, dstRack, srcRack));
    }
  }
}

TEST(RibRouteWeightNormalizerTest, verifyNoNexthopPruning) {
  // verify no nexthop pruning for no failures
  std::map<int, int> planeIdToRemoteCapacity = {
      {0, 6}, {1, 6}, {2, 6}, {3, 6}, {4, 6}, {5, 6}, {6, 6}, {7, 6}};
  std::map<int, int> planeIdToSpineCapacity = {
      {0, 36}, {1, 36}, {2, 36}, {3, 36}, {4, 36}, {5, 36}, {6, 36}, {7, 36}};
  std::map<int, int> planeIdToLocalPathCount = {
      {0, 6}, {1, 6}, {2, 6}, {3, 6}, {4, 6}, {5, 6}, {6, 6}, {7, 6}};
  std::map<int, int> planeIdToLocalRackCapacity = {
      {0, 0}, {1, 0}, {2, 0}, {3, 0}, {4, 0}, {5, 0}, {6, 0}, {7, 0}};
  std::map<int, int> planeIdToExpectedPrunes = {
      {0, 0}, {1, 0}, {2, 0}, {3, 0}, {4, 0}, {5, 0}, {6, 0}, {7, 0}};
  RibRouteWeightNormalizerTest::runNextHopPruningTest(
      planeIdToRemoteCapacity,
      planeIdToSpineCapacity,
      planeIdToLocalPathCount,
      planeIdToLocalRackCapacity,
      planeIdToExpectedPrunes);
}

TEST(RibRouteWeightNormalizerTest, verifyNexthopPruningForRackFailures) {
  // racks have failures
  std::map<int, int> planeIdToRemoteCapacity = {
      {0, 6}, {1, 4}, {2, 5}, {3, 3}, {4, 1}, {5, 2}, {6, 5}, {7, 6}};
  std::map<int, int> planeIdToSpineCapacity = {
      {0, 36}, {1, 36}, {2, 36}, {3, 36}, {4, 36}, {5, 36}, {6, 36}, {7, 36}};
  std::map<int, int> planeIdToLocalPathCount = {
      {0, 6}, {1, 6}, {2, 6}, {3, 6}, {4, 6}, {5, 6}, {6, 6}, {7, 6}};
  std::map<int, int> planeIdToLocalRackCapacity = {
      {0, 0}, {1, 0}, {2, 0}, {3, 0}, {4, 0}, {5, 0}, {6, 0}, {7, 0}};
  // expected prune = total links - failure count
  std::map<int, int> planeIdToExpectedPrunes = {
      {0, 0}, {1, 2}, {2, 1}, {3, 3}, {4, 5}, {5, 4}, {6, 1}, {7, 0}};
  RibRouteWeightNormalizerTest::runNextHopPruningTest(
      planeIdToRemoteCapacity,
      planeIdToSpineCapacity,
      planeIdToLocalPathCount,
      planeIdToLocalRackCapacity,
      planeIdToExpectedPrunes);
}

TEST(RibRouteWeightNormalizerTest, verifyNexthopPruningForLocalRackFailures) {
  // routes from pod local racks
  std::map<int, int> planeIdToRemoteCapacity = {
      {0, 0}, {1, 0}, {2, 0}, {3, 0}, {4, 0}, {5, 0}, {6, 0}, {7, 0}};
  std::map<int, int> planeIdToSpineCapacity = {
      {0, 0}, {1, 0}, {2, 0}, {3, 0}, {4, 0}, {5, 0}, {6, 0}, {7, 0}};
  std::map<int, int> planeIdToLocalPathCount = {
      {0, 6}, {1, 6}, {2, 6}, {3, 6}, {4, 6}, {5, 6}, {6, 6}, {7, 6}};
  // pod local rack has failures
  std::map<int, int> planeIdToLocalRackCapacity = {
      {0, 6}, {1, 4}, {2, 5}, {3, 3}, {4, 1}, {5, 2}, {6, 5}, {7, 6}};
  std::map<int, int> planeIdToExpectedPrunes = {
      {0, 0}, {1, 2}, {2, 1}, {3, 3}, {4, 5}, {5, 4}, {6, 1}, {7, 0}};
  RibRouteWeightNormalizerTest::runNextHopPruningTest(
      planeIdToRemoteCapacity,
      planeIdToSpineCapacity,
      planeIdToLocalPathCount,
      planeIdToLocalRackCapacity,
      planeIdToExpectedPrunes);
}

TEST(RibRouteWeightNormalizerTest, verifyNexthopPruningForSpineFailures) {
  std::map<int, int> planeIdToRemoteCapacity = {
      {0, 6}, {1, 6}, {2, 6}, {3, 6}, {4, 6}, {5, 6}, {6, 6}, {7, 6}};
  std::map<int, int> planeIdToSpineCapacity = {
      {0, 36}, {1, 29}, {2, 23}, {3, 33}, {4, 6}, {5, 1}, {6, 36}, {7, 36}};
  std::map<int, int> planeIdToLocalPathCount = {
      {0, 6}, {1, 6}, {2, 6}, {3, 6}, {4, 6}, {5, 6}, {6, 6}, {7, 6}};
  std::map<int, int> planeIdToLocalRackCapacity = {
      {0, 0}, {1, 0}, {2, 0}, {3, 0}, {4, 0}, {5, 0}, {6, 0}, {7, 0}};
  std::map<int, int> planeIdToExpectedPrunes = {
      {0, 0}, {1, 2}, {2, 3}, {3, 1}, {4, 5}, {5, 6}, {6, 0}, {7, 0}};
  RibRouteWeightNormalizerTest::runNextHopPruningTest(
      planeIdToRemoteCapacity,
      planeIdToSpineCapacity,
      planeIdToLocalPathCount,
      planeIdToLocalRackCapacity,
      planeIdToExpectedPrunes);
}

TEST(RibRouteWeightNormalizerTest, verifyNexthopPruningWithLocalFailures) {
  // remote rack failures
  std::map<int, int> planeIdToRemoteCapacity = {
      {0, 6}, {1, 4}, {2, 6}, {3, 6}, {4, 6}, {5, 6}, {6, 6}, {7, 6}};
  // spine has failures
  std::map<int, int> planeIdToSpineCapacity = {
      {0, 36}, {1, 34}, {2, 35}, {3, 36}, {4, 36}, {5, 36}, {6, 36}, {7, 36}};
  // local rack also has failures
  std::map<int, int> planeIdToLocalPathCount = {
      {0, 6}, {1, 5}, {2, 5}, {3, 6}, {4, 6}, {5, 6}, {6, 6}, {7, 6}};
  std::map<int, int> planeIdToLocalRackCapacity = {
      {0, 0}, {1, 0}, {2, 0}, {3, 0}, {4, 0}, {5, 0}, {6, 0}, {7, 0}};
  // expected failure is based on remote rack + spine failures after
  // accounting for local failures
  std::map<int, int> planeIdToExpectedPrunes = {
      {0, 0}, {1, 1}, {2, 0}, {3, 0}, {4, 0}, {5, 0}, {6, 0}, {7, 0}};
  RibRouteWeightNormalizerTest::runNextHopPruningTest(
      planeIdToRemoteCapacity,
      planeIdToSpineCapacity,
      planeIdToLocalPathCount,
      planeIdToLocalRackCapacity,
      planeIdToExpectedPrunes);
}
