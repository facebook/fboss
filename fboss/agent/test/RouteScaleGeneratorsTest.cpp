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
namespace facebook::fboss {

TEST(RouteScaleGeneratorsTest, RSWDistribution) {
  auto mockPlatform = std::make_unique<testing::NiceMock<MockPlatform>>();
  auto routeDistributionGen = utility::RSWRouteScaleGenerator(
      createTestState(mockPlatform.get()), kChunkSize, 2);

  verifyRouteCount(routeDistributionGen, kExtraRoutes, 7947);
  verifyChunking(routeDistributionGen, 7947, kChunkSize);
}

TEST(RouteScaleGeneratorsTest, FSWDistribution) {
  auto mockPlatform = std::make_unique<testing::NiceMock<MockPlatform>>();
  auto routeDistributionGen = utility::FSWRouteScaleGenerator(
      createTestState(mockPlatform.get()), kChunkSize, 2);

  verifyRouteCount(routeDistributionGen, kExtraRoutes, 16000);
  verifyChunking(routeDistributionGen, 16000, kChunkSize);
}

TEST(RouteScaleGeneratorsTest, THAlpmDistribution) {
  auto mockPlatform = std::make_unique<testing::NiceMock<MockPlatform>>();
  auto routeDistributionGen = utility::THAlpmRouteScaleGenerator(
      createTestState(mockPlatform.get()), kChunkSize, 2);

  verifyRouteCount(routeDistributionGen, kExtraRoutes, 33400);
  verifyChunking(routeDistributionGen, 33400, kChunkSize);
}

TEST(RouteScaleGeneratorsTest, HgridDuRouteScaleGenerator) {
  auto mockPlatform = std::make_unique<testing::NiceMock<MockPlatform>>();
  auto routeDistributionGen = utility::HgridDuRouteScaleGenerator(
      createTestState(mockPlatform.get()), kChunkSize, 2);

  verifyRouteCount(routeDistributionGen, kExtraRoutes, 37249);
  verifyChunking(routeDistributionGen, 37249, kChunkSize);
}

TEST(RouteScaleGeneratorsTest, HgridUuRouteScaleGenerator) {
  auto mockPlatform = std::make_unique<testing::NiceMock<MockPlatform>>();
  auto routeDistributionGen = utility::HgridUuRouteScaleGenerator(
      createTestState(mockPlatform.get()), kChunkSize, 2);

  verifyRouteCount(routeDistributionGen, kExtraRoutes, 48350);
  verifyChunking(routeDistributionGen, 48350, kChunkSize);
}

TEST(RouteScaleGeneratorsTest, TurboFSWRouteScaleGenerator) {
  auto mockPlatform = std::make_unique<testing::NiceMock<MockPlatform>>();
  auto routeDistributionGen = utility::TurboFSWRouteScaleGenerator(
      createTestState(mockPlatform.get()), kChunkSize, 64);
  verifyRouteCount(routeDistributionGen, kExtraRoutes, 424);
  verifyChunking(routeDistributionGen, 424, kChunkSize);
}

} // namespace facebook::fboss
