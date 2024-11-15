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
#include "fboss/agent/test/HwTestHandle.h"
#include "fboss/agent/test/RouteDistributionGenerator.h"
#include "fboss/agent/test/RouteGeneratorTestUtils.h"
#include "fboss/agent/test/TestUtils.h"

namespace facebook::fboss {

TEST(RouteDistributionGeneratorsTest, v4AndV6DistributionSingleChunk) {
  auto cfg = getTestConfig();
  auto handle = createTestHandle(&cfg);
  auto routeDistributionSwitchStatesGen = utility::RouteDistributionGenerator(
      handle->getSw()->getState(),
      {
          {65, 5},
          {127, 5},
      },
      {
          {25, 5},
          {32, 5},
      },
      4000,
      2);
  verifyRouteCount(routeDistributionSwitchStatesGen, kExtraRoutes, 20);
  verifyChunking(routeDistributionSwitchStatesGen, 20, 4000);
}

TEST(RouteDistributionGeneratorsTest, v4AndV6DistributionMultipleChunks) {
  auto cfg = getTestConfig();
  auto handle = createTestHandle(&cfg);
  auto routeDistributionSwitchStatesGen = utility::RouteDistributionGenerator(
      handle->getSw()->getState(),
      {
          {65, 5},
          {127, 5},
      },
      {
          {25, 5},
          {32, 5},
      },
      10,
      2);
  verifyRouteCount(routeDistributionSwitchStatesGen, kExtraRoutes, 20);
  verifyChunking(routeDistributionSwitchStatesGen, 20, 10);
}

TEST(
    RouteDistributionGeneratorsTest,
    v4AndV6DistributionChunksSpillOverMaskLens) {
  auto cfg = getTestConfig();
  auto handle = createTestHandle(&cfg);
  auto routeDistributionSwitchStatesGen = utility::RouteDistributionGenerator(
      handle->getSw()->getState(),
      {
          {65, 3},
          {127, 5},
      },
      {
          {25, 3},
          {32, 7},
      },
      4,
      2);

  verifyRouteCount(routeDistributionSwitchStatesGen, kExtraRoutes, 18);
  verifyChunking(routeDistributionSwitchStatesGen, 18, 4);
}

TEST(
    RouteDistributionGeneratorsTest,
    v4AndV6DistributionChunksSpillOverAddressFamilies) {
  auto cfg = getTestConfig();
  auto handle = createTestHandle(&cfg);
  auto routeDistributionSwitchStatesGen = utility::RouteDistributionGenerator(
      handle->getSw()->getState(),
      {
          {65, 5},
          {127, 6},
      },
      {
          {25, 4},
          {32, 5},
      },
      5,
      2);

  verifyRouteCount(routeDistributionSwitchStatesGen, kExtraRoutes, 20);
  verifyChunking(routeDistributionSwitchStatesGen, 20, 5);
}

TEST(RouteDistributionGeneratorsTest, emptyV4Distribution) {
  auto cfg = getTestConfig();
  auto handle = createTestHandle(&cfg);
  auto routeDistributionSwitchStatesGen = utility::RouteDistributionGenerator(
      handle->getSw()->getState(),
      {
          {65, 5},
      },
      {},
      5,
      2);

  verifyRouteCount(routeDistributionSwitchStatesGen, kExtraRoutes, 5);
  verifyChunking(routeDistributionSwitchStatesGen, 5, 5);
}

TEST(RouteDistributionGeneratorsTest, emptyV6Distribution) {
  auto cfg = getTestConfig();
  auto handle = createTestHandle(&cfg);
  auto routeDistributionSwitchStatesGen = utility::RouteDistributionGenerator(
      handle->getSw()->getState(), {}, {{24, 5}}, 5, 2);

  verifyRouteCount(routeDistributionSwitchStatesGen, kExtraRoutes, 5);
  verifyChunking(routeDistributionSwitchStatesGen, 5, 5);
}

TEST(RouteDistributionGeneratorsTest, emptyV4AndV6Distribution) {
  auto cfg = getTestConfig();
  auto handle = createTestHandle(&cfg);
  auto routeDistributionSwitchStatesGen = utility::RouteDistributionGenerator(
      handle->getSw()->getState(), {}, {}, 5, 2);

  verifyRouteCount(routeDistributionSwitchStatesGen, kExtraRoutes, 0);
  verifyChunking(routeDistributionSwitchStatesGen, 0, 5);
}

} // namespace facebook::fboss
