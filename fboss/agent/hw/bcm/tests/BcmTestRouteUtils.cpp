/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/hw/bcm/tests/BcmTestRouteUtils.h"

#include "fboss/agent/LacpTypes.h"
#include "fboss/agent/state/RouteUpdater.h"

#include <gtest/gtest.h>

namespace facebook::fboss::utility {

namespace {
const auto kTestClient = ClientID(1001);
}

std::shared_ptr<RouteTableMap> addRoute(
    std::shared_ptr<RouteTableMap> routeTables,
    folly::CIDRNetwork network,
    folly::IPAddress nexthop) {
  RouteUpdater updater(routeTables);
  updater.addRoute(
      kRouter0,
      network.first,
      network.second,
      kTestClient,
      RouteNextHopEntry(UnresolvedNextHop(nexthop, ECMP_WEIGHT), DISTANCE));
  auto newRouteTables = updater.updateDone();
  EXPECT_NE(newRouteTables, nullptr);

  return newRouteTables != nullptr ? newRouteTables : routeTables;
}

std::shared_ptr<RouteTableMap> addRoute(
    std::shared_ptr<RouteTableMap> routeTables,
    folly::CIDRNetwork network,
    const std::vector<folly::IPAddress>& nexthopAddrs) {
  RouteUpdater updater(routeTables);
  RouteNextHopEntry::NextHopSet nexthops;

  for (const auto& nexthopAddr : nexthopAddrs) {
    nexthops.emplace(UnresolvedNextHop(nexthopAddr, ECMP_WEIGHT));
  }

  updater.addRoute(
      kRouter0,
      network.first,
      network.second,
      kTestClient,
      RouteNextHopEntry(std::move(nexthops), DISTANCE));
  auto newRouteTables = updater.updateDone();
  EXPECT_NE(newRouteTables, nullptr);

  return newRouteTables != nullptr ? newRouteTables : routeTables;
}

std::shared_ptr<RouteTableMap> deleteRoute(
    std::shared_ptr<RouteTableMap> routeTables,
    folly::CIDRNetwork network) {
  RouteUpdater updater(routeTables);
  updater.delRoute(kRouter0, network.first, network.second, kTestClient);
  auto newRouteTables = updater.updateDone();

  return newRouteTables != nullptr ? newRouteTables : routeTables;
}

} // namespace facebook::fboss::utility
