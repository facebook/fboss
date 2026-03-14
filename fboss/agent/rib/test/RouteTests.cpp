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
#include "fboss/agent/rib/NextHopIDManager.h"
#include "fboss/agent/state/RouteNextHop.h"

#include "fboss/agent/rib/RouteUpdater.h"

#include <folly/IPAddress.h>
#include <folly/json/dynamic.h>

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

  NextHopIDManager nhopIds;
  RibRouteUpdater u2(&v4Routes, &v6Routes, &nhopIds);
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

  NextHopIDManager nhopIds;
  RibRouteUpdater u2(&v4Routes, &v6Routes, &mplsRoutes, &nhopIds);
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

TEST(Route, addRouteWithSrv6NextHops) {
  IPv4NetworkToRouteMap v4Routes;
  IPv6NetworkToRouteMap v6Routes;

  const std::vector<folly::IPAddressV6> segList{
      folly::IPAddressV6("2001:db8::1"), folly::IPAddressV6("2001:db8::2")};

  RouteNextHopSet srv6Nhops;
  srv6Nhops.emplace(UnresolvedNextHop(
      IPAddress("1.1.1.10"),
      ECMP_WEIGHT,
      std::nullopt, // action
      std::nullopt, // disableTTLDecrement
      std::nullopt, // topologyInfo
      std::nullopt, // adjustedWeight
      segList,
      TunnelType::SRV6_ENCAP,
      std::string("tunnel_1")));
  srv6Nhops.emplace(UnresolvedNextHop(
      IPAddress("2.2.2.10"),
      ECMP_WEIGHT,
      std::nullopt,
      std::nullopt,
      std::nullopt,
      std::nullopt,
      segList,
      TunnelType::SRV6_ENCAP,
      std::string("tunnel_2")));

  RouteV4::Prefix r1{IPAddressV4("10.1.1.0"), 24};
  RouteV6::Prefix r2{IPAddressV6("3001::0"), 48};

  NextHopIDManager nhopIds;
  RibRouteUpdater u(&v4Routes, &v6Routes, &nhopIds);
  u.update<RibRouteUpdater::RouteEntry, folly::CIDRNetwork>(
      kClientA,
      {
          {{r1.network(), r1.mask()}, RouteNextHopEntry(srv6Nhops, kDistance)},
          {{r2.network(), r2.mask()}, RouteNextHopEntry(srv6Nhops, kDistance)},
      },
      {},
      false);

  // Verify routes were added
  EXPECT_EQ(v4Routes.size(), 1);
  EXPECT_EQ(v6Routes.size(), 1);

  // Verify SRv6 fields are preserved in the stored routes
  auto v4It = v4Routes.exactMatch(r1.network(), r1.mask());
  ASSERT_NE(v4Routes.end(), v4It);
  auto v4Route = v4It->value();
  auto v4Nhops = v4Route->getEntryForClient(kClientA);
  ASSERT_TRUE(v4Nhops);
  for (const auto& nh : v4Nhops->getNextHopSet()) {
    EXPECT_EQ(nh.srv6SegmentList(), segList);
    EXPECT_EQ(nh.tunnelType(), TunnelType::SRV6_ENCAP);
    EXPECT_TRUE(nh.tunnelId() == "tunnel_1" || nh.tunnelId() == "tunnel_2");
  }
}

TEST(Route, serializeRouteTableWithSrv6) {
  IPv4NetworkToRouteMap v4Routes;
  IPv6NetworkToRouteMap v6Routes;
  LabelToRouteMap mplsRoutes;

  const std::vector<folly::IPAddressV6> segList{
      folly::IPAddressV6("2001:db8::1")};

  RouteNextHopSet srv6Nhops;
  srv6Nhops.emplace(UnresolvedNextHop(
      IPAddress("1.1.1.10"),
      ECMP_WEIGHT,
      std::nullopt,
      std::nullopt,
      std::nullopt,
      std::nullopt,
      segList,
      TunnelType::SRV6_ENCAP,
      std::string("tunnel_1")));

  // Also add a regular (non-SRv6) nexthop
  RouteNextHopSet regularNhops = makeNextHops({"2.2.2.10"});

  RouteV4::Prefix r1{IPAddressV4("10.1.1.0"), 24};
  RouteV4::Prefix r2{IPAddressV4("20.1.1.0"), 24};

  NextHopIDManager nhopIds;
  RibRouteUpdater u(&v4Routes, &v6Routes, &mplsRoutes, &nhopIds);
  u.update<RibRouteUpdater::RouteEntry, folly::CIDRNetwork>(
      kClientA,
      {
          {{r1.network(), r1.mask()}, RouteNextHopEntry(srv6Nhops, kDistance)},
          {{r2.network(), r2.mask()},
           RouteNextHopEntry(regularNhops, kDistance)},
      },
      {},
      false);

  // Verify both routes exist
  EXPECT_EQ(v4Routes.size(), 2);

  // Verify SRv6 route preserved its fields
  auto it = v4Routes.exactMatch(r1.network(), r1.mask());
  ASSERT_NE(v4Routes.end(), it);
  auto route = it->value();
  auto entry = route->getEntryForClient(kClientA);
  ASSERT_TRUE(entry);
  const auto& nhops = entry->getNextHopSet();
  ASSERT_EQ(nhops.size(), 1);
  const auto& nh = *nhops.begin();
  EXPECT_EQ(nh.srv6SegmentList(), segList);
  EXPECT_EQ(nh.tunnelType(), TunnelType::SRV6_ENCAP);
  EXPECT_EQ(nh.tunnelId(), "tunnel_1");

  // Verify regular route has empty SRv6 fields
  auto it2 = v4Routes.exactMatch(r2.network(), r2.mask());
  ASSERT_NE(v4Routes.end(), it2);
  auto route2 = it2->value();
  auto entry2 = route2->getEntryForClient(kClientA);
  ASSERT_TRUE(entry2);
  const auto& nhops2 = entry2->getNextHopSet();
  ASSERT_EQ(nhops2.size(), 1);
  const auto& nh2 = *nhops2.begin();
  EXPECT_TRUE(nh2.srv6SegmentList().empty());
  EXPECT_FALSE(nh2.tunnelType().has_value());
  EXPECT_FALSE(nh2.tunnelId().has_value());
}

// Test that SRv6 fields survive through ECMP route resolution
// (mergeForwardInfosEcmp path - weight=0)
TEST(Route, resolveEcmpRouteWithSrv6NextHops) {
  IPv4NetworkToRouteMap v4Routes;
  IPv6NetworkToRouteMap v6Routes;

  const std::vector<folly::IPAddressV6> segList{
      folly::IPAddressV6("2001:db8::1"), folly::IPAddressV6("2001:db8::2")};

  // Step 1: Add interface routes so nexthops can be resolved
  // Interface route for 1.1.1.0/24 via interface 1
  RouteNextHopSet intfNhop1;
  intfNhop1.emplace(ResolvedNextHop(
      IPAddress("1.1.1.1"), InterfaceID(1), UCMP_DEFAULT_WEIGHT));
  // Interface route for 2.2.2.0/24 via interface 2
  RouteNextHopSet intfNhop2;
  intfNhop2.emplace(ResolvedNextHop(
      IPAddress("2.2.2.2"), InterfaceID(2), UCMP_DEFAULT_WEIGHT));

  NextHopIDManager nhopIds;
  RibRouteUpdater u(&v4Routes, &v6Routes, &nhopIds);
  u.update<RibRouteUpdater::RouteEntry, folly::CIDRNetwork>(
      ClientID::INTERFACE_ROUTE,
      {
          {{IPAddress("1.1.1.0"), 24},
           RouteNextHopEntry(intfNhop1, AdminDistance::DIRECTLY_CONNECTED)},
          {{IPAddress("2.2.2.0"), 24},
           RouteNextHopEntry(intfNhop2, AdminDistance::DIRECTLY_CONNECTED)},
      },
      {},
      false);

  // Step 2: Add a route with SRv6 ECMP nexthops (weight=0 triggers ECMP merge)
  RouteNextHopSet srv6Nhops;
  srv6Nhops.emplace(UnresolvedNextHop(
      IPAddress("1.1.1.10"),
      ECMP_WEIGHT,
      std::nullopt, // action
      std::nullopt, // disableTTLDecrement
      std::nullopt, // topologyInfo
      std::nullopt, // adjustedWeight
      segList,
      TunnelType::SRV6_ENCAP,
      std::string("tunnel_1")));
  srv6Nhops.emplace(UnresolvedNextHop(
      IPAddress("2.2.2.10"),
      ECMP_WEIGHT,
      std::nullopt,
      std::nullopt,
      std::nullopt,
      std::nullopt,
      segList,
      TunnelType::SRV6_ENCAP,
      std::string("tunnel_2")));

  RouteV4::Prefix r1{IPAddressV4("10.1.1.0"), 24};

  u.update<RibRouteUpdater::RouteEntry, folly::CIDRNetwork>(
      kClientA,
      {
          {{r1.network(), r1.mask()}, RouteNextHopEntry(srv6Nhops, kDistance)},
      },
      {},
      false);

  // Step 3: Verify the route is resolved and SRv6 fields are preserved
  auto it = v4Routes.exactMatch(r1.network(), r1.mask());
  ASSERT_NE(v4Routes.end(), it);
  auto route = it->value();
  EXPECT_TRUE(route->isResolved());

  const auto& fwdInfo = route->getForwardInfo();
  const auto& resolvedNhops = fwdInfo.getNextHopSet();
  ASSERT_EQ(resolvedNhops.size(), 2);

  for (const auto& nh : resolvedNhops) {
    EXPECT_EQ(nh.srv6SegmentList(), segList);
    EXPECT_EQ(nh.tunnelType(), TunnelType::SRV6_ENCAP);
    EXPECT_TRUE(nh.tunnelId() == "tunnel_1" || nh.tunnelId() == "tunnel_2");
    // Verify resolved to correct interfaces
    EXPECT_TRUE(nh.isResolved());
    EXPECT_TRUE(nh.intf() == InterfaceID(1) || nh.intf() == InterfaceID(2));
    // ECMP path sets weight to 0
    EXPECT_EQ(nh.weight(), ECMP_WEIGHT);
  }
}

// Test that SRv6 fields survive through UCMP route resolution
// (combineWeights + optimizeWeights path - non-zero weights)
TEST(Route, resolveUcmpRouteWithSrv6NextHops) {
  IPv4NetworkToRouteMap v4Routes;
  IPv6NetworkToRouteMap v6Routes;

  const std::vector<folly::IPAddressV6> segList{
      folly::IPAddressV6("3001:db8:1::"),
      folly::IPAddressV6("3001:db8:2::"),
      folly::IPAddressV6("3001:db8:3::")};

  // Step 1: Add interface routes
  RouteNextHopSet intfNhop1;
  intfNhop1.emplace(ResolvedNextHop(
      IPAddress("1.1.1.1"), InterfaceID(1), UCMP_DEFAULT_WEIGHT));
  RouteNextHopSet intfNhop2;
  intfNhop2.emplace(ResolvedNextHop(
      IPAddress("2.2.2.2"), InterfaceID(2), UCMP_DEFAULT_WEIGHT));

  NextHopIDManager nhopIds;
  RibRouteUpdater u(&v4Routes, &v6Routes, &nhopIds);
  u.update<RibRouteUpdater::RouteEntry, folly::CIDRNetwork>(
      ClientID::INTERFACE_ROUTE,
      {
          {{IPAddress("1.1.1.0"), 24},
           RouteNextHopEntry(intfNhop1, AdminDistance::DIRECTLY_CONNECTED)},
          {{IPAddress("2.2.2.0"), 24},
           RouteNextHopEntry(intfNhop2, AdminDistance::DIRECTLY_CONNECTED)},
      },
      {},
      false);

  // Step 2: Add a route with SRv6 UCMP nexthops (non-zero weights)
  RouteNextHopSet srv6Nhops;
  srv6Nhops.emplace(UnresolvedNextHop(
      IPAddress("1.1.1.10"),
      3, // weight
      std::nullopt,
      std::nullopt,
      std::nullopt,
      std::nullopt,
      segList,
      TunnelType::SRV6_ENCAP,
      std::string("tunnel_1")));
  srv6Nhops.emplace(UnresolvedNextHop(
      IPAddress("2.2.2.10"),
      2, // weight
      std::nullopt,
      std::nullopt,
      std::nullopt,
      std::nullopt,
      segList,
      TunnelType::SRV6_ENCAP,
      std::string("tunnel_2")));

  RouteV4::Prefix r1{IPAddressV4("10.1.1.0"), 24};

  u.update<RibRouteUpdater::RouteEntry, folly::CIDRNetwork>(
      kClientA,
      {
          {{r1.network(), r1.mask()}, RouteNextHopEntry(srv6Nhops, kDistance)},
      },
      {},
      false);

  // Step 3: Verify the route is resolved and SRv6 fields are preserved
  auto it = v4Routes.exactMatch(r1.network(), r1.mask());
  ASSERT_NE(v4Routes.end(), it);
  auto route = it->value();
  EXPECT_TRUE(route->isResolved());

  const auto& fwdInfo = route->getForwardInfo();
  const auto& resolvedNhops = fwdInfo.getNextHopSet();
  ASSERT_EQ(resolvedNhops.size(), 2);

  for (const auto& nh : resolvedNhops) {
    EXPECT_EQ(nh.srv6SegmentList(), segList);
    EXPECT_EQ(nh.tunnelType(), TunnelType::SRV6_ENCAP);
    EXPECT_TRUE(nh.tunnelId() == "tunnel_1" || nh.tunnelId() == "tunnel_2");
    EXPECT_TRUE(nh.isResolved());
    EXPECT_TRUE(nh.intf() == InterfaceID(1) || nh.intf() == InterfaceID(2));
    // UCMP path preserves non-zero weights (optimized)
    EXPECT_GT(nh.weight(), 0);
  }

  // Verify weight ratio is preserved (3:2)
  std::map<InterfaceID, NextHopWeight> weightByIntf;
  for (const auto& nh : resolvedNhops) {
    weightByIntf[nh.intf()] = nh.weight();
  }
  // Weight ratio should be 3:2 (or equivalent after optimization)
  EXPECT_EQ(weightByIntf[InterfaceID(1)] * 2, weightByIntf[InterfaceID(2)] * 3);
}

// Test that SRv6 fields survive through IPv6 route resolution
TEST(Route, resolveV6RouteWithSrv6NextHops) {
  IPv4NetworkToRouteMap v4Routes;
  IPv6NetworkToRouteMap v6Routes;

  const std::vector<folly::IPAddressV6> segList{
      folly::IPAddressV6("2001:db8::1")};

  // Step 1: Add IPv6 interface route
  RouteNextHopSet intfNhop;
  intfNhop.emplace(ResolvedNextHop(
      IPAddress("fc00::1"), InterfaceID(1), UCMP_DEFAULT_WEIGHT));

  NextHopIDManager nhopIds;
  RibRouteUpdater u(&v4Routes, &v6Routes, &nhopIds);
  u.update<RibRouteUpdater::RouteEntry, folly::CIDRNetwork>(
      ClientID::INTERFACE_ROUTE,
      {
          {{IPAddress("fc00::"), 64},
           RouteNextHopEntry(intfNhop, AdminDistance::DIRECTLY_CONNECTED)},
      },
      {},
      false);

  // Step 2: Add a v6 route with SRv6 nexthop
  RouteNextHopSet srv6Nhops;
  srv6Nhops.emplace(UnresolvedNextHop(
      IPAddress("fc00::10"),
      ECMP_WEIGHT,
      std::nullopt,
      std::nullopt,
      std::nullopt,
      std::nullopt,
      segList,
      TunnelType::SRV6_ENCAP,
      std::string("srv6Tunnel0")));

  RouteV6::Prefix r1{IPAddressV6("2800:2::"), 64};

  u.update<RibRouteUpdater::RouteEntry, folly::CIDRNetwork>(
      kClientA,
      {
          {{r1.network(), r1.mask()}, RouteNextHopEntry(srv6Nhops, kDistance)},
      },
      {},
      false);

  // Step 3: Verify
  auto it = v6Routes.exactMatch(r1.network(), r1.mask());
  ASSERT_NE(v6Routes.end(), it);
  auto route = it->value();
  EXPECT_TRUE(route->isResolved());

  const auto& fwdInfo = route->getForwardInfo();
  const auto& resolvedNhops = fwdInfo.getNextHopSet();
  ASSERT_EQ(resolvedNhops.size(), 1);

  const auto& nh = *resolvedNhops.begin();
  EXPECT_EQ(nh.srv6SegmentList(), segList);
  EXPECT_EQ(nh.tunnelType(), TunnelType::SRV6_ENCAP);
  EXPECT_EQ(nh.tunnelId(), "srv6Tunnel0");
  EXPECT_TRUE(nh.isResolved());
  EXPECT_EQ(nh.intf(), InterfaceID(1));
}

// Test mixed SRv6 and plain nexthops through ECMP merge resolution
TEST(Route, resolveEcmpMixedSrv6AndPlainNextHops) {
  IPv4NetworkToRouteMap v4Routes;
  IPv6NetworkToRouteMap v6Routes;

  const std::vector<folly::IPAddressV6> segList{
      folly::IPAddressV6("2001:db8::1")};

  // Step 1: Add interface routes
  RouteNextHopSet intfNhop1;
  intfNhop1.emplace(ResolvedNextHop(
      IPAddress("1.1.1.1"), InterfaceID(1), UCMP_DEFAULT_WEIGHT));
  RouteNextHopSet intfNhop2;
  intfNhop2.emplace(ResolvedNextHop(
      IPAddress("2.2.2.2"), InterfaceID(2), UCMP_DEFAULT_WEIGHT));

  NextHopIDManager nhopIds;
  RibRouteUpdater u(&v4Routes, &v6Routes, &nhopIds);
  u.update<RibRouteUpdater::RouteEntry, folly::CIDRNetwork>(
      ClientID::INTERFACE_ROUTE,
      {
          {{IPAddress("1.1.1.0"), 24},
           RouteNextHopEntry(intfNhop1, AdminDistance::DIRECTLY_CONNECTED)},
          {{IPAddress("2.2.2.0"), 24},
           RouteNextHopEntry(intfNhop2, AdminDistance::DIRECTLY_CONNECTED)},
      },
      {},
      false);

  // Step 2: Add a route with a mix of SRv6 and plain nexthops
  RouteNextHopSet mixedNhops;
  // SRv6 nexthop
  mixedNhops.emplace(UnresolvedNextHop(
      IPAddress("1.1.1.10"),
      ECMP_WEIGHT,
      std::nullopt,
      std::nullopt,
      std::nullopt,
      std::nullopt,
      segList,
      TunnelType::SRV6_ENCAP,
      std::string("tunnel_1")));
  // Plain nexthop (no SRv6)
  mixedNhops.emplace(UnresolvedNextHop(IPAddress("2.2.2.10"), ECMP_WEIGHT));

  RouteV4::Prefix r1{IPAddressV4("10.1.1.0"), 24};

  u.update<RibRouteUpdater::RouteEntry, folly::CIDRNetwork>(
      kClientA,
      {
          {{r1.network(), r1.mask()}, RouteNextHopEntry(mixedNhops, kDistance)},
      },
      {},
      false);

  // Step 3: Verify both nexthops are resolved with correct SRv6 fields
  auto it = v4Routes.exactMatch(r1.network(), r1.mask());
  ASSERT_NE(v4Routes.end(), it);
  auto route = it->value();
  EXPECT_TRUE(route->isResolved());

  const auto& fwdInfo = route->getForwardInfo();
  const auto& resolvedNhops = fwdInfo.getNextHopSet();
  ASSERT_EQ(resolvedNhops.size(), 2);

  bool foundSrv6Nhop = false;
  bool foundPlainNhop = false;
  for (const auto& nh : resolvedNhops) {
    EXPECT_TRUE(nh.isResolved());
    if (nh.intf() == InterfaceID(1)) {
      // SRv6 nexthop
      EXPECT_EQ(nh.srv6SegmentList(), segList);
      EXPECT_EQ(nh.tunnelType(), TunnelType::SRV6_ENCAP);
      EXPECT_EQ(nh.tunnelId(), "tunnel_1");
      foundSrv6Nhop = true;
    } else if (nh.intf() == InterfaceID(2)) {
      // Plain nexthop - should have empty SRv6 fields
      EXPECT_TRUE(nh.srv6SegmentList().empty());
      EXPECT_FALSE(nh.tunnelType().has_value());
      EXPECT_FALSE(nh.tunnelId().has_value());
      foundPlainNhop = true;
    }
  }
  EXPECT_TRUE(foundSrv6Nhop);
  EXPECT_TRUE(foundPlainNhop);
}

// Test that different SRv6 tunnelIds on same interface are kept distinct
// through UCMP resolution (NextHopCombinedWeightsKey differentiation)
TEST(Route, resolveUcmpDistinctSrv6TunnelIds) {
  IPv4NetworkToRouteMap v4Routes;
  IPv6NetworkToRouteMap v6Routes;

  const std::vector<folly::IPAddressV6> segListA{
      folly::IPAddressV6("2001:db8::1")};
  const std::vector<folly::IPAddressV6> segListB{
      folly::IPAddressV6("2001:db8::2")};

  // Step 1: Add interface route
  RouteNextHopSet intfNhop;
  intfNhop.emplace(ResolvedNextHop(
      IPAddress("1.1.1.1"), InterfaceID(1), UCMP_DEFAULT_WEIGHT));

  NextHopIDManager nhopIds;
  RibRouteUpdater u(&v4Routes, &v6Routes, &nhopIds);
  u.update<RibRouteUpdater::RouteEntry, folly::CIDRNetwork>(
      ClientID::INTERFACE_ROUTE,
      {
          {{IPAddress("1.1.1.0"), 24},
           RouteNextHopEntry(intfNhop, AdminDistance::DIRECTLY_CONNECTED)},
      },
      {},
      false);

  // Step 2: Add a route with two SRv6 nexthops to same IP but different
  // tunnels (different segment lists and tunnel IDs)
  RouteNextHopSet srv6Nhops;
  srv6Nhops.emplace(UnresolvedNextHop(
      IPAddress("1.1.1.10"),
      5, // weight
      std::nullopt,
      std::nullopt,
      std::nullopt,
      std::nullopt,
      segListA,
      TunnelType::SRV6_ENCAP,
      std::string("tunnel_A")));
  srv6Nhops.emplace(UnresolvedNextHop(
      IPAddress("1.1.1.10"),
      3, // weight
      std::nullopt,
      std::nullopt,
      std::nullopt,
      std::nullopt,
      segListB,
      TunnelType::SRV6_ENCAP,
      std::string("tunnel_B")));

  RouteV4::Prefix r1{IPAddressV4("10.1.1.0"), 24};

  u.update<RibRouteUpdater::RouteEntry, folly::CIDRNetwork>(
      kClientA,
      {
          {{r1.network(), r1.mask()}, RouteNextHopEntry(srv6Nhops, kDistance)},
      },
      {},
      false);

  // Step 3: Verify both nexthops are distinct in resolved set
  // (NextHopCombinedWeightsKey must differentiate by SRv6 fields)
  auto it = v4Routes.exactMatch(r1.network(), r1.mask());
  ASSERT_NE(v4Routes.end(), it);
  auto route = it->value();
  EXPECT_TRUE(route->isResolved());

  const auto& fwdInfo = route->getForwardInfo();
  const auto& resolvedNhops = fwdInfo.getNextHopSet();
  // Both should be present as distinct nexthops
  ASSERT_EQ(resolvedNhops.size(), 2);

  bool foundTunnelA = false;
  bool foundTunnelB = false;
  for (const auto& nh : resolvedNhops) {
    EXPECT_TRUE(nh.isResolved());
    EXPECT_EQ(nh.intf(), InterfaceID(1));
    EXPECT_EQ(nh.tunnelType(), TunnelType::SRV6_ENCAP);
    if (nh.tunnelId() == "tunnel_A") {
      EXPECT_EQ(nh.srv6SegmentList(), segListA);
      foundTunnelA = true;
    } else if (nh.tunnelId() == "tunnel_B") {
      EXPECT_EQ(nh.srv6SegmentList(), segListB);
      foundTunnelB = true;
    }
  }
  EXPECT_TRUE(foundTunnelA);
  EXPECT_TRUE(foundTunnelB);
}

} // namespace facebook::fboss
