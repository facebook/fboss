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

using namespace facebook::fboss;

class RibRouteWeightNormalizerTest : public RibRouteWeightNormalizer {
 public:
  RibRouteWeightNormalizerTest(int numRacks, int numPlanePathsPerRack)
      : RibRouteWeightNormalizer(numRacks, numPlanePathsPerRack) {}
};

TEST(RibRouteWeightNormalizerTest, GetNumPathsToPruneForZeroFailures) {
  RibRouteWeightNormalizerTest normalizer(6, 6);
  int numFailures = 0;
  RackId dstRack = 0;
  RackId srcRack = 0;
  normalizer.getNumPathsToPrune(numFailures, dstRack, srcRack);
  EXPECT_EQ(normalizer.getNumPathsToPrune(numFailures, dstRack, srcRack), 0);
}

TEST(RibRouteWeightNormalizerTest, GetNumPathsToPruneForNonZeroFailures) {
  RibRouteWeightNormalizerTest normalizer(6, 6);
  int numFailures = 1;
  RackId dstRack;
  RackId srcRack;
  for (dstRack = 0; dstRack < 6; dstRack++) {
    for (srcRack = 0; srcRack < 6; srcRack++) {
      int expectedPrunes = srcRack == dstRack ? 1 : 0;
      EXPECT_EQ(
          expectedPrunes,
          normalizer.getNumPathsToPrune(numFailures, dstRack, srcRack));
    }
  }
  numFailures = 7;
  for (dstRack = 0; dstRack < 6; dstRack++) {
    for (srcRack = 0; srcRack < 6; srcRack++) {
      int expectedPrunes = srcRack == dstRack ? 2 : 1;
      EXPECT_EQ(
          expectedPrunes,
          normalizer.getNumPathsToPrune(numFailures, dstRack, srcRack));
    }
  }
  numFailures = 36;
  for (dstRack = 0; dstRack < 6; dstRack++) {
    for (srcRack = 0; srcRack < 6; srcRack++) {
      int expectedPrunes = 6;
      EXPECT_EQ(
          expectedPrunes,
          normalizer.getNumPathsToPrune(numFailures, dstRack, srcRack));
    }
  }
}
