/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/ApplyThriftConfig.h"

#include "fboss/agent/hw/mock/MockPlatform.h"
#include "fboss/agent/test/RouteDistributionGenerator.h"
#include "fboss/agent/test/RouteGeneratorTestUtils.h"

namespace facebook {
namespace fboss {

TEST(RouteScaleGeneratorsTest, v4AndV6DistributionSingleChunk) {
  auto mockPlatform = std::make_unique<testing::NiceMock<MockPlatform>>();
  auto switchStates = utility::RouteDistributionGenerator(
                          createTestState(mockPlatform.get()),
                          {
                              {65, 5},
                              {127, 5},
                          },
                          {
                              {25, 5},
                              {32, 5},
                          },
                          4000,
                          2)
                          .get();

  EXPECT_EQ(getRouteCount(switchStates.back()), 20 + kExtraRoutes);
  verifyChunking(switchStates, 20, 4000);
}

TEST(RouteScaleGeneratorsTest, v4AndV6DistributionMultipleChunks) {
  auto mockPlatform = std::make_unique<testing::NiceMock<MockPlatform>>();
  auto switchStates = utility::RouteDistributionGenerator(
                          createTestState(mockPlatform.get()),
                          {
                              {65, 5},
                              {127, 5},
                          },
                          {
                              {25, 5},
                              {32, 5},
                          },
                          10,
                          2)
                          .get();

  EXPECT_EQ(getRouteCount(switchStates.back()), 20 + kExtraRoutes);
  verifyChunking(switchStates, 20, 10);
}

TEST(RouteScaleGeneratorsTest, v4AndV6DistributionChunksSpillOverMaskLens) {
  auto mockPlatform = std::make_unique<testing::NiceMock<MockPlatform>>();
  auto switchStates = utility::RouteDistributionGenerator(
                          createTestState(mockPlatform.get()),
                          {
                              {65, 3},
                              {127, 5},
                          },
                          {
                              {25, 3},
                              {32, 7},
                          },
                          4,
                          2)
                          .get();

  EXPECT_EQ(getRouteCount(switchStates.back()), 18 + kExtraRoutes);
  verifyChunking(switchStates, 18, 4);
}

TEST(
    RouteScaleGeneratorsTest,
    v4AndV6DistributionChunksSpillOverAddressFamilies) {
  auto mockPlatform = std::make_unique<testing::NiceMock<MockPlatform>>();
  auto switchStates = utility::RouteDistributionGenerator(
                          createTestState(mockPlatform.get()),
                          {
                              {65, 5},
                              {127, 6},
                          },
                          {
                              {25, 4},
                              {32, 5},
                          },
                          5,
                          2)
                          .get();

  EXPECT_EQ(getRouteCount(switchStates.back()), 20 + kExtraRoutes);
  verifyChunking(switchStates, 20, 5);
}
} // namespace fboss
} // namespace facebook
