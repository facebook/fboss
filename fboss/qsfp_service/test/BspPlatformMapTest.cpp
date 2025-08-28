// Copyright 2021-present Facebook. All Rights Reserved.

#include <folly/testing/TestUtil.h>
#include <gtest/gtest.h>
#include "fboss/lib/bsp/icecube800bc/Icecube800bcBspPlatformMapping.h"
#include "fboss/lib/bsp/icetea800bc/Icetea800bcBspPlatformMapping.h"
#include "fboss/lib/bsp/meru400bfu/Meru400bfuBspPlatformMapping.h"
#include "fboss/lib/bsp/meru400bia/Meru400biaBspPlatformMapping.h"
#include "fboss/lib/bsp/meru400biu/Meru400biuBspPlatformMapping.h"
#include "fboss/lib/bsp/minipack3n/Minipack3NBspPlatformMapping.h"
#include "fboss/lib/bsp/montblanc/MontblancBspPlatformMapping.h"

namespace facebook::fboss {

class BspPlatformMapTest : public ::testing::Test {
 public:
};

TEST_F(BspPlatformMapTest, checkNumPimTransceivers) {
  // Check Montblanc
  auto mbBspPlatformMap = MontblancBspPlatformMapping();
  EXPECT_EQ(mbBspPlatformMap.numPims(), 1);
  EXPECT_EQ(mbBspPlatformMap.numTransceivers(), 65);
  // Check Minipack3N
  auto mp3nBspPlatformMap = Minipack3NBspPlatformMapping();
  EXPECT_EQ(mp3nBspPlatformMap.numPims(), 1);
  EXPECT_EQ(mp3nBspPlatformMap.numTransceivers(), 65);
  // Check Kamet
  auto m400bfuBspPlatformMap = Meru400bfuBspPlatformMapping();
  EXPECT_EQ(m400bfuBspPlatformMap.numPims(), 1);
  EXPECT_EQ(m400bfuBspPlatformMap.numTransceivers(), 48);
  // Check Meru400bia
  auto m400biaBspPlatformMap = Meru400biaBspPlatformMapping();
  EXPECT_EQ(m400biaBspPlatformMap.numPims(), 1);
  EXPECT_EQ(m400biaBspPlatformMap.numTransceivers(), 18);
  // Check Makalu
  auto m400biuBspPlatformMap = Meru400biuBspPlatformMapping();
  EXPECT_EQ(m400biuBspPlatformMap.numPims(), 1);
  EXPECT_EQ(m400biuBspPlatformMap.numTransceivers(), 76);
  // Check Icecube800bc
  auto icecube800bcBspPlatformMap = Icecube800bcBspPlatformMapping();
  EXPECT_EQ(icecube800bcBspPlatformMap.numPims(), 1);
  EXPECT_EQ(icecube800bcBspPlatformMap.numTransceivers(), 65);
  // Check Icetea800bc
  auto icetea800bcBspPlatformMap = Icetea800bcBspPlatformMapping();
  EXPECT_EQ(icetea800bcBspPlatformMap.numPims(), 1);
  EXPECT_EQ(icetea800bcBspPlatformMap.numTransceivers(), 33);
}

} // namespace facebook::fboss
