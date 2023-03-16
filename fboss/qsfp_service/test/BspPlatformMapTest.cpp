// Copyright 2021-present Facebook. All Rights Reserved.

#include <folly/experimental/TestUtil.h>
#include <gtest/gtest.h>
#include "fboss/lib/CommonFileUtils.h"
#include "fboss/lib/bsp/meru400bfu/Meru400bfuBspPlatformMapping.h"
#include "fboss/lib/bsp/meru400biu/Meru400biuBspPlatformMapping.h"
#include "fboss/lib/bsp/montblanc/MontblancBspPlatformMapping.h"
#include "fboss/qsfp_service/platforms/wedge/tests/MockWedgeManager.h"

namespace facebook::fboss {

class BspPlatformMapTest : public ::testing::Test {
 public:
};

TEST_F(BspPlatformMapTest, checkNumPimTransceivers) {
  // Check Montblanc
  auto mbBspPlatformMap = MontblancBspPlatformMapping();
  EXPECT_EQ(mbBspPlatformMap.numPims(), 1);
  EXPECT_EQ(mbBspPlatformMap.numTransceivers(), 64);
  // Check Kamet
  auto m400bfuBspPlatformMap = Meru400bfuBspPlatformMapping();
  EXPECT_EQ(m400bfuBspPlatformMap.numPims(), 1);
  EXPECT_EQ(m400bfuBspPlatformMap.numTransceivers(), 48);
  // Check Makalu
  auto m400biuBspPlatformMap = Meru400biuBspPlatformMapping();
  EXPECT_EQ(m400biuBspPlatformMap.numPims(), 1);
  EXPECT_EQ(m400biuBspPlatformMap.numTransceivers(), 76);
}

} // namespace facebook::fboss
