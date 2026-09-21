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
      int spinePruneStepCount = 1,
      bool enableFpfCapacityPruning = false)
      : RibRouteWeightNormalizer(
            numRacks,
            numPlanePathsPerRack,
            rackId,
            numSpineFailuresToSkip,
            spinePruneStepCount,
            enableFpfCapacityPruning) {}

  // Public wrapper to access protected method for testing
  void testNormalizeWeightsForNexthops(std::vector<ResolvedNextHop>& nhs) {
    normalizeWeightsForNexthops(nhs);
  }

  static void runNextHopPruningTest(
      std::map<int, int>& planeIdToRemoteCapacity,
      std::map<int, int>& planeIdToSpineCapacity,
      std::map<int, int>& planeIdToLocalPathCount,
      std::map<int, int>& planeIdToLocalRackCapacity,
      std::map<int, int>& planeIdToExpectedPrunes,
      int numSpineFailuresToSkip = 0,
      int spinePruneStepCount = 1) {
    int numRacks = 6;
    int numPlanes = 8;
    int numPlanePathsPerRack = 6;
    RibRouteWeightNormalizerTest normalizer(
        numRacks,
        numPlanePathsPerRack,
        1,
        numSpineFailuresToSkip,
        spinePruneStepCount);
    std::vector<ResolvedNextHop> nhs;
    int nhid = 0;
    for (int planeId = 0; planeId < numPlanes; planeId++) {
      for (int localPath = 0; localPath < planeIdToLocalPathCount[planeId];
           localPath++) {
        int remoteCapacity = planeIdToRemoteCapacity[planeId];
        int spineCapacity = planeIdToSpineCapacity[planeId];
        int localRackCapacity = planeIdToLocalRackCapacity[planeId];
        NetworkTopologyInformation topologyInfo;
        topologyInfo.rack_id() = 1;
        topologyInfo.plane_id() = planeId;
        topologyInfo.remote_rack_capacity() = remoteCapacity;
        topologyInfo.spine_capacity() = spineCapacity;
        topologyInfo.local_rack_capacity() = localRackCapacity;
        nhs.emplace_back(makeResolvedNextHop(
            InterfaceID(nhid), "fe80::1", 1 /*weight*/, topologyInfo));
        nhid++;
      }
    }
    normalizer.testNormalizeWeightsForNexthops(nhs);
    auto numPrunedPaths = [](const auto& nhs, const auto planeId) {
      int count = 0;
      for (const auto& nh : nhs) {
        if (*nh.topologyInfo()->plane_id() == planeId &&
            nh.adjustedWeight().has_value() && nh.adjustedWeight() == 0) {
          count++;
        }
      }
      return count;
    };
    for (int planeId = 0; planeId < numPlanes; planeId++) {
      XLOG(DBG4) << "Checking for planeId: " << planeId
                 << " expected prunes: " << planeIdToExpectedPrunes[planeId];
      EXPECT_EQ(numPrunedPaths(nhs, planeId), planeIdToExpectedPrunes[planeId]);
    }
  }

  // FPF pruning is scoped per STSW (spine_id). Each STSW's prune count is
  // derived from the local GTSW->STSW path count (number of nexthops toward the
  // STSW) vs the STSW->remote GTSW capacity carried in remote_rack_capacity,
  // independent of every other STSW.
  static void runFpfNextHopPruningTest(
      const std::map<int, int>& spineIdToRemoteCapacity,
      const std::map<int, int>& spineIdToLocalPathCount,
      const std::map<int, int>& spineIdToExpectedPrunes,
      int numSpineFailuresToSkip = 0,
      int spinePruneStepCount = 1) {
    int numStsws = 8;
    RibRouteWeightNormalizerTest normalizer(
        6 /*numRacks*/,
        6 /*numPlanePathsPerRack*/,
        1 /*rackId*/,
        numSpineFailuresToSkip,
        spinePruneStepCount,
        true /*enableFpfCapacityPruning*/);
    std::vector<ResolvedNextHop> nhs;
    int nhid = 0;
    for (int spineId = 0; spineId < numStsws; spineId++) {
      for (int localPath = 0; localPath < spineIdToLocalPathCount.at(spineId);
           localPath++) {
        NetworkTopologyInformation topologyInfo;
        topologyInfo.spine_id() = spineId;
        // remote_rack_capacity carries the STSW->remote GTSW capacity in FPF
        topologyInfo.remote_rack_capacity() =
            spineIdToRemoteCapacity.at(spineId);
        nhs.emplace_back(makeResolvedNextHop(
            InterfaceID(nhid), "fe80::1", 1 /*weight*/, topologyInfo));
        nhid++;
      }
    }
    normalizer.testNormalizeWeightsForNexthops(nhs);
    auto numPrunedPaths = [](const auto& nhs, const auto spineId) {
      int count = 0;
      for (const auto& nh : nhs) {
        if (*nh.topologyInfo()->spine_id() == spineId &&
            nh.adjustedWeight().has_value() && nh.adjustedWeight() == 0) {
          count++;
        }
      }
      return count;
    };
    for (int spineId = 0; spineId < numStsws; spineId++) {
      XLOG(DBG4) << "Checking for spineId: " << spineId
                 << " expected prunes: " << spineIdToExpectedPrunes.at(spineId);
      EXPECT_EQ(
          numPrunedPaths(nhs, spineId), spineIdToExpectedPrunes.at(spineId));
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

TEST(RibRouteWeightNormalizerTest, verifySpineFailuresToSkipFunctionality) {
  // Test spine failures with numSpineFailuresToSkip = 2
  std::map<int, int> planeIdToRemoteCapacity = {
      {0, 6}, {1, 6}, {2, 6}, {3, 6}, {4, 6}, {5, 6}, {6, 6}, {7, 6}};
  std::map<int, int> planeIdToSpineCapacity = {
      {0, 36}, {1, 35}, {2, 34}, {3, 33}, {4, 32}, {5, 31}, {6, 30}, {7, 27}};
  std::map<int, int> planeIdToLocalPathCount = {
      {0, 6}, {1, 6}, {2, 6}, {3, 6}, {4, 6}, {5, 6}, {6, 6}, {7, 6}};
  std::map<int, int> planeIdToLocalRackCapacity = {
      {0, 0}, {1, 0}, {2, 0}, {3, 0}, {4, 0}, {5, 0}, {6, 0}, {7, 0}};
  // Expected prunes with numSpineFailuresToSkip = 2:
  // plane 0: 0 failures → 0 effective failures → 0 prunes
  // plane 1: 1 failure → 0 effective failures (skipped) → 0 prunes
  // plane 2: 2 failures → 0 effective failures (skipped) → 0 prunes
  // plane 3: 3 failures → 1 effective failure → 1 prune per src/dst
  // plane 4: 4 failures → 2 effective failures → 1 prune per src/dst
  // plane 5: 5 failures → 3 effective failures → 1 prune per src/dst
  // plane 6: 6 failures → 4 effective failures → 1 prune per src/dst
  // plane 7: 9 failures → 7 effective failures → 2 prunes per src/dst
  std::map<int, int> planeIdToExpectedPrunes = {
      {0, 0}, {1, 0}, {2, 0}, {3, 1}, {4, 1}, {5, 1}, {6, 1}, {7, 2}};
  RibRouteWeightNormalizerTest::runNextHopPruningTest(
      planeIdToRemoteCapacity,
      planeIdToSpineCapacity,
      planeIdToLocalPathCount,
      planeIdToLocalRackCapacity,
      planeIdToExpectedPrunes,
      2); // numSpineFailuresToSkip = 2
}

TEST(RibRouteWeightNormalizerTest, verifySpineFailureStepCountFunctionality) {
  // Test spine failures with numSpineFailuresToSkip = 2 and spinePruneStepCount
  // = 3 This test validates both skip and step count functionality working
  // together
  std::map<int, int> planeIdToRemoteCapacity = {
      {0, 6}, {1, 6}, {2, 6}, {3, 6}, {4, 6}, {5, 6}, {6, 6}, {7, 6}};
  std::map<int, int> planeIdToSpineCapacity = {
      {0, 36}, {1, 35}, {2, 34}, {3, 33}, {4, 32}, {5, 31}, {6, 28}, {7, 27}};
  std::map<int, int> planeIdToLocalPathCount = {
      {0, 6}, {1, 6}, {2, 6}, {3, 6}, {4, 6}, {5, 6}, {6, 6}, {7, 6}};
  std::map<int, int> planeIdToLocalRackCapacity = {
      {0, 0}, {1, 0}, {2, 0}, {3, 0}, {4, 0}, {5, 0}, {6, 0}, {7, 0}};

  // Expected prunes with numSpineFailuresToSkip = 2 and spinePruneStepCount =
  // 3: Step count logic: after skipping failures, group remaining failures into
  // steps of 3 Formula: stepGroup = (effectiveFailures - 1) / stepCount; result
  // = stepGroup * stepCount + 1
  //
  // plane 0: 0 failures → 0 effective failures → 0 prunes
  // plane 1: 1 failure → 0 effective failures (1-2=0, max with 0) → 0 prunes
  // plane 2: 2 failures → 0 effective failures (2-2=0, max with 0) → 0 prunes
  // plane 3: 3 failures → 1 effective failure (3-2=1) → step group 0 → 1 prune
  // plane 4: 4 failures → 2 effective failures (4-2=2) → step group 0 → 1 prune
  // plane 5: 5 failures → 3 effective failures (5-2=3) → step group 0 → 1 prune
  // plane 6: 8 failures → 6 effective failures (8-2=6) → step group 1 → 4
  // prunes -> 1 prune per src/dst
  // plane 7: 9 failures → 7 effective failures
  // (9-2=7) → step group 2 → 7 prunes -> 2 prunes per src/dst
  std::map<int, int> planeIdToExpectedPrunes = {
      {0, 0}, {1, 0}, {2, 0}, {3, 1}, {4, 1}, {5, 1}, {6, 1}, {7, 2}};
  RibRouteWeightNormalizerTest::runNextHopPruningTest(
      planeIdToRemoteCapacity,
      planeIdToSpineCapacity,
      planeIdToLocalPathCount,
      planeIdToLocalRackCapacity,
      planeIdToExpectedPrunes,
      2, // numSpineFailuresToSkip = 2
      3); // spinePruneStepCount = 3
}

// Test for parameter validation in getNumPathsToPrune method
TEST(RibRouteWeightNormalizerTest, verifyParameterValidation) {
  RibRouteWeightNormalizerTest normalizer(6, 6);

  // Test invalid dstRack
  EXPECT_THROW(
      normalizer.getNumPathsToPrune(0, 0, 1), FbossError); // dstRack == 0
  EXPECT_THROW(
      normalizer.getNumPathsToPrune(0, 7, 1), FbossError); // dstRack > numRacks

  // Test invalid srcRack
  EXPECT_THROW(
      normalizer.getNumPathsToPrune(0, 1, 0), FbossError); // srcRack == 0
  EXPECT_THROW(
      normalizer.getNumPathsToPrune(0, 1, 7), FbossError); // srcRack > numRacks

  // Test invalid numFailures
  EXPECT_THROW(
      normalizer.getNumPathsToPrune(-1, 1, 1), FbossError); // negative failures
  EXPECT_THROW(
      normalizer.getNumPathsToPrune(1000, 1, 1),
      FbossError); // too large failures

  // Test negative rack_id (security fix: negative values must be rejected)
  EXPECT_THROW(
      normalizer.getNumPathsToPrune(0, -1, 1), FbossError); // negative dstRack
  EXPECT_THROW(
      normalizer.getNumPathsToPrune(0, 1, -1), FbossError); // negative srcRack
  EXPECT_THROW(
      normalizer.getNumPathsToPrune(0, -5, -5), FbossError); // both negative

  // Valid parameter tests - these should not throw

  // Constructor valid parameters
  EXPECT_NO_THROW(
      RibRouteWeightNormalizerTest(1, 1, 1, 0, 1)); // minimum values
  EXPECT_NO_THROW(RibRouteWeightNormalizerTest(6, 6, 1, 0, 1)); // min rack ID
  EXPECT_NO_THROW(RibRouteWeightNormalizerTest(6, 6, 6, 0, 1)); // max rack ID
  EXPECT_NO_THROW(
      RibRouteWeightNormalizerTest(100, 50, 50, 10, 5)); // large values

  // getNumPathsToPrune valid parameters
  EXPECT_NO_THROW(
      normalizer.getNumPathsToPrune(0, 1, 1)); // min rack IDs and zero failures
  EXPECT_NO_THROW(
      normalizer.getNumPathsToPrune(0, 6, 6)); // max rack IDs and zero failures
  EXPECT_NO_THROW(
      normalizer.getNumPathsToPrune(10, 3, 4)); // moderate failure count

  // Test edge cases that should work
  EXPECT_NO_THROW(
      normalizer.getNumPathsToPrune(36, 1, 1)); // max valid failures (6*6)
}

TEST(
    RibRouteWeightNormalizerTest,
    verifyFpfNoPruningWhenRemoteCapacitySufficient) {
  // remote capacity (remote_rack_capacity) >= local path count => no
  // oversubscription
  std::map<int, int> spineIdToRemoteCapacity = {
      {0, 8}, {1, 8}, {2, 8}, {3, 8}, {4, 8}, {5, 8}, {6, 8}, {7, 8}};
  std::map<int, int> spineIdToLocalPathCount = {
      {0, 8}, {1, 6}, {2, 8}, {3, 4}, {4, 8}, {5, 8}, {6, 8}, {7, 8}};
  std::map<int, int> spineIdToExpectedPrunes = {
      {0, 0}, {1, 0}, {2, 0}, {3, 0}, {4, 0}, {5, 0}, {6, 0}, {7, 0}};
  RibRouteWeightNormalizerTest::runFpfNextHopPruningTest(
      spineIdToRemoteCapacity,
      spineIdToLocalPathCount,
      spineIdToExpectedPrunes);
}

TEST(RibRouteWeightNormalizerTest, verifyFpfPruningForRemoteSpineFailures) {
  // local GTSW fully healthy (8 nexthops per STSW), remote STSW->GTSW capacity
  // reduced. prune = local path count - remote_rack_capacity
  std::map<int, int> spineIdToRemoteCapacity = {
      {0, 8}, {1, 7}, {2, 6}, {3, 5}, {4, 4}, {5, 2}, {6, 8}, {7, 8}};
  std::map<int, int> spineIdToLocalPathCount = {
      {0, 8}, {1, 8}, {2, 8}, {3, 8}, {4, 8}, {5, 8}, {6, 8}, {7, 8}};
  std::map<int, int> spineIdToExpectedPrunes = {
      {0, 0}, {1, 1}, {2, 2}, {3, 3}, {4, 4}, {5, 6}, {6, 0}, {7, 0}};
  RibRouteWeightNormalizerTest::runFpfNextHopPruningTest(
      spineIdToRemoteCapacity,
      spineIdToLocalPathCount,
      spineIdToExpectedPrunes);
}

TEST(RibRouteWeightNormalizerTest, verifyFpfPruningDiscountsLocalFailures) {
  // fewer local nexthops shrink the excess since prune = local count - remote
  std::map<int, int> spineIdToRemoteCapacity = {
      {0, 6}, {1, 6}, {2, 5}, {3, 8}, {4, 8}, {5, 8}, {6, 8}, {7, 8}};
  std::map<int, int> spineIdToLocalPathCount = {
      {0, 8}, {1, 6}, {2, 7}, {3, 8}, {4, 8}, {5, 8}, {6, 8}, {7, 8}};
  std::map<int, int> spineIdToExpectedPrunes = {
      {0, 2}, {1, 0}, {2, 2}, {3, 0}, {4, 0}, {5, 0}, {6, 0}, {7, 0}};
  RibRouteWeightNormalizerTest::runFpfNextHopPruningTest(
      spineIdToRemoteCapacity,
      spineIdToLocalPathCount,
      spineIdToExpectedPrunes);
}

TEST(RibRouteWeightNormalizerTest, verifyFpfSkipAndStepCount) {
  // numSpineFailuresToSkip = 2, spinePruneStepCount = 3
  // effective = max((local path count - remote_rack_capacity) - 2, 0); then
  // quantized
  std::map<int, int> spineIdToRemoteCapacity = {
      {0, 8}, {1, 7}, {2, 6}, {3, 5}, {4, 4}, {5, 3}, {6, 0}, {7, 1}};
  std::map<int, int> spineIdToLocalPathCount = {
      {0, 8}, {1, 8}, {2, 8}, {3, 8}, {4, 8}, {5, 8}, {6, 8}, {7, 8}};
  // spine0: excess 0 -> 0
  // spine1: excess 1 -> 1-2<0 -> 0
  // spine2: excess 2 -> 0 -> 0
  // spine3: excess 3 -> 1 -> step group 0 -> 1
  // spine4: excess 4 -> 2 -> step group 0 -> 1
  // spine5: excess 5 -> 3 -> step group 0 -> 1
  // spine6: excess 8 -> 6 -> step group 1 -> 4
  // spine7: excess 7 -> 5 -> step group 1 -> 4
  std::map<int, int> spineIdToExpectedPrunes = {
      {0, 0}, {1, 0}, {2, 0}, {3, 1}, {4, 1}, {5, 1}, {6, 4}, {7, 4}};
  RibRouteWeightNormalizerTest::runFpfNextHopPruningTest(
      spineIdToRemoteCapacity,
      spineIdToLocalPathCount,
      spineIdToExpectedPrunes,
      2 /*numSpineFailuresToSkip*/,
      3 /*spinePruneStepCount*/);
}
