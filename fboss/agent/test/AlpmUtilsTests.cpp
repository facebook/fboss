/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include <gtest/gtest.h>

#include "fboss/agent/AlpmUtils.h"
#include "fboss/agent/state/Route.h"
#include "fboss/agent/state/RouteTable.h"
#include "fboss/agent/state/RouteTableRib.h"
#include "fboss/agent/state/RouteTypes.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/types.h"

using facebook::fboss::Route;
using facebook::fboss::RoutePrefix;
using facebook::fboss::RouterID;
using facebook::fboss::RouteTableRib;
using facebook::fboss::SwitchState;
using folly::IPAddressV4;
using folly::IPAddressV6;

namespace {
template <typename AddrType>
std::shared_ptr<RouteTableRib<AddrType>> getRib(
    const std::shared_ptr<SwitchState>& state) {
  auto routeTable = state->getRouteTables()->getRouteTable(RouterID(0));
  return routeTable->getRib<AddrType>();
}
template <typename AddrType>
std::shared_ptr<Route<AddrType>> getDefaultRoute(
    const std::shared_ptr<SwitchState>& state) {
  auto rib = getRib<AddrType>(state);
  return rib->routes()->getRouteIf(RoutePrefix<AddrType>{AddrType(), 0});
}
} // namespace

namespace facebook::fboss {

TEST(AlpmUtilsTests, DefaultNullRoutesAddedOnEmptyState) {
  auto emptyState = std::make_shared<SwitchState>();
  auto alpmInitState = setupAlpmState(emptyState);
  EXPECT_NE(nullptr, alpmInitState);
  auto v4Rib = getRib<folly::IPAddressV4>(alpmInitState);
  EXPECT_EQ(1, v4Rib->size());
  auto v6Rib = getRib<folly::IPAddressV6>(alpmInitState);
  EXPECT_EQ(1, v6Rib->size());

  auto v4Default = getDefaultRoute<IPAddressV4>(alpmInitState);
  auto v6Default = getDefaultRoute<IPAddressV6>(alpmInitState);
  auto assertNullRoute = [=](const auto& route) {
    EXPECT_NE(nullptr, route);
    EXPECT_TRUE(route->prefix().network.isZero());
    EXPECT_EQ(0, route->prefix().mask);
    EXPECT_TRUE(route->isDrop());
    const auto& fwd = route->getForwardInfo();
    EXPECT_EQ(AdminDistance::MAX_ADMIN_DISTANCE, fwd.getAdminDistance());
    EXPECT_EQ(0, fwd.getNextHopSet().size());
  };
  assertNullRoute(v4Default);
  assertNullRoute(v6Default);
}

TEST(AlpmUtilsTests, DefaultNullRoutesNotReAdded) {
  auto emptyState = std::make_shared<SwitchState>();
  auto alpmInitState = setupAlpmState(emptyState);
  auto alpmAgainState = setupAlpmState(alpmInitState);
  EXPECT_EQ(nullptr, alpmAgainState);
}

TEST(AlpmUtilsTests, minAlpmRouteCount) {
  EXPECT_EQ(2, numMinAlpmRoutes());
}

TEST(AlpmUtilsTests, minAlpmV4RouteCount) {
  EXPECT_EQ(1, numMinAlpmV4Routes());
}

TEST(AlpmUtilsTests, minAlpmV6RouteCount) {
  EXPECT_EQ(1, numMinAlpmV6Routes());
}

} // namespace facebook::fboss
