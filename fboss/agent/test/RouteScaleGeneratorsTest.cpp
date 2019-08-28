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

namespace {
auto constexpr kChunkSize = 4000;
}
namespace facebook {
namespace fboss {
TEST(RouteScaleGeneratorsTest, FSWDistribution) {
  auto mockPlatform = std::make_unique<testing::NiceMock<MockPlatform>>();
  auto routeDistributionSwitchStatesGen =
      utility::FSWRouteScaleGenerator(
          createTestState(mockPlatform.get()), kChunkSize, 2)
          .routeDistributionGenerator();

  verifyRouteCount(routeDistributionSwitchStatesGen, kExtraRoutes, 16000);
  verifyChunking(routeDistributionSwitchStatesGen, 16000, kChunkSize);
}

TEST(RouteScaleGeneratorsTest, THAlpmDistribution) {
  auto mockPlatform = std::make_unique<testing::NiceMock<MockPlatform>>();
  auto routeDistributionSwitchStatesGen =
      utility::THAlpmRouteScaleGenerator(
          createTestState(mockPlatform.get()), kChunkSize, 2)
          .routeDistributionGenerator();

  verifyRouteCount(routeDistributionSwitchStatesGen, kExtraRoutes, 33400);
  verifyChunking(routeDistributionSwitchStatesGen, 33400, kChunkSize);
}

TEST(RouteScaleGeneratorsTest, HgridDuRouteScaleGenerator) {
  auto mockPlatform = std::make_unique<testing::NiceMock<MockPlatform>>();
  auto routeDistributionSwitchStatesGen =
      utility::HgridDuRouteScaleGenerator(
          createTestState(mockPlatform.get()), kChunkSize, 2)
          .routeDistributionGenerator();

  verifyRouteCount(routeDistributionSwitchStatesGen, kExtraRoutes, 37249);
  verifyChunking(routeDistributionSwitchStatesGen, 37249, kChunkSize);
}

TEST(RouteScaleGeneratorsTest, HgridUuRouteScaleGenerator) {
  auto mockPlatform = std::make_unique<testing::NiceMock<MockPlatform>>();
  auto routeDistributionSwitchStatesGen =
      utility::HgridUuRouteScaleGenerator(
          createTestState(mockPlatform.get()), kChunkSize, 2)
          .routeDistributionGenerator();

  verifyRouteCount(routeDistributionSwitchStatesGen, kExtraRoutes, 48350);
  verifyChunking(routeDistributionSwitchStatesGen, 48350, kChunkSize);
}
} // namespace fboss
} // namespace facebook
