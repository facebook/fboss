/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/FbossError.h"
#include "fboss/agent/Utils.h"
#include "fboss/agent/rib/NetworkToRouteMap.h"
#include "fboss/agent/state/RouteNextHop.h"

#include "fboss/agent/rib/RouteUpdater.h"

#include <folly/IPAddress.h>
#include <folly/dynamic.h>
#include <folly/logging/xlog.h>

#include <gtest/gtest.h>
#include <string>
#include <vector>

namespace {
using namespace facebook::fboss;
const ClientID kClientA = ClientID(1001);
const ClientID kClientB = ClientID(1002);
} // namespace

using folly::IPAddress;
using folly::IPAddressV4;
using folly::IPAddressV6;

constexpr AdminDistance kDistance = AdminDistance::MAX_ADMIN_DISTANCE;

namespace {

RouteNextHopSet makeNextHops(
    std::vector<std::string> ipsAsStrings,
    std::optional<LabelForwardingAction> mplsAction = std::nullopt) {
  RouteNextHopSet nhops;
  for (const std::string& ipAsString : ipsAsStrings) {
    nhops.emplace(
        UnresolvedNextHop(IPAddress(ipAsString), ECMP_WEIGHT, mplsAction));
  }
  return nhops;
}

template <typename AddrT>
void EXPECT_ROUTES_MATCH(
    const NetworkToRouteMap<AddrT>* routesA,
    const NetworkToRouteMap<AddrT>* routesB) {
  EXPECT_EQ(routesA->size(), routesB->size());
  for (const auto& entryA : *routesA) {
    auto routeA = entryA.value();
    auto prefixA = routeA->prefix();

    auto iterB = routesB->exactMatch(prefixA.network(), prefixA.mask());
    ASSERT_NE(routesB->end(), iterB);
    auto routeB = iterB->value();

    EXPECT_TRUE(routeB->isSame(routeA.get()));
  }
}

void EXPECT_MPLS_ROUTES_MATCH(
    const LabelToRouteMap* routesA,
    const LabelToRouteMap* routesB) {
  EXPECT_EQ(routesA->size(), routesB->size());
  for (const auto& entryA : *routesA) {
    auto routeA = entryA.second;
    auto labelA = routeA->prefix().label();

    auto iterB = routesB->find(labelA);
    ASSERT_NE(routesB->end(), iterB);
    auto routeB = iterB->second;

    EXPECT_TRUE(routeB->isSame(routeA.get()));
  }
}

} // namespace

namespace facebook::fboss {

TEST(Route, removeRoutesForClient) {
  IPv4NetworkToRouteMap v4Routes;
  IPv6NetworkToRouteMap v6Routes;

  // 2 different nexthops
  RouteNextHopSet nhop1 = makeNextHops({"1.1.1.10"}); // resolved by intf 1
  RouteNextHopSet nhop2 = makeNextHops({"2.2.2.10"}); // resolved by intf 2
  // 4 prefixes
  RouteV4::Prefix r1{IPAddressV4("10.1.1.0"), 24};
  RouteV4::Prefix r2{IPAddressV4("20.1.1.0"), 24};
  RouteV6::Prefix r3{IPAddressV6("1001::0"), 48};
  RouteV6::Prefix r4{IPAddressV6("2001::0"), 48};

  RibRouteUpdater u2(&v4Routes, &v6Routes);
  u2.update<RibRouteUpdater::RouteEntry, folly::CIDRNetwork>(
      kClientA,
      {
          {{r1.network(), r1.mask()}, RouteNextHopEntry(nhop1, kDistance)},
          {{r3.network(), r3.mask()}, RouteNextHopEntry(nhop1, kDistance)},
      },
      {},
      false);
  u2.update<RibRouteUpdater::RouteEntry, folly::CIDRNetwork>(
      kClientB,
      {
          {{r2.network(), r2.mask()}, RouteNextHopEntry(nhop2, kDistance)},
          {{r4.network(), r4.mask()}, RouteNextHopEntry(nhop2, kDistance)},
      },
      {},
      false);
}

TEST(Route, serializeRouteTable) {
  IPv4NetworkToRouteMap v4Routes;
  IPv6NetworkToRouteMap v6Routes;
  LabelToRouteMap mplsRoutes;

  // 2 different nexthops
  RouteNextHopSet nhop1 = makeNextHops({"1.1.1.10"}); // resolved by intf 1
  RouteNextHopSet nhop2 = makeNextHops({"2.2.2.10"}); // resolved by intf 2
  // 4 prefixes
  RouteV4::Prefix r1{IPAddressV4("10.1.1.0"), 24};
  RouteV4::Prefix r2{IPAddressV4("20.1.1.0"), 24};
  RouteV6::Prefix r3{IPAddressV6("1001::0"), 48};
  RouteV6::Prefix r4{IPAddressV6("2001::0"), 48};

  std::optional<RouteCounterID> counterID1("route.counter.0");
  std::optional<RouteCounterID> counterID2("route.counter.1");
  std::optional<cfg::AclLookupClass> classID(
      cfg::AclLookupClass::DST_CLASS_L3_DPR);

  RibRouteUpdater u2(&v4Routes, &v6Routes, &mplsRoutes);
  u2.update<RibRouteUpdater::RouteEntry, folly::CIDRNetwork>(
      kClientA,
      {
          {{r1.network(), r1.mask()}, RouteNextHopEntry(nhop1, kDistance)},
          {{r2.network(), r2.mask()},
           RouteNextHopEntry(nhop2, kDistance, counterID1, classID)},
          {{r3.network(), r3.mask()}, RouteNextHopEntry(nhop1, kDistance)},
          {{r4.network(), r4.mask()},
           RouteNextHopEntry(nhop2, kDistance, counterID2)},
      },
      {},
      false);

  RouteNextHopSet mplsNhop1 = makeNextHops(
      {"1.1.1.10"},
      LabelForwardingAction(LabelForwardingAction::LabelForwardingType::PHP));
  RouteNextHopSet mplsNhop2 = makeNextHops(
      {"2.2.2.10"},
      LabelForwardingAction(LabelForwardingAction::LabelForwardingType::PHP));
  u2.update<RibRouteUpdater::MplsRouteEntry, LabelID>(
      kClientA,
      {
          {LabelID(100), RouteNextHopEntry(mplsNhop1, kDistance)},
          {LabelID(101), RouteNextHopEntry(mplsNhop2, kDistance)},
      },
      {},
      false);
}

} // namespace facebook::fboss
