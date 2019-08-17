/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/test/RouteScaleGenerators.h"
#include "fboss/agent/hw/mock/MockPlatform.h"
#include "fboss/agent/test/RouteGeneratorTestUtils.h"

#include <gtest/gtest.h>

namespace facebook {
namespace fboss {
TEST(RouteScaleGeneratorsTest, FSWDistribution) {
  auto mockPlatform = std::make_unique<testing::NiceMock<MockPlatform>>();
  auto switchStates = utility::FSWRouteScaleGenerator(
                          createTestState(mockPlatform.get()), 4000, 2)
                          .get();

  EXPECT_EQ(getRouteCount(switchStates.back()), 16000 + kExtraRoutes);
  verifyChunking(switchStates, 16000, 4000);
}

TEST(RouteScaleGeneratorsTest, THAlpmDistribution) {
  auto mockPlatform = std::make_unique<testing::NiceMock<MockPlatform>>();
  auto switchStates = utility::THAlpmRouteScaleGenerator(
                          createTestState(mockPlatform.get()), 4000, 2)
                          .get();

  EXPECT_EQ(getRouteCount(switchStates.back()), 33400 + kExtraRoutes);
  verifyChunking(switchStates, 33400, 4000);
}

TEST(RouteScaleGeneratorsTest, HgridDuRouteScaleGenerator) {
  auto mockPlatform = std::make_unique<testing::NiceMock<MockPlatform>>();
  auto switchStates = utility::HgridDuRouteScaleGenerator(
                          createTestState(mockPlatform.get()), 4000, 2)
                          .get();

  EXPECT_EQ(getRouteCount(switchStates.back()), 37249 + kExtraRoutes);
  verifyChunking(switchStates, 37249, 4000);
}

TEST(RouteScaleGeneratorsTest, HgridUuRouteScaleGenerator) {
  auto mockPlatform = std::make_unique<testing::NiceMock<MockPlatform>>();
  auto switchStates = utility::HgridUuRouteScaleGenerator(
                          createTestState(mockPlatform.get()), 4000, 2)
                          .get();

  EXPECT_EQ(getRouteCount(switchStates.back()), 48350 + kExtraRoutes);
  verifyChunking(switchStates, 48350, 4000);
}
} // namespace fboss
} // namespace facebook
