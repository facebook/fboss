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
#include "fboss/agent/test/HwTestHandle.h"
#include "fboss/agent/test/RouteGeneratorTestUtils.h"
#include "fboss/agent/test/TestUtils.h"

namespace {
auto constexpr kChunkSize = 4000;
} // namespace
namespace facebook::fboss {

TEST(RouteScaleGeneratorsTest, RSWDistribution) {
  auto cfg = getTestConfig();
  auto handle = createTestHandle(&cfg);
  auto routeDistributionGen = utility::RSWRouteScaleGenerator(
      handle->getSw()->getState(), kChunkSize, 2);

  verifyRouteCount(routeDistributionGen, kExtraRoutes, 7947);
  verifyChunking(routeDistributionGen, 7947, kChunkSize);
}

TEST(RouteScaleGeneratorsTest, FSWDistribution) {
  auto cfg = getTestConfig();
  auto handle = createTestHandle(&cfg);
  auto routeDistributionGen = utility::FSWRouteScaleGenerator(
      handle->getSw()->getState(), kChunkSize, 2);

  verifyRouteCount(routeDistributionGen, kExtraRoutes, 16000);
  verifyChunking(routeDistributionGen, 16000, kChunkSize);
}

TEST(RouteScaleGeneratorsTest, THAlpmDistribution) {
  auto cfg = getTestConfig();
  auto handle = createTestHandle(&cfg);
  auto routeDistributionGen = utility::THAlpmRouteScaleGenerator(
      handle->getSw()->getState(), kChunkSize, 2);

  verifyRouteCount(routeDistributionGen, kExtraRoutes, 33400);
  verifyChunking(routeDistributionGen, 33400, kChunkSize);
}

TEST(RouteScaleGeneratorsTest, HgridDuRouteScaleGenerator) {
  auto cfg = getTestConfig();
  auto handle = createTestHandle(&cfg);
  auto routeDistributionGen = utility::HgridDuRouteScaleGenerator(
      handle->getSw()->getState(), kChunkSize, 2);

  verifyRouteCount(routeDistributionGen, kExtraRoutes, 37249);
  verifyChunking(routeDistributionGen, 37249, kChunkSize);
}

TEST(RouteScaleGeneratorsTest, HgridUuRouteScaleGenerator) {
  auto cfg = getTestConfig();
  auto handle = createTestHandle(&cfg);
  auto routeDistributionGen = utility::HgridUuRouteScaleGenerator(
      handle->getSw()->getState(), kChunkSize, 2);

  verifyRouteCount(routeDistributionGen, kExtraRoutes, 48350);
  verifyChunking(routeDistributionGen, 48350, kChunkSize);
}

TEST(RouteScaleGeneratorsTest, TurboFSWRouteScaleGenerator) {
  auto cfg = getTestConfig();
  auto handle = createTestHandle(&cfg);
  auto routeDistributionGen = utility::TurboFSWRouteScaleGenerator(
      handle->getSw()->getState(), kChunkSize, 64);
  verifyRouteCount(routeDistributionGen, kExtraRoutes, 7803);
  // Chunking in TurboFSWRouteScaleGenerator is dictated by label distribution
  // which is bespoke to TurboFSWRouteScaleGenerator, so skip verifyChunking for
  // this generator
}

} // namespace facebook::fboss
