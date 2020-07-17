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
#include "fboss/agent/rib/RouteNextHop.h"
#include "fboss/agent/rib/RouteNextHopEntry.h"
#include "fboss/agent/rib/RouteTypes.h"
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
const ClientID kClientC = ClientID(1003);
const ClientID kClientD = ClientID(1004);
const ClientID kClientE = ClientID(1005);
} // namespace

using rib::IPv4NetworkToRouteMap;
using rib::IPv6NetworkToRouteMap;
using rib::Route;
using rib::RouteNextHopEntry;
using rib::RouteNextHopSet;
using rib::RouteUpdater;

using folly::IPAddress;
using folly::IPAddressV4;
using folly::IPAddressV6;

constexpr AdminDistance kDistance = AdminDistance::MAX_ADMIN_DISTANCE;

namespace {
// TODO(samank): move helpers into test fixture
template <typename AddressT>
rib::Route<AddressT>* getRoute(
    rib::NetworkToRouteMap<AddressT>& routes,
    std::string prefixAsString) {
  auto prefix = rib::RoutePrefix<AddressT>::fromString(prefixAsString);
  auto it = routes.exactMatch(prefix.network, prefix.mask);

  return &(it->value());
}

rib::RouteNextHopSet makeNextHops(std::vector<std::string> ipsAsStrings) {
  rib::RouteNextHopSet nhops;
  for (const std::string& ipAsString : ipsAsStrings) {
    nhops.emplace(
        rib::UnresolvedNextHop(IPAddress(ipAsString), rib::ECMP_WEIGHT));
  }
  return nhops;
}

template <typename AddrT>
void EXPECT_FWD_INFO(
    const rib::Route<AddrT>* route,
    InterfaceID interfaceID,
    std::string ipAsString) {
  const auto& fwds = route->getForwardInfo().getNextHopSet();
  EXPECT_EQ(1, fwds.size());
  const auto& fwd = *fwds.begin();
  EXPECT_EQ(interfaceID, fwd.intf());
  EXPECT_EQ(IPAddress(ipAsString), fwd.addr());
}

template <typename AddrT>
void EXPECT_RESOLVED(const rib::Route<AddrT>* route) {
  EXPECT_TRUE(route->isResolved());
  EXPECT_FALSE(route->isUnresolvable());
  EXPECT_FALSE(route->needResolve());
}

template <typename AddrT>
void EXPECT_ROUTES_MATCH(
    const rib::NetworkToRouteMap<AddrT>* routesA,
    const rib::NetworkToRouteMap<AddrT>* routesB) {
  EXPECT_EQ(routesA->size(), routesB->size());
  for (const auto& entryA : *routesA) {
    auto routeA = entryA.value();
    auto prefixA = routeA.prefix();

    auto iterB = routesB->exactMatch(prefixA.network, prefixA.mask);
    ASSERT_NE(routesB->end(), iterB);
    auto routeB = iterB->value();

    EXPECT_TRUE(routeB.isSame(&routeA));
  }
}

template <typename AddressT>
rib::Route<AddressT>* longestMatch(
    rib::NetworkToRouteMap<AddressT>& routes,
    AddressT address) {
  auto it = routes.longestMatch(address, address.bitCount());
  CHECK(it != routes.end());
  return &(it->value());
}

} // namespace

namespace facebook::fboss::rib {

/* The following method updates the RIB with routes that would result from a
 * config with the following interfaces:
 * 1.1.1.1/24 and 1::1/48 on Interface 1
 * 2.2.2.2/24 and 2::1/48 on Interface 2
 * 3.3.3.3/24 and 3::1/48 on Interface 3
 * 4.4.4.4/24 and 4::1/48 on Interface 4
 */
void configRoutes(
    IPv4NetworkToRouteMap* v4Routes,
    IPv6NetworkToRouteMap* v6Routes) {
  RouteUpdater updater(v4Routes, v6Routes);

  updater.addInterfaceRoute(
      folly::IPAddress("1.1.1.1"),
      24,
      folly::IPAddress("1.1.1.1"),
      InterfaceID(1));
  updater.addInterfaceRoute(
      folly::IPAddress("1::1"), 48, folly::IPAddress("1::1"), InterfaceID(1));

  updater.addInterfaceRoute(
      folly::IPAddress("2.2.2.2"),
      24,
      folly::IPAddress("2.2.2.2"),
      InterfaceID(2));
  updater.addInterfaceRoute(
      folly::IPAddress("2::1"), 48, folly::IPAddress("2::1"), InterfaceID(2));

  updater.addInterfaceRoute(
      folly::IPAddress("3.3.3.3"),
      24,
      folly::IPAddress("3.3.3.3"),
      InterfaceID(3));
  updater.addInterfaceRoute(
      folly::IPAddress("3::1"), 48, folly::IPAddress("3::1"), InterfaceID(3));

  updater.addInterfaceRoute(
      folly::IPAddress("4.4.4.4"),
      24,
      folly::IPAddress("4.4.4.4"),
      InterfaceID(4));
  updater.addInterfaceRoute(
      folly::IPAddress("4::1"), 48, folly::IPAddress("4::1"), InterfaceID(4));

  updater.addLinkLocalRoutes();

  updater.updateDone();
}

TEST(Route, resolve) {
  IPv4NetworkToRouteMap v4Routes;
  IPv6NetworkToRouteMap v6Routes;

  configRoutes(&v4Routes, &v6Routes);
  {
    RouteUpdater u1(&v4Routes, &v6Routes);

    // resolved by Interface 1
    RouteNextHopSet nexthops1 = makeNextHops({"1.1.1.10"});
    u1.addRoute(
        IPAddress("1.1.3.0"),
        24,
        kClientA,
        RouteNextHopEntry(nexthops1, kDistance));

    // resolved by 1.1.3/24
    RouteNextHopSet nexthops2 = makeNextHops({"1.1.3.10"});
    u1.addRoute(
        IPAddress("8.8.8.0"),
        24,
        kClientA,
        RouteNextHopEntry(nexthops2, kDistance));

    u1.updateDone();

    auto r21 = getRoute(v4Routes, "1.1.3.0/24");
    EXPECT_RESOLVED(r21);
    EXPECT_FALSE(r21->isConnected());

    auto r22 = getRoute(v4Routes, "8.8.8.0/24");
    EXPECT_RESOLVED(r22);
    EXPECT_FALSE(r22->isConnected());
    // r21 and r22 are different routes
    EXPECT_NE(r21, r22);
    EXPECT_NE(r21->prefix(), r22->prefix());
    // check the forwarding info
    RouteNextHopSet expFwd2;
    expFwd2.emplace(
        ResolvedNextHop(IPAddress("1.1.1.10"), InterfaceID(1), ECMP_WEIGHT));
    EXPECT_EQ(expFwd2, r21->getForwardInfo().getNextHopSet());
    EXPECT_EQ(expFwd2, r22->getForwardInfo().getNextHopSet());
  }
  // recursive lookup loop
  {
    // create a route table w/ the following 3 routes
    // 1. 30/8 -> 20.1.1.1
    // 2. 20/8 -> 10.1.1.1
    // 3. 10/8 -> 30.1.1.1
    // The above 3 routes causes lookup loop, which should result in
    // all unresolvable.
    RouteUpdater u1(&v4Routes, &v6Routes);
    u1.addRoute(
        IPAddress("30.0.0.0"),
        8,
        kClientA,
        RouteNextHopEntry(makeNextHops({"20.1.1.1"}), kDistance));
    u1.addRoute(
        IPAddress("20.0.0.0"),
        8,
        kClientA,
        RouteNextHopEntry(makeNextHops({"10.1.1.1"}), kDistance));
    u1.addRoute(
        IPAddress("10.0.0.0"),
        8,
        kClientA,
        RouteNextHopEntry(makeNextHops({"30.1.1.1"}), kDistance));
    u1.updateDone();

    auto verifyPrefix = [&](std::string prefixStr) {
      auto route = getRoute(v4Routes, prefixStr);
      EXPECT_FALSE(route->isResolved());
      EXPECT_TRUE(route->isUnresolvable());
      EXPECT_FALSE(route->isConnected());
      EXPECT_FALSE(route->needResolve());
      EXPECT_FALSE(route->isProcessing());
    };
    verifyPrefix("10.0.0.0/8");
    verifyPrefix("20.0.0.0/8");
    verifyPrefix("30.0.0.0/8");
  }
  // recursive lookup across 2 updates
  {
    RouteUpdater u1(&v4Routes, &v6Routes);
    RouteNextHopSet nexthops1 = makeNextHops({"50.0.0.1"});
    u1.addRoute(
        IPAddress("40.0.0.0"),
        8,
        kClientA,
        RouteNextHopEntry(nexthops1, kDistance));

    u1.updateDone();

    // 40.0.0.0/8 should be unresolved
    auto r21 = getRoute(v4Routes, "40.0.0.0/8");
    EXPECT_FALSE(r21->isResolved());
    EXPECT_TRUE(r21->isUnresolvable());
    EXPECT_FALSE(r21->isConnected());
    EXPECT_FALSE(r21->needResolve());

    // Resolve 50.0.0.1 this should also resolve 40.0.0.0/8
    RouteUpdater u2(&v4Routes, &v6Routes);
    u2.addRoute(
        IPAddress("50.0.0.0"),
        8,
        kClientA,
        RouteNextHopEntry(makeNextHops({"1.1.1.1"}), kDistance));
    u2.updateDone();

    // 40.0.0.0/8 should be resolved
    auto r31 = getRoute(v4Routes, "40.0.0.0/8");
    EXPECT_RESOLVED(r31);
    EXPECT_FALSE(r31->isConnected());

    // 50.0.0.1/32 should be resolved
    auto r31NextHops = r31->getBestEntry().second->getNextHopSet();
    EXPECT_EQ(1, r31NextHops.size());
    auto r32 = longestMatch(v4Routes, r31NextHops.begin()->addr().asV4());
    EXPECT_RESOLVED(r32);
    EXPECT_FALSE(r32->isConnected());

    // 50.0.0.0/8 should be resolved
    auto r33 = getRoute(v4Routes, "50.0.0.0/8");
    EXPECT_RESOLVED(r33);
    EXPECT_FALSE(r33->isConnected());
  }
}

TEST(Route, resolveDropToCPUMix) {
  IPv4NetworkToRouteMap v4Routes;
  IPv6NetworkToRouteMap v6Routes;

  configRoutes(&v4Routes, &v6Routes);

  // add a DROP route and a ToCPU route
  RouteUpdater u1(&v4Routes, &v6Routes);
  u1.addRoute(
      IPAddress("11.1.1.0"),
      24,
      kClientA,
      RouteNextHopEntry(RouteForwardAction::DROP, kDistance));
  u1.addRoute(
      IPAddress("22.1.1.0"),
      24,
      kClientA,
      RouteNextHopEntry(RouteForwardAction::TO_CPU, kDistance));
  // then, add a route for 4 nexthops. One to each interface, one
  // to the DROP and one to the ToCPU
  RouteNextHopSet nhops = makeNextHops({"1.1.1.10", // interface 1
                                        "2.2.2.10", // interface 2
                                        "11.1.1.10", // drop
                                        "22.1.1.10"}); // to cpu
  u1.addRoute(
      IPAddress("8.8.8.0"), 24, kClientA, RouteNextHopEntry(nhops, kDistance));
  u1.updateDone();

  {
    auto r2 = getRoute(v4Routes, "8.8.8.0/24");
    EXPECT_RESOLVED(r2);
    EXPECT_FALSE(r2->isDrop());
    EXPECT_FALSE(r2->isToCPU());
    EXPECT_FALSE(r2->isConnected());
    const auto& fwd = r2->getForwardInfo();
    EXPECT_EQ(RouteForwardAction::NEXTHOPS, fwd.getAction());
    EXPECT_EQ(2, fwd.getNextHopSet().size());
  }

  // now update the route with just DROP and ToCPU, expect ToCPU to win
  RouteUpdater u2(&v4Routes, &v6Routes);
  RouteNextHopSet nhops2 = makeNextHops({"11.1.1.10", // drop
                                         "22.1.1.10"}); // trap to cpu
  u2.addRoute(
      IPAddress("8.8.8.0"), 24, kClientA, RouteNextHopEntry(nhops2, kDistance));
  u2.updateDone();

  {
    auto r2 = getRoute(v4Routes, "8.8.8.0/24");
    EXPECT_RESOLVED(r2);
    EXPECT_FALSE(r2->isDrop());
    EXPECT_TRUE(r2->isToCPU());
    EXPECT_FALSE(r2->isConnected());
    const auto& fwd = r2->getForwardInfo();
    EXPECT_EQ(RouteForwardAction::TO_CPU, fwd.getAction());
    EXPECT_EQ(0, fwd.getNextHopSet().size());
  }

  // now update the route with just DROP
  RouteUpdater u3(&v4Routes, &v6Routes);
  RouteNextHopSet nhops3 = makeNextHops({"11.1.1.10"}); // drop
  u3.addRoute(
      IPAddress("8.8.8.0"), 24, kClientA, RouteNextHopEntry(nhops3, kDistance));

  {
    auto r2 = getRoute(v4Routes, "8.8.8.0/24");
    EXPECT_RESOLVED(r2);
    EXPECT_TRUE(r2->isDrop());
    EXPECT_FALSE(r2->isToCPU());
    EXPECT_FALSE(r2->isConnected());
    const auto& fwd = r2->getForwardInfo();
    EXPECT_EQ(RouteForwardAction::DROP, fwd.getAction());
    EXPECT_EQ(0, fwd.getNextHopSet().size());
  }
}

// Testing add and delete ECMP routes
TEST(Route, addDel) {
  IPv4NetworkToRouteMap v4Routes;
  IPv6NetworkToRouteMap v6Routes;

  configRoutes(&v4Routes, &v6Routes);

  RouteNextHopSet nexthops = makeNextHops({"1.1.1.10", // interface 1
                                           "2::2", // interface 2
                                           "1.1.2.10"}); // un-resolvable
  RouteNextHopSet nexthops2 = makeNextHops({"1.1.3.10", // un-resolvable
                                            "11:11::1"}); // un-resolvable

  RouteUpdater u1(&v4Routes, &v6Routes);
  u1.addRoute(
      IPAddress("10.1.1.1"),
      24,
      kClientA,
      RouteNextHopEntry(nexthops, kDistance));
  u1.addRoute(
      IPAddress("2001::1"),
      48,
      kClientA,
      RouteNextHopEntry(nexthops, kDistance));
  u1.updateDone();

  // v4 route
  auto r2 = getRoute(v4Routes, "10.1.1.0/24");
  EXPECT_RESOLVED(r2);
  EXPECT_FALSE(r2->isDrop());
  EXPECT_FALSE(r2->isToCPU());
  EXPECT_FALSE(r2->isConnected());
  // v6 route
  auto r2v6 = getRoute(v6Routes, "2001::0/48");
  EXPECT_RESOLVED(r2v6);
  EXPECT_FALSE(r2v6->isDrop());
  EXPECT_FALSE(r2v6->isToCPU());
  EXPECT_FALSE(r2v6->isConnected());
  // forwarding info
  EXPECT_EQ(RouteForwardAction::NEXTHOPS, r2->getForwardInfo().getAction());
  EXPECT_EQ(RouteForwardAction::NEXTHOPS, r2v6->getForwardInfo().getAction());
  const auto& fwd2 = r2->getForwardInfo().getNextHopSet();
  const auto& fwd2v6 = r2v6->getForwardInfo().getNextHopSet();
  EXPECT_EQ(2, fwd2.size());
  EXPECT_EQ(2, fwd2v6.size());
  RouteNextHopSet expFwd2;
  expFwd2.emplace(
      ResolvedNextHop(IPAddress("1.1.1.10"), InterfaceID(1), ECMP_WEIGHT));
  expFwd2.emplace(
      ResolvedNextHop(IPAddress("2::2"), InterfaceID(2), ECMP_WEIGHT));
  EXPECT_EQ(expFwd2, fwd2);
  EXPECT_EQ(expFwd2, fwd2v6);

  // change the nexthops of the V4 route
  RouteUpdater u2(&v4Routes, &v6Routes);
  u2.addRoute(
      IPAddress("10.1.1.1"),
      24,
      kClientA,
      RouteNextHopEntry(nexthops2, kDistance));
  u2.updateDone();

  auto r3 = getRoute(v4Routes, "10.1.1.0/24");
  ASSERT_NE(nullptr, r3);
  EXPECT_FALSE(r3->isResolved());
  EXPECT_TRUE(r3->isUnresolvable());
  EXPECT_FALSE(r3->isConnected());
  EXPECT_FALSE(r3->needResolve());

  // re-add the same route does not cause change
  RouteUpdater u3(&v4Routes, &v6Routes);
  u3.addRoute(
      IPAddress("10.1.1.1"),
      24,
      kClientA,
      RouteNextHopEntry(nexthops2, kDistance));
  u3.updateDone();

  // now delete the V4 route
  RouteUpdater u4(&v4Routes, &v6Routes);
  u4.delRoute(IPAddress("10.1.1.1"), 24, kClientA);
  u4.updateDone();

  auto it = v4Routes.exactMatch(IPAddressV4("10.1.1.0"), 24);
  EXPECT_EQ(v4Routes.end(), it);

  // change an old route to punt to CPU, add a new route to DROP
  RouteUpdater u5(&v4Routes, &v6Routes);
  u5.addRoute(
      IPAddress("10.1.1.0"),
      24,
      kClientA,
      RouteNextHopEntry(RouteForwardAction::TO_CPU, kDistance));
  u5.addRoute(
      IPAddress("10.1.2.0"),
      24,
      kClientA,
      RouteNextHopEntry(RouteForwardAction::DROP, kDistance));
  u5.updateDone();

  auto r6_1 = getRoute(v4Routes, "10.1.1.0/24");
  EXPECT_RESOLVED(r6_1);
  EXPECT_FALSE(r6_1->isConnected());
  EXPECT_TRUE(r6_1->isToCPU());
  EXPECT_FALSE(r6_1->isDrop());
  EXPECT_EQ(RouteForwardAction::TO_CPU, r6_1->getForwardInfo().getAction());

  auto r6_2 = getRoute(v4Routes, "10.1.2.0/24");
  EXPECT_RESOLVED(r6_2);
  EXPECT_FALSE(r6_2->isConnected());
  EXPECT_FALSE(r6_2->isToCPU());
  EXPECT_TRUE(r6_2->isDrop());
  EXPECT_EQ(RouteForwardAction::DROP, r6_2->getForwardInfo().getAction());
}

// Test interface routes
TEST(Route, Interface) {
  IPv4NetworkToRouteMap v4Routes;
  IPv6NetworkToRouteMap v6Routes;

  {
    RouteUpdater updater(&v4Routes, &v6Routes);

    updater.addInterfaceRoute(
        folly::IPAddress("1.1.1.1"),
        24,
        folly::IPAddress("1.1.1.1"),
        InterfaceID(1));
    updater.addInterfaceRoute(
        folly::IPAddress("1::1"), 48, folly::IPAddress("1::1"), InterfaceID(1));

    updater.addInterfaceRoute(
        folly::IPAddress("2.2.2.2"),
        24,
        folly::IPAddress("2.2.2.2"),
        InterfaceID(2));
    updater.addInterfaceRoute(
        folly::IPAddress("2::1"), 48, folly::IPAddress("2::1"), InterfaceID(2));

    updater.addLinkLocalRoutes();

    updater.updateDone();
  }

  EXPECT_EQ(2, v4Routes.size());
  EXPECT_EQ(3, v6Routes.size());
  // verify the ipv4 interface route
  {
    auto rt = getRoute(v4Routes, "1.1.1.0/24");
    EXPECT_RESOLVED(rt);
    EXPECT_TRUE(rt->isConnected());
    EXPECT_FALSE(rt->isToCPU());
    EXPECT_FALSE(rt->isDrop());
    EXPECT_EQ(RouteForwardAction::NEXTHOPS, rt->getForwardInfo().getAction());
    EXPECT_FWD_INFO(rt, InterfaceID(1), "1.1.1.1");
  }
  // verify the ipv6 interface route
  {
    auto rt = getRoute(v6Routes, "2::0/48");
    EXPECT_RESOLVED(rt);
    EXPECT_TRUE(rt->isConnected());
    EXPECT_FALSE(rt->isToCPU());
    EXPECT_FALSE(rt->isDrop());
    EXPECT_EQ(RouteForwardAction::NEXTHOPS, rt->getForwardInfo().getAction());
    EXPECT_FWD_INFO(rt, InterfaceID(2), "2::1");
  }

  {
    // verify v6 link local route
    auto rt = getRoute(v6Routes, "fe80::/64");
    EXPECT_RESOLVED(rt);
    EXPECT_FALSE(rt->isConnected());
    EXPECT_TRUE(rt->isToCPU());
    EXPECT_EQ(RouteForwardAction::TO_CPU, rt->getForwardInfo().getAction());
    const auto& fwds = rt->getForwardInfo().getNextHopSet();
    EXPECT_EQ(0, fwds.size());
  }

  // swap the interface addresses which causes route change
  {
    RouteUpdater updater(&v4Routes, &v6Routes);

    updater.addInterfaceRoute(
        folly::IPAddress("1.1.1.1"),
        24,
        folly::IPAddress("1.1.1.1"),
        InterfaceID(2));
    updater.addInterfaceRoute(
        folly::IPAddress("1::1"), 48, folly::IPAddress("1::1"), InterfaceID(2));

    updater.addInterfaceRoute(
        folly::IPAddress("2.2.2.2"),
        24,
        folly::IPAddress("2.2.2.2"),
        InterfaceID(1));
    updater.addInterfaceRoute(
        folly::IPAddress("2::1"), 48, folly::IPAddress("2::1"), InterfaceID(1));

    updater.updateDone();
  }

  EXPECT_EQ(2, v4Routes.size());
  EXPECT_EQ(3, v6Routes.size());

  // verify the ipv4 route
  {
    auto rt = getRoute(v4Routes, "1.1.1.0/24");
    EXPECT_FWD_INFO(rt, InterfaceID(2), "1.1.1.1");
  }
  // verify the ipv6 route
  {
    auto rt = getRoute(v6Routes, "2::0/48");
    EXPECT_FWD_INFO(rt, InterfaceID(1), "2::1");
  }
}

// Test interface routes when we have more than one address per
// address family in an interface
TEST(Route, MultipleAddressInterface) {
  IPv4NetworkToRouteMap v4Routes;
  IPv6NetworkToRouteMap v6Routes;

  {
    RouteUpdater updater(&v4Routes, &v6Routes);

    updater.addInterfaceRoute(
        folly::IPAddress("1.1.1.1"),
        24,
        folly::IPAddress("1.1.1.1"),
        InterfaceID(1));
    updater.addInterfaceRoute(
        folly::IPAddress("1.1.1.1"),
        24,
        folly::IPAddress("1.1.1.2"),
        InterfaceID(1));
    updater.addInterfaceRoute(
        folly::IPAddress("1::1"), 48, folly::IPAddress("1::1"), InterfaceID(1));
    updater.addInterfaceRoute(
        folly::IPAddress("1::1"), 48, folly::IPAddress("1::2"), InterfaceID(1));

    updater.addLinkLocalRoutes();

    updater.updateDone();
  }

  EXPECT_EQ(1, v4Routes.size());
  EXPECT_EQ(2, v6Routes.size());
  // verify the ipv4 route
  {
    auto rt = getRoute(v4Routes, "1.1.1.0/24");
    EXPECT_RESOLVED(rt);
    EXPECT_TRUE(rt->isConnected());
    EXPECT_FALSE(rt->isToCPU());
    EXPECT_FALSE(rt->isDrop());
    EXPECT_EQ(RouteForwardAction::NEXTHOPS, rt->getForwardInfo().getAction());
    EXPECT_FWD_INFO(rt, InterfaceID(1), "1.1.1.2");
  }
  // verify the ipv6 route
  {
    auto rt = getRoute(v6Routes, "1::0/48");
    EXPECT_RESOLVED(rt);
    EXPECT_TRUE(rt->isConnected());
    EXPECT_FALSE(rt->isToCPU());
    EXPECT_FALSE(rt->isDrop());
    EXPECT_EQ(RouteForwardAction::NEXTHOPS, rt->getForwardInfo().getAction());
    EXPECT_FWD_INFO(rt, InterfaceID(1), "1::2");
  }
}

// Test interface + static routes
TEST(Route, InterfaceAndStatic) {
  IPv4NetworkToRouteMap v4Routes;
  IPv6NetworkToRouteMap v6Routes;

  {
    RouteUpdater updater(&v4Routes, &v6Routes);

    updater.addInterfaceRoute(
        folly::IPAddress("1.1.1.1"),
        24,
        folly::IPAddress("1.1.1.1"),
        InterfaceID(1));
    updater.addInterfaceRoute(
        folly::IPAddress("1::1"), 48, folly::IPAddress("1::1"), InterfaceID(1));

    updater.addInterfaceRoute(
        folly::IPAddress("2.2.2.2"),
        24,
        folly::IPAddress("2.2.2.2"),
        InterfaceID(2));
    updater.addInterfaceRoute(
        folly::IPAddress("2::1"), 48, folly::IPAddress("2::1"), InterfaceID(2));

    updater.addLinkLocalRoutes();

    updater.updateDone();
  }

  auto staticClientId = ClientID::STATIC_ROUTE;
  auto staticAdminDistance = AdminDistance::STATIC_ROUTE;

  RouteUpdater updater(&v4Routes, &v6Routes);

  // Add v4/v6 static routes with a single next-hop each
  RouteNextHopSet nexthop;

  nexthop = {UnresolvedNextHop(folly::IPAddress("2::2"), UCMP_DEFAULT_WEIGHT)};
  updater.addRoute(
      IPAddress("2001::"),
      64,
      staticClientId,
      RouteNextHopEntry(nexthop, staticAdminDistance));

  nexthop = {
      UnresolvedNextHop(folly::IPAddress("2.2.2.3"), UCMP_DEFAULT_WEIGHT)};
  updater.addRoute(
      IPAddress("20.20.20.0"),
      24,
      staticClientId,
      RouteNextHopEntry(nexthop, staticAdminDistance));

  auto insertStaticNoNhopRoutes = [&](bool isToCpu, int prefixStartIdx) {
    RouteForwardAction action =
        isToCpu ? RouteForwardAction::TO_CPU : RouteForwardAction::DROP;

    updater.addRoute(
        IPAddress(folly::sformat("30.30.{}.0", prefixStartIdx)),
        24,
        staticClientId,
        RouteNextHopEntry(action, staticAdminDistance));

    updater.addRoute(
        IPAddress(folly::sformat("240{}::", prefixStartIdx)),
        64,
        staticClientId,
        RouteNextHopEntry(action, staticAdminDistance));
  };
  // Add v4/v6 static routes to CPU/NULL
  insertStaticNoNhopRoutes(true /* TO_CPU */, 1);
  insertStaticNoNhopRoutes(false /* DROP */, 2);

  updater.updateDone();

  // 5 = 2 (interface routes) + 1 (static routes with nhops) +
  // 1 static routes to CPU) + 1 (static routes to NULL)
  EXPECT_EQ(5, v4Routes.size());
  // 6 = 2 (interface routes) + 1 (static routes with nhops) +
  // 1 (static routes to CPU) + 1 (static routes to NULL) + 1 (link local route)
  EXPECT_EQ(6, v6Routes.size());
}

// Utility function for creating a nexthops list of size n,
// starting with the prefix.  For prefix "1.1.1.", first
// IP in the list will be 1.1.1.10
RouteNextHopSet newNextHops(int n, std::string prefix) {
  RouteNextHopSet h;
  for (int i = 0; i < n; i++) {
    auto ipStr = prefix + std::to_string(i + 10);
    h.emplace(UnresolvedNextHop(IPAddress(ipStr), UCMP_DEFAULT_WEIGHT));
  }
  return h;
}

// Test adding and deleting per-client nexthops lists
TEST(Route, modRoutes) {
  IPv4NetworkToRouteMap v4Routes;
  IPv6NetworkToRouteMap v6Routes;

  RouteUpdater u1(&v4Routes, &v6Routes);

  RouteV4::Prefix prefix10{IPAddressV4("10.10.10.10"), 32};
  RouteV4::Prefix prefix99{IPAddressV4("99.99.99.99"), 32};

  RouteNextHopSet nexthops1 = newNextHops(3, "1.1.1.");
  RouteNextHopSet nexthops2 = newNextHops(3, "2.2.2.");
  RouteNextHopSet nexthops3 = newNextHops(3, "3.3.3.");

  u1.addRoute(
      IPAddress("10.10.10.10"),
      32,
      kClientA,
      RouteNextHopEntry(nexthops1, kDistance));
  u1.addRoute(
      IPAddress("10.10.10.10"),
      32,
      kClientB,
      RouteNextHopEntry(nexthops2, kDistance));
  u1.addRoute(
      IPAddress("99.99.99.99"),
      32,
      kClientA,
      RouteNextHopEntry(nexthops3, kDistance));
  u1.updateDone();

  IPv4NetworkToRouteMap::Iterator it;
  RouteV4 *rt10, *rt99;

  it = v4Routes.exactMatch(prefix10.network, prefix10.mask);
  ASSERT_NE(v4Routes.end(), it);
  rt10 = &(it->value());

  it = v4Routes.exactMatch(prefix99.network, prefix99.mask);
  ASSERT_NE(v4Routes.end(), it);
  rt99 = &(it->value());

  // route 10 has two nexthop sets and route 99 has one
  EXPECT_TRUE(rt10->has(kClientA, RouteNextHopEntry(nexthops1, kDistance)));
  EXPECT_TRUE(rt10->has(kClientB, RouteNextHopEntry(nexthops2, kDistance)));
  EXPECT_TRUE(rt99->has(kClientA, RouteNextHopEntry(nexthops3, kDistance)));

  RouteUpdater u2(&v4Routes, &v6Routes);
  u2.delRoute(IPAddress("10.10.10.10"), 32, kClientA);
  u2.updateDone();

  it = v4Routes.exactMatch(prefix10.network, prefix10.mask);
  ASSERT_NE(v4Routes.end(), it);
  rt10 = &(it->value());

  it = v4Routes.exactMatch(prefix99.network, prefix99.mask);
  ASSERT_NE(v4Routes.end(), it);
  rt99 = &(it->value());

  // Only the 10.10.10.10 route for client kClientA should be missing
  EXPECT_FALSE(rt10->has(kClientA, RouteNextHopEntry(nexthops1, kDistance)));
  EXPECT_TRUE(rt10->has(kClientB, RouteNextHopEntry(nexthops2, kDistance)));
  EXPECT_TRUE(rt99->has(kClientA, RouteNextHopEntry(nexthops3, kDistance)));
  EXPECT_EQ(rt10->getEntryForClient(kClientA), nullptr);
  EXPECT_NE(rt10->getEntryForClient(kClientB), nullptr);

  // Delete the second client/nexthop pair.
  // The route & prefix should disappear altogether.
  RouteUpdater u3(&v4Routes, &v6Routes);
  u3.delRoute(IPAddress("10.10.10.10"), 32, kClientB);
  u3.updateDone();

  it = v4Routes.exactMatch(prefix10.network, prefix10.mask);
  EXPECT_EQ(v4Routes.end(), it);
}

// Test adding empty nextHops lists
TEST(Route, disallowEmptyNexthops) {
  IPv4NetworkToRouteMap v4Routes;
  IPv6NetworkToRouteMap v6Routes;

  RouteUpdater u1(&v4Routes, &v6Routes);

  // It's illegal to add an empty nextHops list to a route

  // Test the case where the empty list is the first to be added to the Route
  ASSERT_THROW(
      u1.addRoute(
          IPAddress("5.5.5.5"),
          32,
          kClientA,
          RouteNextHopEntry(newNextHops(0, "20.20.20."), kDistance)),
      FbossError);

  // Test the case where the empty list is the second to be added to the Route
  u1.addRoute(
      IPAddress("10.10.10.10"),
      32,
      kClientA,
      RouteNextHopEntry(newNextHops(3, "10.10.10."), kDistance));
  ASSERT_THROW(
      u1.addRoute(
          IPAddress("10.10.10.10"),
          32,
          kClientB,
          RouteNextHopEntry(newNextHops(0, "20.20.20."), kDistance)),
      FbossError);
}

TEST(Route, delRoutes) {
  IPv4NetworkToRouteMap v4Routes;
  IPv6NetworkToRouteMap v6Routes;

  RouteUpdater u1(&v4Routes, &v6Routes);

  RouteV4::Prefix prefix10{IPAddressV4("10.10.10.10"), 32};
  RouteV4::Prefix prefix22{IPAddressV4("22.22.22.22"), 32};

  u1.addRoute(
      IPAddress("10.10.10.10"),
      32,
      kClientA,
      RouteNextHopEntry(newNextHops(3, "1.1.1."), kDistance));
  u1.addRoute(
      IPAddress("22.22.22.22"),
      32,
      kClientB,
      RouteNextHopEntry(TO_CPU, kDistance));
  u1.updateDone();

  // Both routes should be present
  EXPECT_NE(
      v4Routes.end(), v4Routes.exactMatch(prefix10.network, prefix10.mask));
  EXPECT_NE(
      v4Routes.end(), v4Routes.exactMatch(prefix22.network, prefix22.mask));

  // delRoute() should work for the route with TO_CPU.
  RouteUpdater u2(&v4Routes, &v6Routes);
  u2.delRoute(IPAddress("22.22.22.22"), 32, kClientB);
  u2.updateDone();

  // Route for 10.10.10.10 should still be there,
  // but route for 22.22.22.22 should be gone
  EXPECT_NE(
      v4Routes.end(), v4Routes.exactMatch(prefix10.network, prefix10.mask));
  EXPECT_EQ(
      v4Routes.end(), v4Routes.exactMatch(prefix22.network, prefix22.mask));
}

// Test equality of RouteNextHopsMulti.
TEST(Route, equality) {
  // Create two identical RouteNextHopsMulti, and compare
  RouteNextHopsMulti nhm1;
  nhm1.update(kClientA, RouteNextHopEntry(newNextHops(3, "1.1.1."), kDistance));
  nhm1.update(kClientB, RouteNextHopEntry(newNextHops(3, "2.2.2."), kDistance));

  RouteNextHopsMulti nhm2;
  nhm2.update(kClientA, RouteNextHopEntry(newNextHops(3, "1.1.1."), kDistance));
  nhm2.update(kClientB, RouteNextHopEntry(newNextHops(3, "2.2.2."), kDistance));

  EXPECT_TRUE(nhm1 == nhm2);

  // Delete data for kClientC.  But there wasn't any.  Two objs still equal
  nhm1.delEntryForClient(kClientC);
  EXPECT_TRUE(nhm1 == nhm2);

  // Delete obj1's kClientB.  Now, objs should be NOT equal
  nhm1.delEntryForClient(kClientB);
  EXPECT_FALSE(nhm1 == nhm2);

  // Now replace obj1's kClientB list with a shorter list.
  // Objs should be NOT equal.
  nhm1.update(kClientB, RouteNextHopEntry(newNextHops(2, "2.2.2."), kDistance));
  EXPECT_FALSE(nhm1 == nhm2);

  // Now replace obj1's kClientB list with the original list.
  // But construct the list in opposite order.
  // Objects should still be equal, despite the order of construction.
  RouteNextHopSet nextHopsRev;
  nextHopsRev.emplace(
      UnresolvedNextHop(IPAddress("2.2.2.12"), UCMP_DEFAULT_WEIGHT));
  nextHopsRev.emplace(
      UnresolvedNextHop(IPAddress("2.2.2.11"), UCMP_DEFAULT_WEIGHT));
  nextHopsRev.emplace(
      UnresolvedNextHop(IPAddress("2.2.2.10"), UCMP_DEFAULT_WEIGHT));
  nhm1.update(kClientB, RouteNextHopEntry(nextHopsRev, kDistance));
  EXPECT_TRUE(nhm1 == nhm2);
}

// Test that a copy of a RouteNextHopsMulti is a deep copy, and that the
// resulting objects can be modified independently.
TEST(Route, deepCopy) {
  // Create two identical RouteNextHopsMulti, and compare
  RouteNextHopsMulti nhm1;
  auto origHops = newNextHops(3, "1.1.1.");
  nhm1.update(kClientA, RouteNextHopEntry(origHops, kDistance));
  nhm1.update(kClientB, RouteNextHopEntry(newNextHops(3, "2.2.2."), kDistance));

  // Copy it
  RouteNextHopsMulti nhm2 = nhm1;

  // The two should be identical
  EXPECT_TRUE(nhm1 == nhm2);

  // Now modify the underlying nexthop list.
  // Should be changed in nhm1, but not nhm2.
  auto newHops = newNextHops(4, "10.10.10.");
  nhm1.update(kClientA, RouteNextHopEntry(newHops, kDistance));

  EXPECT_FALSE(nhm1 == nhm2);

  EXPECT_TRUE(nhm1.isSame(kClientA, RouteNextHopEntry(newHops, kDistance)));
  EXPECT_TRUE(nhm2.isSame(kClientA, RouteNextHopEntry(origHops, kDistance)));
}

// Test serialization of RouteNextHopsMulti.
TEST(Route, toThriftRouteNextHopsMulti) {
  // This function tests toThrift of:
  // RouteNextHopsMulti

  // test for new format to new format
  RouteNextHopsMulti nhm1;
  nhm1.update(kClientA, RouteNextHopEntry(newNextHops(3, "1.1.1."), kDistance));
  nhm1.update(kClientB, RouteNextHopEntry(newNextHops(1, "2.2.2."), kDistance));
  nhm1.update(kClientC, RouteNextHopEntry(newNextHops(4, "3.3.3."), kDistance));

  auto clientAndNexthops = nhm1.toThrift();
  EXPECT_EQ(3, clientAndNexthops.size());
  for (auto const& entry : clientAndNexthops) {
    EXPECT_GE(entry.nextHops.size(), 1);
    ASSERT_EQ(entry.nextHops.size(), entry.nextHopAddrs.size());
    for (auto i = 0; i < entry.nextHops.size(); ++i) {
      EXPECT_EQ(entry.nextHopAddrs.at(i), *entry.nextHops.at(i).address_ref());
    }
  }
}

// Test serialization of RouteNextHopsMulti.
TEST(Route, serializeRouteNextHopsMulti) {
  // This function tests [de]serialization of:
  // RouteNextHopsMulti
  // RouteNextHopEntry
  // NextHop

  // test for new format to new format
  RouteNextHopsMulti nhm1;
  nhm1.update(kClientA, RouteNextHopEntry(newNextHops(3, "1.1.1."), kDistance));
  nhm1.update(kClientB, RouteNextHopEntry(newNextHops(1, "2.2.2."), kDistance));
  nhm1.update(kClientC, RouteNextHopEntry(newNextHops(4, "3.3.3."), kDistance));
  nhm1.update(kClientD, RouteNextHopEntry(RouteForwardAction::DROP, kDistance));
  nhm1.update(
      kClientE, RouteNextHopEntry(RouteForwardAction::TO_CPU, kDistance));

  auto serialized = nhm1.toFollyDynamic();

  auto nhm2 = RouteNextHopsMulti::fromFollyDynamic(serialized);

  EXPECT_TRUE(nhm1 == nhm2);
}

// Test priority ranking of nexthop lists within a RouteNextHopsMulti.
TEST(Route, listRanking) {
  auto list00 = newNextHops(3, "0.0.0.");
  auto list07 = newNextHops(3, "7.7.7.");
  auto list01 = newNextHops(3, "1.1.1.");
  auto list20 = newNextHops(3, "20.20.20.");
  auto list30 = newNextHops(3, "30.30.30.");

  RouteNextHopsMulti nhm;
  nhm.update(ClientID(20), RouteNextHopEntry(list20, AdminDistance::EBGP));
  nhm.update(
      ClientID(1), RouteNextHopEntry(list01, AdminDistance::STATIC_ROUTE));
  nhm.update(
      ClientID(30),
      RouteNextHopEntry(list30, AdminDistance::MAX_ADMIN_DISTANCE));
  EXPECT_TRUE(nhm.getBestEntry().second->getNextHopSet() == list01);

  nhm.update(
      ClientID(10),
      RouteNextHopEntry(list00, AdminDistance::DIRECTLY_CONNECTED));
  EXPECT_TRUE(nhm.getBestEntry().second->getNextHopSet() == list00);

  nhm.delEntryForClient(ClientID(10));
  EXPECT_TRUE(nhm.getBestEntry().second->getNextHopSet() == list01);

  nhm.delEntryForClient(ClientID(30));
  EXPECT_TRUE(nhm.getBestEntry().second->getNextHopSet() == list01);

  nhm.delEntryForClient(ClientID(1));
  EXPECT_TRUE(nhm.getBestEntry().second->getNextHopSet() == list20);

  nhm.delEntryForClient(ClientID(20));
  EXPECT_THROW(nhm.getBestEntry().second->getNextHopSet(), FbossError);
}

bool stringStartsWith(std::string s1, std::string prefix) {
  return s1.compare(0, prefix.size(), prefix) == 0;
}

void assertClientsNotPresent(
    IPv4NetworkToRouteMap& routes,
    RouteV4::Prefix prefix,
    std::vector<int16_t> clientIds) {
  IPv4NetworkToRouteMap::Iterator it;

  for (int16_t clientId : clientIds) {
    it = routes.exactMatch(prefix.network, prefix.mask);
    CHECK(it != routes.end());
    auto route = it->value();

    auto entry = route.getEntryForClient(ClientID(clientId));
    EXPECT_EQ(nullptr, entry);
  }
}

void assertClientsPresent(
    IPv4NetworkToRouteMap& routes,
    RouteV4::Prefix prefix,
    std::vector<int16_t> clientIds) {
  IPv4NetworkToRouteMap::Iterator it;

  for (int16_t clientId : clientIds) {
    it = routes.exactMatch(prefix.network, prefix.mask);
    CHECK(it != routes.end());
    auto route = it->value();

    auto entry = route.getEntryForClient(ClientID(clientId));
    EXPECT_NE(nullptr, entry);
  }
}

void expectFwdInfo(
    IPv4NetworkToRouteMap& routes,
    RouteV4::Prefix prefix,
    std::string ipPrefix) {
  auto it = routes.exactMatch(prefix.network, prefix.mask);
  CHECK(routes.end() != it);

  const auto& fwd = it->value().getForwardInfo();
  const auto& nhops = fwd.getNextHopSet();
  // Expect the fwd'ing info to be 3 IPs all starting with 'ipPrefix'
  EXPECT_EQ(3, nhops.size());
  for (auto const& it : nhops) {
    EXPECT_TRUE(stringStartsWith(it.addr().str(), ipPrefix));
  }
}

void addNextHopsForClient(
    IPv4NetworkToRouteMap& routes,
    IPv6NetworkToRouteMap& v6Routes,
    RouteV4::Prefix prefix,
    int16_t clientId,
    std::string ipPrefix,
    AdminDistance adminDistance = AdminDistance::MAX_ADMIN_DISTANCE) {
  RouteUpdater u(&routes, &v6Routes);
  u.addRoute(
      prefix.network,
      prefix.mask,
      ClientID(clientId),
      RouteNextHopEntry(newNextHops(3, ipPrefix), adminDistance));
  u.updateDone();
}

void deleteNextHopsForClient(
    IPv4NetworkToRouteMap& routes,
    IPv6NetworkToRouteMap& v6Routes,
    RouteV4::Prefix prefix,
    int16_t clientId) {
  RouteUpdater u(&routes, &v6Routes);
  u.delRoute(prefix.network, prefix.mask, ClientID(clientId));
  u.updateDone();
}

// Add and remove per-client NextHop lists to the same route, and make sure
// the lowest admin distance is the one that determines the forwarding info
TEST(Route, fwdInfoRanking) {
  IPv4NetworkToRouteMap v4Routes;
  IPv6NetworkToRouteMap v6Routes;

  // We'll be adding and removing a bunch of nexthops for this Network & Mask
  auto network = IPAddressV4("22.22.22.22");
  uint8_t mask = 32;
  RouteV4::Prefix prefix{network, mask};

  // Add client 30, plus an interface for resolution.
  RouteUpdater u1(&v4Routes, &v6Routes);
  // This is the route all the others will resolve to.
  u1.addRoute(
      IPAddress("10.10.0.0"),
      16,
      ClientID::INTERFACE_ROUTE,
      RouteNextHopEntry(
          ResolvedNextHop(
              IPAddress("10.10.0.1"), InterfaceID(9), UCMP_DEFAULT_WEIGHT),
          AdminDistance::DIRECTLY_CONNECTED));
  u1.addRoute(
      network,
      mask,
      ClientID(30),
      RouteNextHopEntry(newNextHops(3, "10.10.30."), kDistance));
  u1.updateDone();

  // Expect fwdInfo based on client 30
  assertClientsPresent(v4Routes, prefix, {30});
  assertClientsNotPresent(v4Routes, prefix, {10, 20, 40, 50, 999});
  expectFwdInfo(v4Routes, prefix, "10.10.30.");

  // Add client 20
  addNextHopsForClient(
      v4Routes, v6Routes, prefix, 20, "10.10.20.", AdminDistance::EBGP);

  // Expect fwdInfo based on client 20
  assertClientsPresent(v4Routes, prefix, {20, 30});
  assertClientsNotPresent(v4Routes, prefix, {10, 40, 50, 999});
  expectFwdInfo(v4Routes, prefix, "10.10.20.");

  // Add client 40
  addNextHopsForClient(v4Routes, v6Routes, prefix, 40, "10.10.40.");

  // Expect fwdInfo still based on client 20
  assertClientsPresent(v4Routes, prefix, {20, 30, 40});
  assertClientsNotPresent(v4Routes, prefix, {10, 50, 999});
  expectFwdInfo(v4Routes, prefix, "10.10.20.");

  // Add client 10
  addNextHopsForClient(
      v4Routes, v6Routes, prefix, 10, "10.10.10.", AdminDistance::STATIC_ROUTE);

  // Expect fwdInfo based on client 10
  assertClientsPresent(v4Routes, prefix, {10, 20, 30, 40});
  assertClientsNotPresent(v4Routes, prefix, {50, 999});
  expectFwdInfo(v4Routes, prefix, "10.10.10.");

  // Remove client 20
  deleteNextHopsForClient(v4Routes, v6Routes, prefix, 20);

  // Winner should still be 10
  assertClientsPresent(v4Routes, prefix, {10, 30, 40});
  assertClientsNotPresent(v4Routes, prefix, {20, 50, 999});
  expectFwdInfo(v4Routes, prefix, "10.10.10.");

  // Remove client 10
  deleteNextHopsForClient(v4Routes, v6Routes, prefix, 10);

  // Winner should now be 30
  assertClientsPresent(v4Routes, prefix, {30, 40});
  assertClientsNotPresent(v4Routes, prefix, {10, 20, 50, 999});
  expectFwdInfo(v4Routes, prefix, "10.10.30.");

  // Remove client 30
  deleteNextHopsForClient(v4Routes, v6Routes, prefix, 30);

  // Winner should now be 40
  assertClientsPresent(v4Routes, prefix, {40});
  assertClientsNotPresent(v4Routes, prefix, {10, 20, 30, 50, 999});
  expectFwdInfo(v4Routes, prefix, "10.10.40.");
}

TEST(Route, dropRoutes) {
  IPv4NetworkToRouteMap v4Routes;
  IPv6NetworkToRouteMap v6Routes;

  RouteUpdater u1(&v4Routes, &v6Routes);
  u1.addRoute(
      IPAddress("10.10.10.10"),
      32,
      kClientA,
      RouteNextHopEntry(DROP, kDistance));
  u1.addRoute(
      IPAddress("2001::0"), 128, kClientA, RouteNextHopEntry(DROP, kDistance));
  // Check recursive resolution for drop routes
  RouteNextHopSet v4nexthops = makeNextHops({"10.10.10.10"});
  u1.addRoute(
      IPAddress("20.20.20.0"),
      24,
      kClientA,
      RouteNextHopEntry(v4nexthops, kDistance));
  RouteNextHopSet v6nexthops = makeNextHops({"2001::0"});
  u1.addRoute(
      IPAddress("2001:1::"),
      64,
      kClientA,
      RouteNextHopEntry(v6nexthops, kDistance));

  u1.updateDone();

  // Check routes
  auto r1 = getRoute(v4Routes, "10.10.10.10/32");
  EXPECT_RESOLVED(r1);
  EXPECT_FALSE(r1->isConnected());
  EXPECT_TRUE(r1->has(kClientA, RouteNextHopEntry(DROP, kDistance)));
  EXPECT_EQ(r1->getForwardInfo(), RouteNextHopEntry(DROP, kDistance));

  auto r2 = getRoute(v4Routes, "20.20.20.0/24");
  EXPECT_RESOLVED(r2);
  EXPECT_FALSE(r2->isConnected());
  EXPECT_FALSE(r2->has(kClientA, RouteNextHopEntry(DROP, kDistance)));
  EXPECT_EQ(r2->getForwardInfo(), RouteNextHopEntry(DROP, kDistance));

  auto r3 = getRoute(v6Routes, "2001::0/128");
  EXPECT_RESOLVED(r3);
  EXPECT_FALSE(r3->isConnected());
  EXPECT_TRUE(r3->has(kClientA, RouteNextHopEntry(DROP, kDistance)));
  EXPECT_EQ(r3->getForwardInfo(), RouteNextHopEntry(DROP, kDistance));

  auto r4 = getRoute(v6Routes, "2001:1::/64");
  EXPECT_RESOLVED(r4);
  EXPECT_FALSE(r4->isConnected());
  EXPECT_EQ(r4->getForwardInfo(), RouteNextHopEntry(DROP, kDistance));
}

TEST(Route, toCPURoutes) {
  IPv4NetworkToRouteMap v4Routes;
  IPv6NetworkToRouteMap v6Routes;

  RouteUpdater u1(&v4Routes, &v6Routes);
  u1.addRoute(
      IPAddress("10.10.10.10"),
      32,
      kClientA,
      RouteNextHopEntry(TO_CPU, kDistance));
  u1.addRoute(
      IPAddress("2001::0"),
      128,
      kClientA,
      RouteNextHopEntry(TO_CPU, kDistance));
  // Check recursive resolution for to_cpu routes
  RouteNextHopSet v4nexthops = makeNextHops({"10.10.10.10"});
  u1.addRoute(
      IPAddress("20.20.20.0"),
      24,
      kClientA,
      RouteNextHopEntry(v4nexthops, kDistance));
  RouteNextHopSet v6nexthops = makeNextHops({"2001::0"});
  u1.addRoute(
      IPAddress("2001:1::"),
      64,
      kClientA,
      RouteNextHopEntry(v6nexthops, kDistance));

  u1.updateDone();

  // Check routes
  auto r1 = getRoute(v4Routes, "10.10.10.10/32");
  EXPECT_RESOLVED(r1);
  EXPECT_FALSE(r1->isConnected());
  EXPECT_TRUE(r1->has(kClientA, RouteNextHopEntry(TO_CPU, kDistance)));
  EXPECT_EQ(r1->getForwardInfo(), RouteNextHopEntry(TO_CPU, kDistance));

  auto r2 = getRoute(v4Routes, "20.20.20.0/24");
  EXPECT_RESOLVED(r2);
  EXPECT_FALSE(r2->isConnected());
  EXPECT_EQ(r2->getForwardInfo(), RouteNextHopEntry(TO_CPU, kDistance));

  auto r3 = getRoute(v6Routes, "2001::0/128");
  EXPECT_RESOLVED(r3);
  EXPECT_FALSE(r3->isConnected());
  EXPECT_TRUE(r3->has(kClientA, RouteNextHopEntry(TO_CPU, kDistance)));
  EXPECT_EQ(r3->getForwardInfo(), RouteNextHopEntry(TO_CPU, kDistance));

  auto r5 = getRoute(v6Routes, "2001:1::/64");
  EXPECT_RESOLVED(r5);
  EXPECT_FALSE(r5->isConnected());
  EXPECT_EQ(r5->getForwardInfo(), RouteNextHopEntry(TO_CPU, kDistance));
}

// Very basic test for serialization/deseralization of Routes
TEST(Route, serializeRoute) {
  ClientID clientId = ClientID(1);
  auto nxtHops = makeNextHops({"10.10.10.10", "11.11.11.11"});
  Route<IPAddressV4> rt(PrefixV4::fromString("1.2.3.4/32"));
  rt.update(clientId, RouteNextHopEntry(nxtHops, kDistance));

  // to folly dynamic
  folly::dynamic obj = rt.toFollyDynamic();
  // to string
  folly::json::serialization_opts serOpts;
  serOpts.allow_non_string_keys = true;
  std::string json = folly::json::serialize(obj, serOpts);
  // back to folly dynamic
  folly::dynamic obj2 = folly::parseJson(json, serOpts);
  // back to Route object
  auto rt2 = Route<IPAddressV4>::fromFollyDynamic(obj2);
  ASSERT_TRUE(rt2.has(clientId, RouteNextHopEntry(nxtHops, kDistance)));
}

TEST(Route, serializeRouteTable) {
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

  RouteUpdater u2(&v4Routes, &v6Routes);
  u2.addRoute(
      r1.network, r1.mask, kClientA, RouteNextHopEntry(nhop1, kDistance));
  u2.addRoute(
      r2.network, r2.mask, kClientA, RouteNextHopEntry(nhop2, kDistance));
  u2.addRoute(
      r3.network, r3.mask, kClientA, RouteNextHopEntry(nhop1, kDistance));
  u2.addRoute(
      r4.network, r4.mask, kClientA, RouteNextHopEntry(nhop2, kDistance));
  u2.updateDone();

  folly::json::serialization_opts serOpts;
  serOpts.allow_non_string_keys = true;

  auto origV4Routes = &v4Routes;
  folly::dynamic serializedAsObjectV4 = origV4Routes->toFollyDynamic();
  std::string serializedAsJsonV4 =
      folly::json::serialize(serializedAsObjectV4, serOpts);
  folly::dynamic deserializedAsObjectV4 =
      folly::parseJson(serializedAsJsonV4, serOpts);
  auto newV4Routes = NetworkToRouteMap<folly::IPAddressV4>::fromFollyDynamic(
      deserializedAsObjectV4);
  EXPECT_ROUTES_MATCH(origV4Routes, &newV4Routes);

  auto origV6Routes = &v6Routes;
  auto serializedAsObjectV6 = origV6Routes->toFollyDynamic();
  auto serializedAsJsonV6 =
      folly::json::serialize(serializedAsObjectV6, serOpts);
  auto deserializedAsObjectV6 = folly::parseJson(serializedAsJsonV6, serOpts);
  auto newV6Routes = NetworkToRouteMap<folly::IPAddressV6>::fromFollyDynamic(
      deserializedAsObjectV6);
  EXPECT_ROUTES_MATCH(origV6Routes, &newV6Routes);
}

// Test utility functions for converting RouteNextHopSet to thrift and back
TEST(RouteTypes, toFromRouteNextHops) {
  RouteNextHopSet nhs;
  // Non v4 link-local address without interface scoping
  nhs.emplace(
      UnresolvedNextHop(folly::IPAddress("10.0.0.1"), UCMP_DEFAULT_WEIGHT));

  // v4 link-local address without interface scoping
  nhs.emplace(
      UnresolvedNextHop(folly::IPAddress("169.254.0.1"), UCMP_DEFAULT_WEIGHT));

  // Non v6 link-local address without interface scoping
  nhs.emplace(
      UnresolvedNextHop(folly::IPAddress("face:b00c::1"), UCMP_DEFAULT_WEIGHT));

  // v6 link-local address with interface scoping
  nhs.emplace(ResolvedNextHop(
      folly::IPAddress("fe80::1"), InterfaceID(4), UCMP_DEFAULT_WEIGHT));

  // v6 link-local address without interface scoping
  EXPECT_THROW(
      nhs.emplace(UnresolvedNextHop(folly::IPAddress("fe80::1"), 42)),
      FbossError);

  // Convert to thrift object
  auto nhts = rib::util::fromRouteNextHopSet(nhs);
  ASSERT_EQ(4, nhts.size());

  auto verify = [&](const std::string& ipaddr,
                    std::optional<InterfaceID> intf) {
    auto bAddr = facebook::network::toBinaryAddress(folly::IPAddress(ipaddr));
    if (intf.has_value()) {
      bAddr.ifName_ref() =
          facebook::fboss::util::createTunIntfName(intf.value());
    }
    bool found = false;
    for (const auto& entry : nhts) {
      if (*entry.address_ref() == bAddr) {
        if (intf.has_value()) {
          EXPECT_TRUE(entry.address_ref()->ifName_ref());
          EXPECT_EQ(
              bAddr.ifName_ref().value_or({}),
              entry.address_ref()->ifName_ref().value_or({}));
        }
        found = true;
        break;
      }
    }
    XLOG(INFO) << "**** " << ipaddr;
    EXPECT_TRUE(found);
  };

  verify("10.0.0.1", std::nullopt);
  verify("169.254.0.1", std::nullopt);
  verify("face:b00c::1", std::nullopt);
  verify("fe80::1", InterfaceID(4));

  // Convert back to RouteNextHopSet
  auto newNhs = rib::util::toRouteNextHopSet(nhts);
  EXPECT_EQ(nhs, newNhs);

  //
  // Some ignore cases
  //

  facebook::network::thrift::BinaryAddress addr;

  addr = facebook::network::toBinaryAddress(folly::IPAddress("10.0.0.1"));
  addr.ifName_ref() = "fboss10";
  NextHopThrift nht;
  *nht.address_ref() = addr;
  {
    NextHop nh = rib::util::fromThrift(nht);
    EXPECT_EQ(folly::IPAddress("10.0.0.1"), nh.addr());
    EXPECT_EQ(std::nullopt, nh.intfID());
  }

  addr = facebook::network::toBinaryAddress(folly::IPAddress("face::1"));
  addr.ifName_ref() = "fboss10";
  *nht.address_ref() = addr;
  {
    NextHop nh = rib::util::fromThrift(nht);
    EXPECT_EQ(folly::IPAddress("face::1"), nh.addr());
    EXPECT_EQ(std::nullopt, nh.intfID());
  }

  addr = facebook::network::toBinaryAddress(folly::IPAddress("fe80::1"));
  addr.ifName_ref() = "fboss10";
  *nht.address_ref() = addr;
  {
    NextHop nh = rib::util::fromThrift(nht);
    EXPECT_EQ(folly::IPAddress("fe80::1"), nh.addr());
    EXPECT_EQ(InterfaceID(10), nh.intfID());
  }
}

TEST(Route, nextHopTest) {
  folly::IPAddress addr("1.1.1.1");
  NextHop unh = UnresolvedNextHop(addr, 0);
  NextHop rnh = ResolvedNextHop(addr, InterfaceID(1), 0);
  EXPECT_FALSE(unh.isResolved());
  EXPECT_TRUE(rnh.isResolved());
  EXPECT_EQ(unh.addr(), addr);
  EXPECT_EQ(rnh.addr(), addr);
  EXPECT_THROW(unh.intf(), std::bad_optional_access);
  EXPECT_EQ(rnh.intf(), InterfaceID(1));
  EXPECT_NE(unh, rnh);
  auto unh2 = unh;
  EXPECT_EQ(unh, unh2);
  auto rnh2 = rnh;
  EXPECT_EQ(rnh, rnh2);
  EXPECT_FALSE(rnh < unh && unh < rnh);
  EXPECT_TRUE(rnh < unh || unh < rnh);
  EXPECT_TRUE(unh < rnh && rnh > unh);
}

TEST(Route, withLabelForwardingAction) {
  std::array<folly::IPAddressV4, 4> nextHopAddrs{
      folly::IPAddressV4("1.1.1.0"),
      folly::IPAddressV4("1.1.2.0"),
      folly::IPAddressV4("1.1.3.0"),
      folly::IPAddressV4("1.1.4.0"),
  };

  std::array<LabelForwardingAction::LabelStack, 4> nextHopStacks{
      LabelForwardingAction::LabelStack({101, 201, 301}),
      LabelForwardingAction::LabelStack({102, 202, 302}),
      LabelForwardingAction::LabelStack({103, 203, 303}),
      LabelForwardingAction::LabelStack({104, 204, 304}),
  };

  std::map<folly::IPAddressV4, LabelForwardingAction::LabelStack>
      labeledNextHops;
  RouteNextHopSet nexthops;

  for (auto i = 0; i < 4; i++) {
    labeledNextHops.emplace(std::make_pair(nextHopAddrs[i], nextHopStacks[i]));
    nexthops.emplace(UnresolvedNextHop(
        nextHopAddrs[i],
        1,
        std::make_optional<LabelForwardingAction>(
            LabelForwardingAction::LabelForwardingType::PUSH,
            nextHopStacks[i])));
  }

  IPv4NetworkToRouteMap v4Routes;
  IPv6NetworkToRouteMap v6Routes;

  RouteUpdater updater(&v4Routes, &v6Routes);
  updater.addRoute(
      folly::IPAddressV4("1.1.0.0"),
      16,
      kClientA,
      RouteNextHopEntry(nexthops, kDistance));

  updater.updateDone();

  EXPECT_EQ(1, v4Routes.size());
  EXPECT_EQ(0, v6Routes.size());

  const auto* route = longestMatch(v4Routes, folly::IPAddressV4("1.1.2.2"));

  EXPECT_EQ(true, route->has(kClientA, RouteNextHopEntry(nexthops, kDistance)));

  auto entry = route->getBestEntry();
  for (const auto& nh : entry.second->getNextHopSet()) {
    EXPECT_EQ(nh.labelForwardingAction().has_value(), true);
    EXPECT_EQ(
        nh.labelForwardingAction()->type(),
        LabelForwardingAction::LabelForwardingType::PUSH);
    EXPECT_EQ(nh.labelForwardingAction()->pushStack().has_value(), true);
    EXPECT_EQ(nh.labelForwardingAction()->pushStack()->size(), 3);
    EXPECT_EQ(
        labeledNextHops[nh.addr().asV4()],
        nh.labelForwardingAction()->pushStack().value());
  }
}

TEST(Route, withNoLabelForwardingAction) {
  auto routeNextHopEntry = RouteNextHopEntry(
      makeNextHops({"1.1.1.1", "1.1.2.1", "1.1.3.1", "1.1.4.1"}), kDistance);

  IPv4NetworkToRouteMap v4Routes;
  IPv6NetworkToRouteMap v6Routes;

  RouteUpdater updater(&v4Routes, &v6Routes);

  updater.addRoute(
      folly::IPAddressV4("1.1.0.0"), 16, kClientA, routeNextHopEntry);

  updater.updateDone();

  EXPECT_EQ(1, v4Routes.size());
  EXPECT_EQ(0, v6Routes.size());

  const auto* route = longestMatch(v4Routes, folly::IPAddressV4("1.1.2.2"));

  EXPECT_EQ(route->has(kClientA, routeNextHopEntry), true);
  auto entry = route->getBestEntry();
  for (const auto& nh : entry.second->getNextHopSet()) {
    EXPECT_EQ(nh.labelForwardingAction().has_value(), false);
  }
  EXPECT_EQ(*entry.second, routeNextHopEntry);
}

TEST(Route, withInvalidLabelForwardingAction) {
  std::array<folly::IPAddressV4, 5> nextHopAddrs{
      folly::IPAddressV4("1.1.1.0"),
      folly::IPAddressV4("1.1.2.0"),
      folly::IPAddressV4("1.1.3.0"),
      folly::IPAddressV4("1.1.4.0"),
      folly::IPAddressV4("1.1.5.0"),
  };

  std::array<LabelForwardingAction, 5> nextHopLabelActions{
      LabelForwardingAction(
          LabelForwardingAction::LabelForwardingType::PUSH,
          LabelForwardingAction::LabelStack({101, 201, 301})),
      LabelForwardingAction(
          LabelForwardingAction::LabelForwardingType::SWAP,
          LabelForwardingAction::Label{202}),
      LabelForwardingAction(LabelForwardingAction::LabelForwardingType::NOOP),
      LabelForwardingAction(
          LabelForwardingAction::LabelForwardingType::POP_AND_LOOKUP),
      LabelForwardingAction(LabelForwardingAction::LabelForwardingType::PHP),
  };

  std::map<folly::IPAddressV4, LabelForwardingAction::LabelStack>
      labeledNextHops;
  RouteNextHopSet nexthops;

  for (auto i = 0; i < 5; i++) {
    nexthops.emplace(
        UnresolvedNextHop(nextHopAddrs[i], 1, nextHopLabelActions[i]));
  }

  IPv4NetworkToRouteMap v4Routes;
  IPv6NetworkToRouteMap v6Routes;

  RouteUpdater updater(&v4Routes, &v6Routes);

  EXPECT_THROW(
      {
        updater.addRoute(
            folly::IPAddressV4("1.1.0.0"),
            16,
            kClientA,
            RouteNextHopEntry(nexthops, kDistance));
      },
      FbossError);
}

/*
 * Class that makes it easy to run tests with the following
 * configurable entities:
 * Four interfaces: I1, I2, I3, I4
 * Three routes which require resolution: R1, R2, R3
 */
class UcmpTest : public ::testing::Test {
 public:
  void SetUp() override {
    configRoutes(&v4Routes_, &v6Routes_);
  }
  void resolveRoutes(std::vector<std::pair<folly::IPAddress, RouteNextHopSet>>
                         networkAndNextHops) {
    RouteUpdater ru(&v4Routes_, &v6Routes_);
    for (const auto& nnhs : networkAndNextHops) {
      folly::IPAddress net = nnhs.first;
      RouteNextHopSet nhs = nnhs.second;
      ru.addRoute(net, mask, kClientA, RouteNextHopEntry(nhs, kDistance));
    }
    ru.updateDone();

    for (const auto& nnhs : networkAndNextHops) {
      folly::IPAddress net = nnhs.first;
      auto pfx = folly::sformat("{}/{}", net.str(), mask);
      auto r = getRoute(v4Routes_, pfx);
      EXPECT_RESOLVED(r);
      EXPECT_FALSE(r->isConnected());
      resolvedRoutes.push_back(*r);
    }
  }

  void runRecursiveTest(
      const std::vector<RouteNextHopSet>& routeUnresolvedNextHops,
      const std::vector<NextHopWeight>& resolvedWeights) {
    std::vector<std::pair<folly::IPAddress, RouteNextHopSet>>
        networkAndNextHops;
    auto netsIter = nets.begin();
    for (const auto& nhs : routeUnresolvedNextHops) {
      networkAndNextHops.push_back({*netsIter, nhs});
      netsIter++;
    }
    resolveRoutes(networkAndNextHops);

    RouteNextHopSet expFwd1;
    uint8_t i = 0;
    for (const auto& w : resolvedWeights) {
      expFwd1.emplace(ResolvedNextHop(intfIps[i], InterfaceID(i + 1), w));
      ++i;
    }
    EXPECT_EQ(expFwd1, resolvedRoutes[0].getForwardInfo().getNextHopSet());
  }

  void runTwoDeepRecursiveTest(
      const std::vector<std::vector<NextHopWeight>>& unresolvedWeights,
      const std::vector<NextHopWeight>& resolvedWeights) {
    std::vector<RouteNextHopSet> routeUnresolvedNextHops;
    auto rsIter = rnhs.begin();
    for (const auto& ws : unresolvedWeights) {
      auto nhIter = rsIter->begin();
      RouteNextHopSet nexthops;
      for (const auto& w : ws) {
        nexthops.insert(UnresolvedNextHop(*nhIter, w));
        nhIter++;
      }
      routeUnresolvedNextHops.push_back(nexthops);
      rsIter++;
    }
    runRecursiveTest(routeUnresolvedNextHops, resolvedWeights);
  }

  void runVaryFromHundredTest(
      NextHopWeight w,
      const std::vector<NextHopWeight>& resolvedWeights) {
    runRecursiveTest(
        {{UnresolvedNextHop(intfIp1, 100),
          UnresolvedNextHop(intfIp2, 100),
          UnresolvedNextHop(intfIp3, 100),
          UnresolvedNextHop(intfIp4, w)}},
        resolvedWeights);
  }

  std::vector<Route<folly::IPAddressV4>> resolvedRoutes;
  const folly::IPAddress intfIp1{"1.1.1.10"};
  const folly::IPAddress intfIp2{"2.2.2.20"};
  const folly::IPAddress intfIp3{"3.3.3.30"};
  const folly::IPAddress intfIp4{"4.4.4.40"};
  const std::array<folly::IPAddress, 4> intfIps{
      {intfIp1, intfIp2, intfIp3, intfIp4}};
  const folly::IPAddress r2Nh{"42.42.42.42"};
  const folly::IPAddress r3Nh{"43.43.43.43"};
  std::array<folly::IPAddress, 2> r1Nhs{{r2Nh, r3Nh}};
  std::array<folly::IPAddress, 2> r2Nhs{{intfIp1, intfIp2}};
  std::array<folly::IPAddress, 2> r3Nhs{{intfIp3, intfIp4}};
  const std::array<std::array<folly::IPAddress, 2>, 3> rnhs{
      {r1Nhs, r2Nhs, r3Nhs}};
  const folly::IPAddress r1Net{"41.41.41.0"};
  const folly::IPAddress r2Net{"42.42.42.0"};
  const folly::IPAddress r3Net{"43.43.43.0"};
  const std::array<folly::IPAddress, 3> nets{{r1Net, r2Net, r3Net}};
  const uint8_t mask{24};

 private:
  IPv4NetworkToRouteMap v4Routes_;
  IPv6NetworkToRouteMap v6Routes_;
};

/*
 * Four interfaces: I1, I2, I3, I4
 * Three routes which require resolution: R1, R2, R3
 * R1 has R2 and R3 as next hops with weights 3 and 2
 * R2 has I1 and I2 as next hops with weights 5 and 4
 * R3 has I3 and I4 as next hops with weights 3 and 2
 * expect R1 to resolve to I1:25, I2:20, I3:18, I4:12
 */
TEST_F(UcmpTest, recursiveUcmp) {
  runTwoDeepRecursiveTest({{3, 2}, {5, 4}, {3, 2}}, {25, 20, 18, 12});
}

/*
 * Two interfaces: I1, I2
 * Three routes which require resolution: R1, R2, R3
 * R1 has R2 and R3 as next hops with weights 2 and 1
 * R2 has I1 and I2 as next hops with weights 1 and 1
 * R3 has I1 as next hop with weight 1
 * expect R1 to resolve to I1:2, I2:1
 */
TEST_F(UcmpTest, recursiveUcmpDuplicateIntf) {
  runRecursiveTest(
      {{UnresolvedNextHop(r2Nh, 2), UnresolvedNextHop(r3Nh, 1)},
       {UnresolvedNextHop(intfIp1, 1), UnresolvedNextHop(intfIp2, 1)},
       {UnresolvedNextHop(intfIp1, 1)}},
      {2, 1});
}

/*
 * Two interfaces: I1, I2
 * Three routes which require resolution: R1, R2, R3
 * R1 has R2 and R3 as next hops with ECMP
 * R2 has I1 and I2 as next hops with ECMP
 * R3 has I1 as next hop with weight 1
 * expect R1 to resolve to ECMP
 */
TEST_F(UcmpTest, recursiveEcmpDuplicateIntf) {
  runRecursiveTest(
      {{UnresolvedNextHop(r2Nh, ECMP_WEIGHT),
        UnresolvedNextHop(r3Nh, ECMP_WEIGHT)},
       {UnresolvedNextHop(intfIp1, ECMP_WEIGHT),
        UnresolvedNextHop(intfIp2, ECMP_WEIGHT)},
       {UnresolvedNextHop(intfIp1, 1)}},
      {ECMP_WEIGHT, ECMP_WEIGHT});
}

/*
 * Two interfaces: I1, I2
 * One route which requires resolution: R1
 * R1 has I1 and I2 as next hops with weights 0 (ECMP) and 1
 * expect R1 to resolve to ECMP between I1, I2
 */
TEST_F(UcmpTest, mixedUcmpVsEcmp_EcmpWins) {
  runRecursiveTest(
      {{UnresolvedNextHop(intfIp1, ECMP_WEIGHT),
        UnresolvedNextHop(intfIp2, 1)}},
      {ECMP_WEIGHT, ECMP_WEIGHT});
}

/*
 * Four interfaces: I1, I2, I3, I4
 * Three routes which require resolution: R1, R2, R3
 * R1 has R2 and R3 as next hops with weights 3 and 2
 * R2 has I1 and I2 as next hops with weights 5 and 4
 * R3 has I3 and I4 as next hops with ECMP
 * expect R1 to resolve to ECMP between I1, I2, I3, I4
 */
TEST_F(UcmpTest, recursiveEcmpPropagatesUp) {
  runTwoDeepRecursiveTest({{3, 2}, {5, 4}, {0, 0}}, {0, 0, 0, 0});
}

/*
 * Four interfaces: I1, I2, I3, I4
 * Three routes which require resolution: R1, R2, R3
 * R1 has R2 and R3 as next hops with weights 3 and 2
 * R2 has I1 and I2 as next hops with weights 5 and 4
 * R3 has I3 and I4 as next hops with weights 0 (ECMP) and 1
 * expect R1 to resolve to ECMP between I1, I2, I3, I4
 */
TEST_F(UcmpTest, recursiveMixedEcmpPropagatesUp) {
  runTwoDeepRecursiveTest({{3, 2}, {5, 4}, {0, 1}}, {0, 0, 0, 0});
}

/*
 * Four interfaces: I1, I2, I3, I4
 * Three routes which require resolution: R1, R2, R3
 * R1 has R2 and R3 as next hops with ECMP
 * R2 has I1 and I2 as next hops with weights 5 and 4
 * R3 has I3 and I4 as next hops with weights 3 and 2
 * expect R1 to resolve to ECMP between I1, I2, I3, I4
 */
TEST_F(UcmpTest, recursiveEcmpPropagatesDown) {
  runTwoDeepRecursiveTest({{0, 0}, {5, 4}, {3, 2}}, {0, 0, 0, 0});
}

/*
 * Two interfaces: I1, I2
 * Two routes which require resolution: R1, R2
 * R1 has I1 and I2 as next hops with ECMP
 * R2 has I1 and I2 as next hops with weights 2 and 1
 * expect R1 to resolve to ECMP between I1, I2
 * expect R2 to resolve to I1:2, I2: 1
 */
TEST_F(UcmpTest, separateEcmpUcmp) {
  runRecursiveTest(
      {{UnresolvedNextHop(intfIp1, ECMP_WEIGHT),
        UnresolvedNextHop(intfIp2, ECMP_WEIGHT)},
       {UnresolvedNextHop(intfIp1, 2), UnresolvedNextHop(intfIp2, 1)}},
      {ECMP_WEIGHT, ECMP_WEIGHT});
  RouteNextHopSet route2ExpFwd;
  route2ExpFwd.emplace(
      ResolvedNextHop(IPAddress("1.1.1.10"), InterfaceID(1), 2));
  route2ExpFwd.emplace(
      ResolvedNextHop(IPAddress("2.2.2.20"), InterfaceID(2), 1));
  EXPECT_EQ(route2ExpFwd, resolvedRoutes[1].getForwardInfo().getNextHopSet());
}

// The following set of tests will start with 4 next hops all weight 100
// then vary one next hop by 10 weight increments to 90, 80, ... , 10

TEST_F(UcmpTest, Hundred) {
  runVaryFromHundredTest(100, {1, 1, 1, 1});
}

TEST_F(UcmpTest, Ninety) {
  runVaryFromHundredTest(90, {10, 10, 10, 9});
}

TEST_F(UcmpTest, Eighty) {
  runVaryFromHundredTest(80, {5, 5, 5, 4});
}

TEST_F(UcmpTest, Seventy) {
  runVaryFromHundredTest(70, {10, 10, 10, 7});
}

TEST_F(UcmpTest, Sixty) {
  runVaryFromHundredTest(60, {5, 5, 5, 3});
}

TEST_F(UcmpTest, Fifty) {
  runVaryFromHundredTest(50, {2, 2, 2, 1});
}

TEST_F(UcmpTest, Forty) {
  runVaryFromHundredTest(40, {5, 5, 5, 2});
}

TEST_F(UcmpTest, Thirty) {
  runVaryFromHundredTest(30, {10, 10, 10, 3});
}

TEST_F(UcmpTest, Twenty) {
  runVaryFromHundredTest(20, {5, 5, 5, 1});
}

TEST_F(UcmpTest, Ten) {
  runVaryFromHundredTest(10, {10, 10, 10, 1});
}

} // namespace facebook::fboss::rib
