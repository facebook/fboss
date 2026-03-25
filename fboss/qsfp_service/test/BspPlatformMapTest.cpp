// Copyright 2021-present Facebook. All Rights Reserved.

#include <folly/testing/TestUtil.h>
#include <gtest/gtest.h>
#include "fboss/lib/bsp/icecube800bc/Icecube800bcBspPlatformMapping.h"
#include "fboss/lib/bsp/icetea800bc/Icetea800bcBspPlatformMapping.h"
#include "fboss/lib/bsp/minipack3bta/Minipack3BTABspPlatformMapping.h"
#include "fboss/lib/bsp/minipack3n/Minipack3NBspPlatformMapping.h"
#include "fboss/lib/bsp/montblanc/MontblancBspPlatformMapping.h"
#include "fboss/lib/bsp/tahansb800bc/Tahansb800bcBspPlatformMapping.h"
#include "fboss/lib/bsp/wedge800bact/Wedge800BACTBspPlatformMapping.h"
#include "fboss/lib/bsp/wedge800cact/Wedge800CACTBspPlatformMapping.h"

namespace facebook::fboss {

class BspPlatformMapTest : public ::testing::Test {
 public:
};

TEST_F(BspPlatformMapTest, checkNumPimTransceivers) {
  // Check Montblanc
  auto mbBspPlatformMap = MontblancBspPlatformMapping();
  EXPECT_EQ(mbBspPlatformMap.numPims(), 1);
  EXPECT_EQ(mbBspPlatformMap.numTransceivers(), 65);
  // Check Minkpack3BTA
  auto mp3btaBspPlatformMap = Minipack3BTABspPlatformMapping();
  EXPECT_EQ(mp3btaBspPlatformMap.numPims(), 1);
  EXPECT_EQ(mp3btaBspPlatformMap.numTransceivers(), 65);
  // Check Minipack3N
  auto mp3nBspPlatformMap = Minipack3NBspPlatformMapping();
  EXPECT_EQ(mp3nBspPlatformMap.numPims(), 1);
  EXPECT_EQ(mp3nBspPlatformMap.numTransceivers(), 65);
  // Check Icecube800bc
  auto icecube800bcBspPlatformMap = Icecube800bcBspPlatformMapping();
  EXPECT_EQ(icecube800bcBspPlatformMap.numPims(), 1);
  EXPECT_EQ(icecube800bcBspPlatformMap.numTransceivers(), 65);
  // Check Icetea800bc
  auto icetea800bcBspPlatformMap = Icetea800bcBspPlatformMapping();
  EXPECT_EQ(icetea800bcBspPlatformMap.numPims(), 1);
  EXPECT_EQ(icetea800bcBspPlatformMap.numTransceivers(), 33);
  // Check Tahansb800bc
  auto tahansb800bcBspPlatformMap = Tahansb800bcBspPlatformMapping();
  EXPECT_EQ(tahansb800bcBspPlatformMap.numPims(), 1);
  EXPECT_EQ(tahansb800bcBspPlatformMap.numTransceivers(), 9);
  // Check WEDGE800BACT
  auto wedge800bactBspPlatformMap = Wedge800BACTBspPlatformMapping();
  EXPECT_EQ(wedge800bactBspPlatformMap.numPims(), 1);
  EXPECT_EQ(wedge800bactBspPlatformMap.numTransceivers(), 33);
  // Check WEDGE800CACT
  auto wedge800cactBspPlatformMap = Wedge800CACTBspPlatformMapping();
  EXPECT_EQ(wedge800cactBspPlatformMap.numPims(), 1);
  EXPECT_EQ(wedge800cactBspPlatformMap.numTransceivers(), 33);
}

} // namespace facebook::fboss
