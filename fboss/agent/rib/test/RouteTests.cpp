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
#include "fboss/agent/rib/FibUpdateHelpers.h"
#include "fboss/agent/rib/NetworkToRouteMap.h"
#include "fboss/agent/rib/NextHopIDManager.h"
#include "fboss/agent/rib/RoutingInformationBase.h"
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
const std::string kSrv6Tunnel0{"srv6Tunnel0"};
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
  RibRouteUpdater u2(&v4Routes, &v6Routes, &nhopIds, nullptr);
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
  RibRouteUpdater u2(&v4Routes, &v6Routes, &mplsRoutes, &nhopIds, nullptr);
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
      kSrv6Tunnel0));
  srv6Nhops.emplace(UnresolvedNextHop(
      IPAddress("2.2.2.10"),
      ECMP_WEIGHT,
      std::nullopt,
      std::nullopt,
      std::nullopt,
      std::nullopt,
      segList,
      TunnelType::SRV6_ENCAP,
      kSrv6Tunnel0));

  RouteV4::Prefix r1{IPAddressV4("10.1.1.0"), 24};
  RouteV6::Prefix r2{IPAddressV6("3001::0"), 48};

  NextHopIDManager nhopIds;
  RibRouteUpdater u(&v4Routes, &v6Routes, &nhopIds, nullptr);
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
    EXPECT_EQ(nh.tunnelId(), kSrv6Tunnel0);
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
      kSrv6Tunnel0));

  // Also add a regular (non-SRv6) nexthop
  RouteNextHopSet regularNhops = makeNextHops({"2.2.2.10"});

  RouteV4::Prefix r1{IPAddressV4("10.1.1.0"), 24};
  RouteV4::Prefix r2{IPAddressV4("20.1.1.0"), 24};

  NextHopIDManager nhopIds;
  RibRouteUpdater u(&v4Routes, &v6Routes, &mplsRoutes, &nhopIds, nullptr);
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
  EXPECT_EQ(nh.tunnelId(), kSrv6Tunnel0);

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
  RibRouteUpdater u(&v4Routes, &v6Routes, &nhopIds, nullptr);
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
      kSrv6Tunnel0));
  srv6Nhops.emplace(UnresolvedNextHop(
      IPAddress("2.2.2.10"),
      ECMP_WEIGHT,
      std::nullopt,
      std::nullopt,
      std::nullopt,
      std::nullopt,
      segList,
      TunnelType::SRV6_ENCAP,
      kSrv6Tunnel0));

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
    EXPECT_EQ(nh.tunnelId(), kSrv6Tunnel0);
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
  RibRouteUpdater u(&v4Routes, &v6Routes, &nhopIds, nullptr);
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
      kSrv6Tunnel0));
  srv6Nhops.emplace(UnresolvedNextHop(
      IPAddress("2.2.2.10"),
      2, // weight
      std::nullopt,
      std::nullopt,
      std::nullopt,
      std::nullopt,
      segList,
      TunnelType::SRV6_ENCAP,
      kSrv6Tunnel0));

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
    EXPECT_EQ(nh.tunnelId(), kSrv6Tunnel0);
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
  RibRouteUpdater u(&v4Routes, &v6Routes, &nhopIds, nullptr);
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
      kSrv6Tunnel0));

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
  EXPECT_EQ(nh.tunnelId(), kSrv6Tunnel0);
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
  RibRouteUpdater u(&v4Routes, &v6Routes, &nhopIds, nullptr);
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
      kSrv6Tunnel0));
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
      EXPECT_EQ(nh.tunnelId(), kSrv6Tunnel0);
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

// Test that different SRv6 SID lists on same interface are kept distinct
// through UCMP resolution (NextHopCombinedWeightsKey differentiation)
TEST(Route, resolveUcmpDistinctSrv6SegmentLists) {
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
  RibRouteUpdater u(&v4Routes, &v6Routes, &nhopIds, nullptr);
  u.update<RibRouteUpdater::RouteEntry, folly::CIDRNetwork>(
      ClientID::INTERFACE_ROUTE,
      {
          {{IPAddress("1.1.1.0"), 24},
           RouteNextHopEntry(intfNhop, AdminDistance::DIRECTLY_CONNECTED)},
      },
      {},
      false);

  // Step 2: Add a route with two SRv6 nexthops to same IP and same tunnel
  // but different segment lists
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
      kSrv6Tunnel0));
  srv6Nhops.emplace(UnresolvedNextHop(
      IPAddress("1.1.1.10"),
      3, // weight
      std::nullopt,
      std::nullopt,
      std::nullopt,
      std::nullopt,
      segListB,
      TunnelType::SRV6_ENCAP,
      kSrv6Tunnel0));

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

  bool foundSegListA = false;
  bool foundSegListB = false;
  for (const auto& nh : resolvedNhops) {
    EXPECT_TRUE(nh.isResolved());
    EXPECT_EQ(nh.intf(), InterfaceID(1));
    EXPECT_EQ(nh.tunnelType(), TunnelType::SRV6_ENCAP);
    EXPECT_EQ(nh.tunnelId(), kSrv6Tunnel0);
    if (nh.srv6SegmentList() == segListA) {
      foundSegListA = true;
    } else if (nh.srv6SegmentList() == segListB) {
      foundSegListB = true;
    }
  }
  EXPECT_TRUE(foundSegListA);
  EXPECT_TRUE(foundSegListB);
}

TEST(Route, resolveEcmpRouteWithCost) {
  IPv4NetworkToRouteMap v4Routes;
  IPv6NetworkToRouteMap v6Routes;

  RouteNextHopSet intfNhop1;
  intfNhop1.emplace(ResolvedNextHop(
      IPAddress("fc00:1::1"), InterfaceID(1), UCMP_DEFAULT_WEIGHT));
  RouteNextHopSet intfNhop2;
  intfNhop2.emplace(ResolvedNextHop(
      IPAddress("fc00:2::1"), InterfaceID(2), UCMP_DEFAULT_WEIGHT));

  NextHopIDManager nhopIds;
  RibRouteUpdater u(&v4Routes, &v6Routes, &nhopIds, nullptr);
  u.update<RibRouteUpdater::RouteEntry, folly::CIDRNetwork>(
      ClientID::INTERFACE_ROUTE,
      {
          {{IPAddress("fc00:1::"), 64},
           RouteNextHopEntry(intfNhop1, AdminDistance::DIRECTLY_CONNECTED)},
          {{IPAddress("fc00:2::"), 64},
           RouteNextHopEntry(intfNhop2, AdminDistance::DIRECTLY_CONNECTED)},
      },
      {},
      false);

  RouteNextHopSet nhops;
  nhops.emplace(UnresolvedNextHop(
      IPAddress("fc00:1::10"),
      ECMP_WEIGHT,
      std::nullopt,
      std::nullopt,
      std::nullopt,
      std::nullopt,
      {},
      std::nullopt,
      std::nullopt,
      int64_t(100)));
  nhops.emplace(UnresolvedNextHop(
      IPAddress("fc00:2::10"),
      ECMP_WEIGHT,
      std::nullopt,
      std::nullopt,
      std::nullopt,
      std::nullopt,
      {},
      std::nullopt,
      std::nullopt,
      int64_t(200)));

  RouteV6::Prefix r1{IPAddressV6("2800:1::"), 64};

  u.update<RibRouteUpdater::RouteEntry, folly::CIDRNetwork>(
      kClientA,
      {
          {{r1.network(), r1.mask()}, RouteNextHopEntry(nhops, kDistance)},
      },
      {},
      false);

  auto it = v6Routes.exactMatch(r1.network(), r1.mask());
  ASSERT_NE(v6Routes.end(), it);
  auto route = it->value();
  EXPECT_TRUE(route->isResolved());

  const auto& resolvedNhops = route->getForwardInfo().getNextHopSet();
  ASSERT_EQ(resolvedNhops.size(), 2);

  for (const auto& nh : resolvedNhops) {
    EXPECT_TRUE(nh.isResolved());
    EXPECT_TRUE(nh.cost().has_value());
    if (nh.intf() == InterfaceID(1)) {
      EXPECT_EQ(nh.cost(), int64_t(100));
    } else {
      EXPECT_EQ(nh.cost(), int64_t(200));
    }
  }
}

TEST(Route, resolveUcmpRouteWithCost) {
  IPv4NetworkToRouteMap v4Routes;
  IPv6NetworkToRouteMap v6Routes;

  RouteNextHopSet intfNhop1;
  intfNhop1.emplace(ResolvedNextHop(
      IPAddress("fc00:1::1"), InterfaceID(1), UCMP_DEFAULT_WEIGHT));
  RouteNextHopSet intfNhop2;
  intfNhop2.emplace(ResolvedNextHop(
      IPAddress("fc00:2::1"), InterfaceID(2), UCMP_DEFAULT_WEIGHT));

  NextHopIDManager nhopIds;
  RibRouteUpdater u(&v4Routes, &v6Routes, &nhopIds, nullptr);
  u.update<RibRouteUpdater::RouteEntry, folly::CIDRNetwork>(
      ClientID::INTERFACE_ROUTE,
      {
          {{IPAddress("fc00:1::"), 64},
           RouteNextHopEntry(intfNhop1, AdminDistance::DIRECTLY_CONNECTED)},
          {{IPAddress("fc00:2::"), 64},
           RouteNextHopEntry(intfNhop2, AdminDistance::DIRECTLY_CONNECTED)},
      },
      {},
      false);

  RouteNextHopSet nhops;
  nhops.emplace(UnresolvedNextHop(
      IPAddress("fc00:1::10"),
      3,
      std::nullopt,
      std::nullopt,
      std::nullopt,
      std::nullopt,
      {},
      std::nullopt,
      std::nullopt,
      int64_t(500)));
  nhops.emplace(UnresolvedNextHop(
      IPAddress("fc00:2::10"),
      2,
      std::nullopt,
      std::nullopt,
      std::nullopt,
      std::nullopt,
      {},
      std::nullopt,
      std::nullopt,
      int64_t(600)));

  RouteV6::Prefix r1{IPAddressV6("2800:1::"), 64};

  u.update<RibRouteUpdater::RouteEntry, folly::CIDRNetwork>(
      kClientA,
      {
          {{r1.network(), r1.mask()}, RouteNextHopEntry(nhops, kDistance)},
      },
      {},
      false);

  auto it = v6Routes.exactMatch(r1.network(), r1.mask());
  ASSERT_NE(v6Routes.end(), it);
  auto route = it->value();
  EXPECT_TRUE(route->isResolved());

  const auto& resolvedNhops = route->getForwardInfo().getNextHopSet();
  ASSERT_EQ(resolvedNhops.size(), 2);

  for (const auto& nh : resolvedNhops) {
    EXPECT_TRUE(nh.isResolved());
    EXPECT_TRUE(nh.cost().has_value());
    if (nh.intf() == InterfaceID(1)) {
      EXPECT_EQ(nh.cost(), int64_t(500));
    } else {
      EXPECT_EQ(nh.cost(), int64_t(600));
    }
  }
}

TEST(Route, resolveUcmpDistinctCosts) {
  IPv4NetworkToRouteMap v4Routes;
  IPv6NetworkToRouteMap v6Routes;

  RouteNextHopSet intfNhop;
  intfNhop.emplace(ResolvedNextHop(
      IPAddress("fc00:1::1"), InterfaceID(1), UCMP_DEFAULT_WEIGHT));

  NextHopIDManager nhopIds;
  RibRouteUpdater u(&v4Routes, &v6Routes, &nhopIds, nullptr);
  u.update<RibRouteUpdater::RouteEntry, folly::CIDRNetwork>(
      ClientID::INTERFACE_ROUTE,
      {
          {{IPAddress("fc00:1::"), 64},
           RouteNextHopEntry(intfNhop, AdminDistance::DIRECTLY_CONNECTED)},
      },
      {},
      false);

  RouteNextHopSet nhops;
  nhops.emplace(UnresolvedNextHop(
      IPAddress("fc00:1::10"),
      5,
      std::nullopt,
      std::nullopt,
      std::nullopt,
      std::nullopt,
      {},
      std::nullopt,
      std::nullopt,
      int64_t(100)));
  nhops.emplace(UnresolvedNextHop(
      IPAddress("fc00:1::10"),
      3,
      std::nullopt,
      std::nullopt,
      std::nullopt,
      std::nullopt,
      {},
      std::nullopt,
      std::nullopt,
      int64_t(200)));

  RouteV6::Prefix r1{IPAddressV6("2800:1::"), 64};

  u.update<RibRouteUpdater::RouteEntry, folly::CIDRNetwork>(
      kClientA,
      {
          {{r1.network(), r1.mask()}, RouteNextHopEntry(nhops, kDistance)},
      },
      {},
      false);

  auto it = v6Routes.exactMatch(r1.network(), r1.mask());
  ASSERT_NE(v6Routes.end(), it);
  auto route = it->value();
  EXPECT_TRUE(route->isResolved());

  const auto& resolvedNhops = route->getForwardInfo().getNextHopSet();
  ASSERT_EQ(resolvedNhops.size(), 2);

  bool foundCost100 = false;
  bool foundCost200 = false;
  for (const auto& nh : resolvedNhops) {
    EXPECT_TRUE(nh.isResolved());
    EXPECT_EQ(nh.intf(), InterfaceID(1));
    if (nh.cost() == int64_t(100)) {
      foundCost100 = true;
    } else if (nh.cost() == int64_t(200)) {
      foundCost200 = true;
    }
  }
  EXPECT_TRUE(foundCost100);
  EXPECT_TRUE(foundCost200);
}

TEST(Route, resolveMixedCostAndNoCost) {
  IPv4NetworkToRouteMap v4Routes;
  IPv6NetworkToRouteMap v6Routes;

  RouteNextHopSet intfNhop1;
  intfNhop1.emplace(ResolvedNextHop(
      IPAddress("fc00:1::1"), InterfaceID(1), UCMP_DEFAULT_WEIGHT));
  RouteNextHopSet intfNhop2;
  intfNhop2.emplace(ResolvedNextHop(
      IPAddress("fc00:2::1"), InterfaceID(2), UCMP_DEFAULT_WEIGHT));

  NextHopIDManager nhopIds;
  RibRouteUpdater u(&v4Routes, &v6Routes, &nhopIds, nullptr);
  u.update<RibRouteUpdater::RouteEntry, folly::CIDRNetwork>(
      ClientID::INTERFACE_ROUTE,
      {
          {{IPAddress("fc00:1::"), 64},
           RouteNextHopEntry(intfNhop1, AdminDistance::DIRECTLY_CONNECTED)},
          {{IPAddress("fc00:2::"), 64},
           RouteNextHopEntry(intfNhop2, AdminDistance::DIRECTLY_CONNECTED)},
      },
      {},
      false);

  RouteNextHopSet nhops;
  nhops.emplace(UnresolvedNextHop(
      IPAddress("fc00:1::10"),
      ECMP_WEIGHT,
      std::nullopt,
      std::nullopt,
      std::nullopt,
      std::nullopt,
      {},
      std::nullopt,
      std::nullopt,
      int64_t(42)));
  nhops.emplace(UnresolvedNextHop(IPAddress("fc00:2::10"), ECMP_WEIGHT));

  RouteV6::Prefix r1{IPAddressV6("2800:1::"), 64};

  u.update<RibRouteUpdater::RouteEntry, folly::CIDRNetwork>(
      kClientA,
      {
          {{r1.network(), r1.mask()}, RouteNextHopEntry(nhops, kDistance)},
      },
      {},
      false);

  auto it = v6Routes.exactMatch(r1.network(), r1.mask());
  ASSERT_NE(v6Routes.end(), it);
  auto route = it->value();
  EXPECT_TRUE(route->isResolved());

  const auto& resolvedNhops = route->getForwardInfo().getNextHopSet();
  ASSERT_EQ(resolvedNhops.size(), 2);

  for (const auto& nh : resolvedNhops) {
    EXPECT_TRUE(nh.isResolved());
    if (nh.intf() == InterfaceID(1)) {
      EXPECT_EQ(nh.cost(), int64_t(42));
    } else {
      EXPECT_FALSE(nh.cost().has_value());
    }
  }
}

TEST(Route, resolveRecursiveRouteWithCost) {
  IPv4NetworkToRouteMap v4Routes;
  IPv6NetworkToRouteMap v6Routes;

  RouteNextHopSet intfNhop;
  intfNhop.emplace(ResolvedNextHop(
      IPAddress("fc00:1::1"), InterfaceID(1), UCMP_DEFAULT_WEIGHT));

  NextHopIDManager nhopIds;
  RibRouteUpdater u(&v4Routes, &v6Routes, &nhopIds, nullptr);
  u.update<RibRouteUpdater::RouteEntry, folly::CIDRNetwork>(
      ClientID::INTERFACE_ROUTE,
      {
          {{IPAddress("fc00:1::"), 64},
           RouteNextHopEntry(intfNhop, AdminDistance::DIRECTLY_CONNECTED)},
      },
      {},
      false);

  // Intermediate route: fc00:10::/32 via fc00:1::10 (resolves via interface)
  RouteNextHopSet intermediateNhops;
  intermediateNhops.emplace(
      UnresolvedNextHop(IPAddress("fc00:1::10"), ECMP_WEIGHT));

  u.update<RibRouteUpdater::RouteEntry, folly::CIDRNetwork>(
      kClientA,
      {
          {{IPAddress("fc00:10::"), 32},
           RouteNextHopEntry(intermediateNhops, kDistance)},
      },
      {},
      false);

  // Final route: 2800:2::/64 via fc00:10::10 with cost
  // Resolves recursively through the intermediate route
  RouteNextHopSet finalNhops;
  finalNhops.emplace(UnresolvedNextHop(
      IPAddress("fc00:10::10"),
      ECMP_WEIGHT,
      std::nullopt,
      std::nullopt,
      std::nullopt,
      std::nullopt,
      {},
      std::nullopt,
      std::nullopt,
      int64_t(999)));

  RouteV6::Prefix r1{IPAddressV6("2800:2::"), 64};

  u.update<RibRouteUpdater::RouteEntry, folly::CIDRNetwork>(
      kClientA,
      {
          {{r1.network(), r1.mask()}, RouteNextHopEntry(finalNhops, kDistance)},
      },
      {},
      false);

  auto it = v6Routes.exactMatch(r1.network(), r1.mask());
  ASSERT_NE(v6Routes.end(), it);
  auto route = it->value();
  EXPECT_TRUE(route->isResolved());

  const auto& resolvedNhops = route->getForwardInfo().getNextHopSet();
  ASSERT_EQ(resolvedNhops.size(), 1);

  const auto& nh = *resolvedNhops.begin();
  EXPECT_TRUE(nh.isResolved());
  EXPECT_EQ(nh.intf(), InterfaceID(1));
  EXPECT_EQ(nh.cost(), int64_t(999));
}

TEST(Route, resolveRecursiveUcmpRouteWithCost) {
  IPv4NetworkToRouteMap v4Routes;
  IPv6NetworkToRouteMap v6Routes;

  RouteNextHopSet intfNhop1;
  intfNhop1.emplace(ResolvedNextHop(
      IPAddress("fc00:1::1"), InterfaceID(1), UCMP_DEFAULT_WEIGHT));
  RouteNextHopSet intfNhop2;
  intfNhop2.emplace(ResolvedNextHop(
      IPAddress("fc00:2::1"), InterfaceID(2), UCMP_DEFAULT_WEIGHT));

  NextHopIDManager nhopIds;
  RibRouteUpdater u(&v4Routes, &v6Routes, &nhopIds, nullptr);
  u.update<RibRouteUpdater::RouteEntry, folly::CIDRNetwork>(
      ClientID::INTERFACE_ROUTE,
      {
          {{IPAddress("fc00:1::"), 64},
           RouteNextHopEntry(intfNhop1, AdminDistance::DIRECTLY_CONNECTED)},
          {{IPAddress("fc00:2::"), 64},
           RouteNextHopEntry(intfNhop2, AdminDistance::DIRECTLY_CONNECTED)},
      },
      {},
      false);

  // Intermediate route: fc00:10::/32 via fc00:1::10 and fc00:2::10
  RouteNextHopSet intermediateNhops;
  intermediateNhops.emplace(
      UnresolvedNextHop(IPAddress("fc00:1::10"), ECMP_WEIGHT));
  intermediateNhops.emplace(
      UnresolvedNextHop(IPAddress("fc00:2::10"), ECMP_WEIGHT));

  u.update<RibRouteUpdater::RouteEntry, folly::CIDRNetwork>(
      kClientA,
      {
          {{IPAddress("fc00:10::"), 32},
           RouteNextHopEntry(intermediateNhops, kDistance)},
      },
      {},
      false);

  // Final route: 2800:2::/64 via fc00:10::10 with cost, UCMP weight
  // Resolves recursively — the non-connected branch copies the intermediate
  // route's resolved next hops but applies cost from the original next hop
  RouteNextHopSet finalNhops;
  finalNhops.emplace(UnresolvedNextHop(
      IPAddress("fc00:10::10"),
      3,
      std::nullopt,
      std::nullopt,
      std::nullopt,
      std::nullopt,
      {},
      std::nullopt,
      std::nullopt,
      int64_t(777)));

  RouteV6::Prefix r1{IPAddressV6("2800:2::"), 64};

  u.update<RibRouteUpdater::RouteEntry, folly::CIDRNetwork>(
      kClientA,
      {
          {{r1.network(), r1.mask()}, RouteNextHopEntry(finalNhops, kDistance)},
      },
      {},
      false);

  auto it = v6Routes.exactMatch(r1.network(), r1.mask());
  ASSERT_NE(v6Routes.end(), it);
  auto route = it->value();
  EXPECT_TRUE(route->isResolved());

  const auto& resolvedNhops = route->getForwardInfo().getNextHopSet();
  ASSERT_EQ(resolvedNhops.size(), 2);

  for (const auto& nh : resolvedNhops) {
    EXPECT_TRUE(nh.isResolved());
    EXPECT_EQ(nh.cost(), int64_t(777));
  }
}

// Intermediate route's next hops carry cost, but the final (immediate) route's
// next hop has no cost. The final resolved next hops should have no cost
// because getFwdInfoFromNhop uses the caller's cost, not the intermediate's.
TEST(Route, resolveRecursiveEcmpIntermediateCostDropped) {
  IPv4NetworkToRouteMap v4Routes;
  IPv6NetworkToRouteMap v6Routes;

  RouteNextHopSet intfNhop1;
  intfNhop1.emplace(ResolvedNextHop(
      IPAddress("fc00:1::1"), InterfaceID(1), UCMP_DEFAULT_WEIGHT));
  RouteNextHopSet intfNhop2;
  intfNhop2.emplace(ResolvedNextHop(
      IPAddress("fc00:2::1"), InterfaceID(2), UCMP_DEFAULT_WEIGHT));

  NextHopIDManager nhopIds;
  RibRouteUpdater u(&v4Routes, &v6Routes, &nhopIds, nullptr);
  u.update<RibRouteUpdater::RouteEntry, folly::CIDRNetwork>(
      ClientID::INTERFACE_ROUTE,
      {
          {{IPAddress("fc00:1::"), 64},
           RouteNextHopEntry(intfNhop1, AdminDistance::DIRECTLY_CONNECTED)},
          {{IPAddress("fc00:2::"), 64},
           RouteNextHopEntry(intfNhop2, AdminDistance::DIRECTLY_CONNECTED)},
      },
      {},
      false);

  // Intermediate route with cost on its next hops
  RouteNextHopSet intermediateNhops;
  intermediateNhops.emplace(UnresolvedNextHop(
      IPAddress("fc00:1::10"),
      ECMP_WEIGHT,
      std::nullopt,
      std::nullopt,
      std::nullopt,
      std::nullopt,
      {},
      std::nullopt,
      std::nullopt,
      int64_t(333)));
  intermediateNhops.emplace(UnresolvedNextHop(
      IPAddress("fc00:2::10"),
      ECMP_WEIGHT,
      std::nullopt,
      std::nullopt,
      std::nullopt,
      std::nullopt,
      {},
      std::nullopt,
      std::nullopt,
      int64_t(444)));

  u.update<RibRouteUpdater::RouteEntry, folly::CIDRNetwork>(
      kClientA,
      {
          {{IPAddress("fc00:10::"), 32},
           RouteNextHopEntry(intermediateNhops, kDistance)},
      },
      {},
      false);

  // Verify intermediate route resolved with cost preserved
  auto intIt = v6Routes.exactMatch(IPAddressV6("fc00:10::"), 32);
  ASSERT_NE(v6Routes.end(), intIt);
  auto intRoute = intIt->value();
  EXPECT_TRUE(intRoute->isResolved());
  for (const auto& nh : intRoute->getForwardInfo().getNextHopSet()) {
    EXPECT_TRUE(nh.cost().has_value());
  }

  // Final (immediate) route with NO cost, resolves recursively
  RouteNextHopSet finalNhops;
  finalNhops.emplace(UnresolvedNextHop(IPAddress("fc00:10::10"), ECMP_WEIGHT));

  RouteV6::Prefix r1{IPAddressV6("2800:3::"), 64};

  u.update<RibRouteUpdater::RouteEntry, folly::CIDRNetwork>(
      kClientA,
      {
          {{r1.network(), r1.mask()}, RouteNextHopEntry(finalNhops, kDistance)},
      },
      {},
      false);

  auto it = v6Routes.exactMatch(r1.network(), r1.mask());
  ASSERT_NE(v6Routes.end(), it);
  auto route = it->value();
  EXPECT_TRUE(route->isResolved());

  const auto& resolvedNhops = route->getForwardInfo().getNextHopSet();
  ASSERT_EQ(resolvedNhops.size(), 2);

  // Intermediate costs (333, 444) are not inherited — the immediate next
  // hop's cost (nullopt) is what gets applied during recursive resolution
  for (const auto& nh : resolvedNhops) {
    EXPECT_TRUE(nh.isResolved());
    EXPECT_FALSE(nh.cost().has_value());
  }
}

TEST(Route, resolveRecursiveUcmpIntermediateCostDropped) {
  IPv4NetworkToRouteMap v4Routes;
  IPv6NetworkToRouteMap v6Routes;

  RouteNextHopSet intfNhop1;
  intfNhop1.emplace(ResolvedNextHop(
      IPAddress("fc00:1::1"), InterfaceID(1), UCMP_DEFAULT_WEIGHT));
  RouteNextHopSet intfNhop2;
  intfNhop2.emplace(ResolvedNextHop(
      IPAddress("fc00:2::1"), InterfaceID(2), UCMP_DEFAULT_WEIGHT));

  NextHopIDManager nhopIds;
  RibRouteUpdater u(&v4Routes, &v6Routes, &nhopIds, nullptr);
  u.update<RibRouteUpdater::RouteEntry, folly::CIDRNetwork>(
      ClientID::INTERFACE_ROUTE,
      {
          {{IPAddress("fc00:1::"), 64},
           RouteNextHopEntry(intfNhop1, AdminDistance::DIRECTLY_CONNECTED)},
          {{IPAddress("fc00:2::"), 64},
           RouteNextHopEntry(intfNhop2, AdminDistance::DIRECTLY_CONNECTED)},
      },
      {},
      false);

  // Intermediate route with cost on its next hops
  RouteNextHopSet intermediateNhops;
  intermediateNhops.emplace(UnresolvedNextHop(
      IPAddress("fc00:1::10"),
      ECMP_WEIGHT,
      std::nullopt,
      std::nullopt,
      std::nullopt,
      std::nullopt,
      {},
      std::nullopt,
      std::nullopt,
      int64_t(111)));
  intermediateNhops.emplace(UnresolvedNextHop(
      IPAddress("fc00:2::10"),
      ECMP_WEIGHT,
      std::nullopt,
      std::nullopt,
      std::nullopt,
      std::nullopt,
      {},
      std::nullopt,
      std::nullopt,
      int64_t(222)));

  u.update<RibRouteUpdater::RouteEntry, folly::CIDRNetwork>(
      kClientA,
      {
          {{IPAddress("fc00:10::"), 32},
           RouteNextHopEntry(intermediateNhops, kDistance)},
      },
      {},
      false);

  // Verify intermediate route resolved with cost preserved
  auto intIt = v6Routes.exactMatch(IPAddressV6("fc00:10::"), 32);
  ASSERT_NE(v6Routes.end(), intIt);
  auto intRoute = intIt->value();
  EXPECT_TRUE(intRoute->isResolved());
  for (const auto& nh : intRoute->getForwardInfo().getNextHopSet()) {
    EXPECT_TRUE(nh.cost().has_value());
  }

  // Final (immediate) route with NO cost, resolves recursively
  RouteNextHopSet finalNhops;
  finalNhops.emplace(UnresolvedNextHop(IPAddress("fc00:10::10"), 5));

  RouteV6::Prefix r1{IPAddressV6("2800:3::"), 64};

  u.update<RibRouteUpdater::RouteEntry, folly::CIDRNetwork>(
      kClientA,
      {
          {{r1.network(), r1.mask()}, RouteNextHopEntry(finalNhops, kDistance)},
      },
      {},
      false);

  auto it = v6Routes.exactMatch(r1.network(), r1.mask());
  ASSERT_NE(v6Routes.end(), it);
  auto route = it->value();
  EXPECT_TRUE(route->isResolved());

  const auto& resolvedNhops = route->getForwardInfo().getNextHopSet();
  ASSERT_EQ(resolvedNhops.size(), 2);

  // Intermediate costs (111, 222) are not inherited — the immediate next
  // hop's cost (nullopt) is what gets applied during recursive resolution
  for (const auto& nh : resolvedNhops) {
    EXPECT_TRUE(nh.isResolved());
    EXPECT_FALSE(nh.cost().has_value());
  }
}

TEST(Route, resolveRecursiveSrv6WithIntermediateLinkLocalCost) {
  IPv4NetworkToRouteMap v4Routes;
  IPv6NetworkToRouteMap v6Routes;

  const std::vector<folly::IPAddressV6> segListA{
      folly::IPAddressV6("2001:db8::1"), folly::IPAddressV6("2001:db8::2")};
  const std::vector<folly::IPAddressV6> segListB{
      folly::IPAddressV6("2001:db8::3"), folly::IPAddressV6("2001:db8::4")};

  NextHopIDManager nhopIds;
  RibRouteUpdater u(&v4Routes, &v6Routes, &nhopIds, nullptr);

  // Covering route for fdad:ff02:10b::d:0 with 2 link-local next hops
  // that carry cost
  RouteNextHopSet coverNhopsD;
  coverNhopsD.emplace(ResolvedNextHop(
      IPAddress("fe80::1"),
      InterfaceID(1),
      ECMP_WEIGHT,
      std::nullopt,
      std::nullopt,
      std::nullopt,
      std::nullopt,
      {},
      std::nullopt,
      std::nullopt,
      int64_t(100)));
  coverNhopsD.emplace(ResolvedNextHop(
      IPAddress("fe80::2"),
      InterfaceID(2),
      ECMP_WEIGHT,
      std::nullopt,
      std::nullopt,
      std::nullopt,
      std::nullopt,
      {},
      std::nullopt,
      std::nullopt,
      int64_t(200)));

  // Covering route for fdad:ff02:10b::c:0 with 2 link-local next hops
  // that carry cost
  RouteNextHopSet coverNhopsC;
  coverNhopsC.emplace(ResolvedNextHop(
      IPAddress("fe80::3"),
      InterfaceID(3),
      ECMP_WEIGHT,
      std::nullopt,
      std::nullopt,
      std::nullopt,
      std::nullopt,
      {},
      std::nullopt,
      std::nullopt,
      int64_t(300)));
  coverNhopsC.emplace(ResolvedNextHop(
      IPAddress("fe80::4"),
      InterfaceID(4),
      ECMP_WEIGHT,
      std::nullopt,
      std::nullopt,
      std::nullopt,
      std::nullopt,
      {},
      std::nullopt,
      std::nullopt,
      int64_t(400)));

  // OpenR routes (covering routes) with link-local nexthops carrying cost
  u.update<RibRouteUpdater::RouteEntry, folly::CIDRNetwork>(
      ClientID::OPENR,
      {
          {{IPAddress("fdad:ff02:10b::d:0"), 112},
           RouteNextHopEntry(coverNhopsD, AdminDistance::OPENR)},
          {{IPAddress("fdad:ff02:10b::c:0"), 112},
           RouteNextHopEntry(coverNhopsC, AdminDistance::OPENR)},
      },
      {},
      false);

  // Verify covering routes resolved with cost on their link-local next hops
  auto covDIt = v6Routes.exactMatch(IPAddressV6("fdad:ff02:10b::d:0"), 112);
  ASSERT_NE(v6Routes.end(), covDIt);
  EXPECT_TRUE(covDIt->value()->isResolved());
  for (const auto& nh : covDIt->value()->getForwardInfo().getNextHopSet()) {
    EXPECT_TRUE(nh.cost().has_value());
  }

  auto covCIt = v6Routes.exactMatch(IPAddressV6("fdad:ff02:10b::c:0"), 112);
  ASSERT_NE(v6Routes.end(), covCIt);
  EXPECT_TRUE(covCIt->value()->isResolved());
  for (const auto& nh : covCIt->value()->getForwardInfo().getNextHopSet()) {
    EXPECT_TRUE(nh.cost().has_value());
  }

  // BGP route 2001::/64 with 2 next hops, each with distinct SID lists
  // but no cost — resolves recursively over the OpenR covering routes
  RouteNextHopSet bgpNhops;
  bgpNhops.emplace(UnresolvedNextHop(
      IPAddress("fdad:ff02:10b::d:0"),
      ECMP_WEIGHT,
      std::nullopt,
      std::nullopt,
      std::nullopt,
      std::nullopt,
      segListA,
      TunnelType::SRV6_ENCAP,
      kSrv6Tunnel0));
  bgpNhops.emplace(UnresolvedNextHop(
      IPAddress("fdad:ff02:10b::c:0"),
      ECMP_WEIGHT,
      std::nullopt,
      std::nullopt,
      std::nullopt,
      std::nullopt,
      segListB,
      TunnelType::SRV6_ENCAP,
      kSrv6Tunnel0));

  RouteV6::Prefix bgpPrefix{IPAddressV6("2001::"), 64};

  u.update<RibRouteUpdater::RouteEntry, folly::CIDRNetwork>(
      ClientID::BGPD,
      {
          {{bgpPrefix.network(), bgpPrefix.mask()},
           RouteNextHopEntry(bgpNhops, AdminDistance::EBGP)},
      },
      {},
      false);

  auto it = v6Routes.exactMatch(bgpPrefix.network(), bgpPrefix.mask());
  ASSERT_NE(v6Routes.end(), it);
  auto route = it->value();
  EXPECT_TRUE(route->isResolved());

  const auto& resolvedNhops = route->getForwardInfo().getNextHopSet();
  ASSERT_EQ(resolvedNhops.size(), 4);

  int segListACount = 0;
  int segListBCount = 0;
  for (const auto& nh : resolvedNhops) {
    EXPECT_TRUE(nh.isResolved());
    EXPECT_FALSE(nh.cost().has_value());

    if (nh.srv6SegmentList() == segListA) {
      EXPECT_EQ(nh.tunnelType(), TunnelType::SRV6_ENCAP);
      EXPECT_EQ(nh.tunnelId(), kSrv6Tunnel0);
      EXPECT_TRUE(nh.intf() == InterfaceID(1) || nh.intf() == InterfaceID(2));
      segListACount++;
    } else if (nh.srv6SegmentList() == segListB) {
      EXPECT_EQ(nh.tunnelType(), TunnelType::SRV6_ENCAP);
      EXPECT_EQ(nh.tunnelId(), kSrv6Tunnel0);
      EXPECT_TRUE(nh.intf() == InterfaceID(3) || nh.intf() == InterfaceID(4));
      segListBCount++;
    } else {
      FAIL() << "Unexpected SID list on resolved next hop";
    }
  }
  EXPECT_EQ(segListACount, 2);
  EXPECT_EQ(segListBCount, 2);
}

TEST(Route, resolveRecursiveSrv6OpenrRouteChange) {
  IPv4NetworkToRouteMap v4Routes;
  IPv6NetworkToRouteMap v6Routes;

  const std::vector<folly::IPAddressV6> segListA{
      folly::IPAddressV6("2001:db8::1"), folly::IPAddressV6("2001:db8::2")};

  NextHopIDManager nhopIds;
  RibRouteUpdater u(&v4Routes, &v6Routes, &nhopIds, nullptr);

  // OpenR covering route with link-local nexthops on interfaces 1 and 2.
  RouteNextHopSet openrNhops{
      ResolvedNextHop(IPAddress("fe80::1"), InterfaceID(1), ECMP_WEIGHT),
      ResolvedNextHop(IPAddress("fe80::2"), InterfaceID(2), ECMP_WEIGHT)};

  u.update<RibRouteUpdater::RouteEntry, folly::CIDRNetwork>(
      ClientID::OPENR,
      {
          {{IPAddress("fdad:ff02:10b::d:0"), 112},
           RouteNextHopEntry(openrNhops, AdminDistance::OPENR)},
      },
      {},
      false);

  // BGP route resolving over the OpenR route with SRV6 fields.
  RouteNextHopSet bgpNhops{UnresolvedNextHop(
      IPAddress("fdad:ff02:10b::d:0"),
      ECMP_WEIGHT,
      std::nullopt,
      std::nullopt,
      std::nullopt,
      std::nullopt,
      segListA,
      TunnelType::SRV6_ENCAP,
      kSrv6Tunnel0)};

  RouteV6::Prefix bgpPrefix{IPAddressV6("2001::"), 64};
  u.update<RibRouteUpdater::RouteEntry, folly::CIDRNetwork>(
      ClientID::BGPD,
      {
          {{bgpPrefix.network(), bgpPrefix.mask()},
           RouteNextHopEntry(bgpNhops, AdminDistance::EBGP)},
      },
      {},
      false);

  // Verify initial resolution: 2 nexthops on interfaces 1 and 2.
  {
    auto it = v6Routes.exactMatch(bgpPrefix.network(), bgpPrefix.mask());
    ASSERT_NE(v6Routes.end(), it);
    EXPECT_TRUE(it->value()->isResolved());
    const auto& resolved = it->value()->getForwardInfo().getNextHopSet();
    ASSERT_EQ(resolved.size(), 2);
    std::set<InterfaceID> intfs;
    for (const auto& nh : resolved) {
      EXPECT_EQ(nh.srv6SegmentList(), segListA);
      EXPECT_EQ(nh.tunnelType(), TunnelType::SRV6_ENCAP);
      EXPECT_EQ(nh.tunnelId(), kSrv6Tunnel0);
      intfs.insert(nh.intf());
    }
    EXPECT_TRUE(intfs.count(InterfaceID(1)));
    EXPECT_TRUE(intfs.count(InterfaceID(2)));
  }

  // Update the OpenR route to use different link-local nexthops on
  // interfaces 3 and 4.
  RouteNextHopSet updatedOpenrNhops{
      ResolvedNextHop(IPAddress("fe80::3"), InterfaceID(3), ECMP_WEIGHT),
      ResolvedNextHop(IPAddress("fe80::4"), InterfaceID(4), ECMP_WEIGHT)};

  u.update<RibRouteUpdater::RouteEntry, folly::CIDRNetwork>(
      ClientID::OPENR,
      {
          {{IPAddress("fdad:ff02:10b::d:0"), 112},
           RouteNextHopEntry(updatedOpenrNhops, AdminDistance::OPENR)},
      },
      {},
      false);

  // Verify re-resolution: BGP route now resolves to fe80::3 and fe80::4
  // on interfaces 3 and 4, while retaining SRV6 fields from the BGP nhop.
  auto it = v6Routes.exactMatch(bgpPrefix.network(), bgpPrefix.mask());
  ASSERT_NE(v6Routes.end(), it);
  EXPECT_TRUE(it->value()->isResolved());
  const auto& resolvedNhops = it->value()->getForwardInfo().getNextHopSet();
  ASSERT_EQ(resolvedNhops.size(), 2);

  std::set<InterfaceID> intfs;
  for (const auto& nh : resolvedNhops) {
    EXPECT_TRUE(nh.isResolved());
    EXPECT_EQ(nh.srv6SegmentList(), segListA);
    EXPECT_EQ(nh.tunnelType(), TunnelType::SRV6_ENCAP);
    EXPECT_EQ(nh.tunnelId(), kSrv6Tunnel0);
    intfs.insert(nh.intf());
  }
  EXPECT_TRUE(intfs.count(InterfaceID(3)));
  EXPECT_TRUE(intfs.count(InterfaceID(4)));
}

// SRv6-over-SRv6 recursion: outer route R1 has no SID list and resolves over a
// single OpenR route R2 whose link-local next hops themselves carry SID lists.
// Documents current behavior: the inner SID lists are NOT inherited; the
// outer's empty SID list is stamped on the resolved leaves.
TEST(Route, resolveRecursiveSrv6InnerSidListThroughSingleOpenrRoute) {
  IPv4NetworkToRouteMap v4Routes;
  IPv6NetworkToRouteMap v6Routes;

  const std::vector<folly::IPAddressV6> sidList1{
      folly::IPAddressV6("fdad:ffff:0001:0002::")};
  const std::vector<folly::IPAddressV6> sidList2{
      folly::IPAddressV6("fdad:ffff:0003:0004::")};

  NextHopIDManager nhopIds;
  RibRouteUpdater u(&v4Routes, &v6Routes, &nhopIds, nullptr);

  // 1. OpenR route R2 with link-local next hops that carry their own SID lists.
  RouteNextHopSet openrNhops;
  openrNhops.emplace(ResolvedNextHop(
      IPAddress("fe80::1"),
      InterfaceID(1),
      ECMP_WEIGHT,
      std::nullopt,
      std::nullopt,
      std::nullopt,
      std::nullopt,
      sidList1,
      TunnelType::SRV6_ENCAP,
      kSrv6Tunnel0));
  openrNhops.emplace(ResolvedNextHop(
      IPAddress("fe80::2"),
      InterfaceID(2),
      ECMP_WEIGHT,
      std::nullopt,
      std::nullopt,
      std::nullopt,
      std::nullopt,
      sidList2,
      TunnelType::SRV6_ENCAP,
      kSrv6Tunnel0));

  u.update<RibRouteUpdater::RouteEntry, folly::CIDRNetwork>(
      ClientID::OPENR,
      {
          {{IPAddress("fdad:feff:0202::0:d:0"), 112},
           RouteNextHopEntry(openrNhops, AdminDistance::OPENR)},
      },
      {},
      false);

  // 2. BGP route R1 with a single plain next hop (no SID list) resolving
  //    recursively over R2.
  RouteNextHopSet bgpNhops{
      UnresolvedNextHop(IPAddress("fdad:feff:0202::0:d:0"), ECMP_WEIGHT)};

  RouteV6::Prefix bgpPrefix{IPAddressV6("2001::"), 64};
  u.update<RibRouteUpdater::RouteEntry, folly::CIDRNetwork>(
      ClientID::BGPD,
      {
          {{bgpPrefix.network(), bgpPrefix.mask()},
           RouteNextHopEntry(bgpNhops, AdminDistance::EBGP)},
      },
      {},
      false);

  // 3. R1 resolves to fe80::1/fe80::2 but the inner SID lists are dropped
  //    because the outer next hop carried none.
  auto it = v6Routes.exactMatch(bgpPrefix.network(), bgpPrefix.mask());
  ASSERT_NE(v6Routes.end(), it);
  EXPECT_TRUE(it->value()->isResolved());
  const auto& resolvedNhops = it->value()->getForwardInfo().getNextHopSet();
  ASSERT_EQ(resolvedNhops.size(), 2);

  std::set<InterfaceID> intfs;
  for (const auto& nh : resolvedNhops) {
    EXPECT_TRUE(nh.isResolved());
    EXPECT_TRUE(nh.srv6SegmentList().empty());
    EXPECT_FALSE(nh.tunnelType().has_value());
    EXPECT_FALSE(nh.tunnelId().has_value());
    intfs.insert(nh.intf());
  }
  EXPECT_TRUE(intfs.count(InterfaceID(1)));
  EXPECT_TRUE(intfs.count(InterfaceID(2)));
}

// SRv6-over-SRv6 recursion: outer route R1 carries sidList1 and resolves over a
// single OpenR route R2 whose link-local next hops carry their own SID lists.
// The outer SID list overrides the inner ones on every resolved leaf.
TEST(Route, resolveRecursiveSrv6OuterSidListThroughSingleOpenrRoute) {
  IPv4NetworkToRouteMap v4Routes;
  IPv6NetworkToRouteMap v6Routes;

  const std::vector<folly::IPAddressV6> sidList1{
      folly::IPAddressV6("fdad:ffff:0001:0002::")};
  const std::vector<folly::IPAddressV6> sidList2{
      folly::IPAddressV6("fdad:ffff:0003:0004::")};
  const std::vector<folly::IPAddressV6> sidList3{
      folly::IPAddressV6("fdad:ffff:0005:0006::")};

  NextHopIDManager nhopIds;
  RibRouteUpdater u(&v4Routes, &v6Routes, &nhopIds, nullptr);

  // 1. OpenR route R2 with link-local next hops carrying their own SID lists.
  RouteNextHopSet openrNhops;
  openrNhops.emplace(ResolvedNextHop(
      IPAddress("fe80::1"),
      InterfaceID(1),
      ECMP_WEIGHT,
      std::nullopt,
      std::nullopt,
      std::nullopt,
      std::nullopt,
      sidList2,
      TunnelType::SRV6_ENCAP,
      kSrv6Tunnel0));
  openrNhops.emplace(ResolvedNextHop(
      IPAddress("fe80::2"),
      InterfaceID(2),
      ECMP_WEIGHT,
      std::nullopt,
      std::nullopt,
      std::nullopt,
      std::nullopt,
      sidList3,
      TunnelType::SRV6_ENCAP,
      kSrv6Tunnel0));

  // 2. OpenR route R1 carrying sidList1, resolving recursively over R2.
  RouteNextHopSet r1Nhops{UnresolvedNextHop(
      IPAddress("fdad:feff:0202::0:d:0"),
      ECMP_WEIGHT,
      std::nullopt,
      std::nullopt,
      std::nullopt,
      std::nullopt,
      sidList1,
      TunnelType::SRV6_ENCAP,
      kSrv6Tunnel0)};

  RouteV6::Prefix r1Prefix{IPAddressV6("2001::"), 64};
  u.update<RibRouteUpdater::RouteEntry, folly::CIDRNetwork>(
      ClientID::OPENR,
      {
          {{IPAddress("fdad:feff:0202::0:d:0"), 112},
           RouteNextHopEntry(openrNhops, AdminDistance::OPENR)},
          {{r1Prefix.network(), r1Prefix.mask()},
           RouteNextHopEntry(r1Nhops, AdminDistance::OPENR)},
      },
      {},
      false);

  // 3. Both resolved leaves carry the outer sidList1, not the inner SID lists.
  auto it = v6Routes.exactMatch(r1Prefix.network(), r1Prefix.mask());
  ASSERT_NE(v6Routes.end(), it);
  EXPECT_TRUE(it->value()->isResolved());
  const auto& resolvedNhops = it->value()->getForwardInfo().getNextHopSet();
  ASSERT_EQ(resolvedNhops.size(), 2);

  std::set<InterfaceID> intfs;
  for (const auto& nh : resolvedNhops) {
    EXPECT_TRUE(nh.isResolved());
    EXPECT_EQ(nh.srv6SegmentList(), sidList1);
    EXPECT_EQ(nh.tunnelType(), TunnelType::SRV6_ENCAP);
    EXPECT_EQ(nh.tunnelId(), kSrv6Tunnel0);
    intfs.insert(nh.intf());
  }
  EXPECT_TRUE(intfs.count(InterfaceID(1)));
  EXPECT_TRUE(intfs.count(InterfaceID(2)));
}

// SRv6-over-SRv6 recursion: outer route R1 has no SID list and its two plain
// next hops resolve over two distinct OpenR routes R2/R3, each with a
// link-local next hop carrying its own SID list. Documents current behavior:
// inner SID lists are dropped (outer carried none).
TEST(Route, resolveRecursiveSrv6InnerSidListThroughTwoOpenrRoutes) {
  IPv4NetworkToRouteMap v4Routes;
  IPv6NetworkToRouteMap v6Routes;

  const std::vector<folly::IPAddressV6> sidList1{
      folly::IPAddressV6("fdad:ffff:0001:0002::")};
  const std::vector<folly::IPAddressV6> sidList2{
      folly::IPAddressV6("fdad:ffff:0003:0004::")};

  NextHopIDManager nhopIds;
  RibRouteUpdater u(&v4Routes, &v6Routes, &nhopIds, nullptr);

  // 1. OpenR routes R2 (d:0) and R3 (c:0), each with one link-local next hop
  //    carrying its own SID list.
  RouteNextHopSet openrNhopsD{ResolvedNextHop(
      IPAddress("fe80::1"),
      InterfaceID(1),
      ECMP_WEIGHT,
      std::nullopt,
      std::nullopt,
      std::nullopt,
      std::nullopt,
      sidList1,
      TunnelType::SRV6_ENCAP,
      kSrv6Tunnel0)};
  RouteNextHopSet openrNhopsC{ResolvedNextHop(
      IPAddress("fe80::2"),
      InterfaceID(2),
      ECMP_WEIGHT,
      std::nullopt,
      std::nullopt,
      std::nullopt,
      std::nullopt,
      sidList2,
      TunnelType::SRV6_ENCAP,
      kSrv6Tunnel0)};

  u.update<RibRouteUpdater::RouteEntry, folly::CIDRNetwork>(
      ClientID::OPENR,
      {
          {{IPAddress("fdad:feff:0202::0:d:0"), 112},
           RouteNextHopEntry(openrNhopsD, AdminDistance::OPENR)},
          {{IPAddress("fdad:feff:0202::0:c:0"), 112},
           RouteNextHopEntry(openrNhopsC, AdminDistance::OPENR)},
      },
      {},
      false);

  // 2. BGP route R1 with two plain next hops (no SID list), one resolving over
  //    R2 and the other over R3.
  RouteNextHopSet bgpNhops{
      UnresolvedNextHop(IPAddress("fdad:feff:0202::0:d:0"), ECMP_WEIGHT),
      UnresolvedNextHop(IPAddress("fdad:feff:0202::0:c:0"), ECMP_WEIGHT)};

  RouteV6::Prefix bgpPrefix{IPAddressV6("2001::"), 64};
  u.update<RibRouteUpdater::RouteEntry, folly::CIDRNetwork>(
      ClientID::BGPD,
      {
          {{bgpPrefix.network(), bgpPrefix.mask()},
           RouteNextHopEntry(bgpNhops, AdminDistance::EBGP)},
      },
      {},
      false);

  // 3. R1 resolves to fe80::1/fe80::2 with empty SID lists.
  auto it = v6Routes.exactMatch(bgpPrefix.network(), bgpPrefix.mask());
  ASSERT_NE(v6Routes.end(), it);
  EXPECT_TRUE(it->value()->isResolved());
  const auto& resolvedNhops = it->value()->getForwardInfo().getNextHopSet();
  ASSERT_EQ(resolvedNhops.size(), 2);

  std::set<InterfaceID> intfs;
  for (const auto& nh : resolvedNhops) {
    EXPECT_TRUE(nh.isResolved());
    EXPECT_TRUE(nh.srv6SegmentList().empty());
    EXPECT_FALSE(nh.tunnelType().has_value());
    EXPECT_FALSE(nh.tunnelId().has_value());
    intfs.insert(nh.intf());
  }
  EXPECT_TRUE(intfs.count(InterfaceID(1)));
  EXPECT_TRUE(intfs.count(InterfaceID(2)));
}

// SRv6-over-SRv6 recursion: outer route R1 carries sidList1 on both next hops,
// which resolve over two distinct OpenR routes R2/R3 whose link-local next hops
// carry their own SID lists. The outer sidList1 overrides the inner ones.
TEST(Route, resolveRecursiveSrv6OuterSidListThroughTwoOpenrRoutes) {
  IPv4NetworkToRouteMap v4Routes;
  IPv6NetworkToRouteMap v6Routes;

  const std::vector<folly::IPAddressV6> sidList1{
      folly::IPAddressV6("fdad:ffff:0001:0002::")};
  const std::vector<folly::IPAddressV6> sidList2{
      folly::IPAddressV6("fdad:ffff:0003:0004::")};
  const std::vector<folly::IPAddressV6> sidList3{
      folly::IPAddressV6("fdad:ffff:0005:0006::")};

  NextHopIDManager nhopIds;
  RibRouteUpdater u(&v4Routes, &v6Routes, &nhopIds, nullptr);

  // 1. OpenR routes R2 (d:0) and R3 (c:0), each with one link-local next hop
  //    carrying its own SID list.
  RouteNextHopSet openrNhopsD{ResolvedNextHop(
      IPAddress("fe80::1"),
      InterfaceID(1),
      ECMP_WEIGHT,
      std::nullopt,
      std::nullopt,
      std::nullopt,
      std::nullopt,
      sidList2,
      TunnelType::SRV6_ENCAP,
      kSrv6Tunnel0)};
  RouteNextHopSet openrNhopsC{ResolvedNextHop(
      IPAddress("fe80::2"),
      InterfaceID(2),
      ECMP_WEIGHT,
      std::nullopt,
      std::nullopt,
      std::nullopt,
      std::nullopt,
      sidList3,
      TunnelType::SRV6_ENCAP,
      kSrv6Tunnel0)};

  // 2. OpenR route R1 carrying sidList1 on both next hops, resolving over
  // R2/R3.
  RouteNextHopSet r1Nhops{
      UnresolvedNextHop(
          IPAddress("fdad:feff:0202::0:d:0"),
          ECMP_WEIGHT,
          std::nullopt,
          std::nullopt,
          std::nullopt,
          std::nullopt,
          sidList1,
          TunnelType::SRV6_ENCAP,
          kSrv6Tunnel0),
      UnresolvedNextHop(
          IPAddress("fdad:feff:0202::0:c:0"),
          ECMP_WEIGHT,
          std::nullopt,
          std::nullopt,
          std::nullopt,
          std::nullopt,
          sidList1,
          TunnelType::SRV6_ENCAP,
          kSrv6Tunnel0)};

  RouteV6::Prefix r1Prefix{IPAddressV6("2001::"), 64};
  u.update<RibRouteUpdater::RouteEntry, folly::CIDRNetwork>(
      ClientID::OPENR,
      {
          {{IPAddress("fdad:feff:0202::0:d:0"), 112},
           RouteNextHopEntry(openrNhopsD, AdminDistance::OPENR)},
          {{IPAddress("fdad:feff:0202::0:c:0"), 112},
           RouteNextHopEntry(openrNhopsC, AdminDistance::OPENR)},
          {{r1Prefix.network(), r1Prefix.mask()},
           RouteNextHopEntry(r1Nhops, AdminDistance::OPENR)},
      },
      {},
      false);

  // 3. Both resolved leaves carry the outer sidList1, not the inner SID lists.
  auto it = v6Routes.exactMatch(r1Prefix.network(), r1Prefix.mask());
  ASSERT_NE(v6Routes.end(), it);
  EXPECT_TRUE(it->value()->isResolved());
  const auto& resolvedNhops = it->value()->getForwardInfo().getNextHopSet();
  ASSERT_EQ(resolvedNhops.size(), 2);

  std::set<InterfaceID> intfs;
  for (const auto& nh : resolvedNhops) {
    EXPECT_TRUE(nh.isResolved());
    EXPECT_EQ(nh.srv6SegmentList(), sidList1);
    EXPECT_EQ(nh.tunnelType(), TunnelType::SRV6_ENCAP);
    EXPECT_EQ(nh.tunnelId(), kSrv6Tunnel0);
    intfs.insert(nh.intf());
  }
  EXPECT_TRUE(intfs.count(InterfaceID(1)));
  EXPECT_TRUE(intfs.count(InterfaceID(2)));
}

// Same-prefix client preference: an OpenR route (no SID lists) and a TE_Agent
// route (with SID lists) share a prefix. The TE_Agent route has the lower admin
// distance, so it wins best-entry selection and its SID lists are programmed.
TEST(Route, srv6TeAgentRoutePreferredOverOpenrByAdminDistance) {
  IPv4NetworkToRouteMap v4Routes;
  IPv6NetworkToRouteMap v6Routes;

  const std::vector<folly::IPAddressV6> sidList1{
      folly::IPAddressV6("fdad:ffff:0001:0002::")};
  const std::vector<folly::IPAddressV6> sidList2{
      folly::IPAddressV6("fdad:ffff:0003:0004::")};

  NextHopIDManager nhopIds;
  RibRouteUpdater u(&v4Routes, &v6Routes, &nhopIds, nullptr);

  RouteV6::Prefix prefix{IPAddressV6("2001::"), 64};

  // 1. OpenR route for the prefix with two link-local next hops, no SID lists.
  RouteNextHopSet openrNhops{
      ResolvedNextHop(IPAddress("fe80::1"), InterfaceID(1), ECMP_WEIGHT),
      ResolvedNextHop(IPAddress("fe80::2"), InterfaceID(2), ECMP_WEIGHT)};
  u.update<RibRouteUpdater::RouteEntry, folly::CIDRNetwork>(
      ClientID::OPENR,
      {
          {{prefix.network(), prefix.mask()},
           RouteNextHopEntry(openrNhops, AdminDistance::OPENR)},
      },
      {},
      false);

  // 2. TE_Agent route for the SAME prefix with the same next hops, but each
  //    carrying its own SID list. Lower admin distance than OpenR.
  RouteNextHopSet teNhops;
  teNhops.emplace(ResolvedNextHop(
      IPAddress("fe80::1"),
      InterfaceID(1),
      ECMP_WEIGHT,
      std::nullopt,
      std::nullopt,
      std::nullopt,
      std::nullopt,
      sidList1,
      TunnelType::SRV6_ENCAP,
      kSrv6Tunnel0));
  teNhops.emplace(ResolvedNextHop(
      IPAddress("fe80::2"),
      InterfaceID(2),
      ECMP_WEIGHT,
      std::nullopt,
      std::nullopt,
      std::nullopt,
      std::nullopt,
      sidList2,
      TunnelType::SRV6_ENCAP,
      kSrv6Tunnel0));
  u.update<RibRouteUpdater::RouteEntry, folly::CIDRNetwork>(
      ClientID::TE_AGENT,
      {
          {{prefix.network(), prefix.mask()},
           RouteNextHopEntry(teNhops, AdminDistance::TE_AGENT)},
      },
      {},
      false);

  // 3. TE_Agent entry wins (admin distance 2 < OpenR's 10), so the resolved
  //    next hops carry the TE_Agent SID lists.
  auto it = v6Routes.exactMatch(prefix.network(), prefix.mask());
  ASSERT_NE(v6Routes.end(), it);
  auto route = it->value();
  EXPECT_TRUE(route->isResolved());
  EXPECT_EQ(route->getBestEntry().first, ClientID::TE_AGENT);
  EXPECT_EQ(
      route->getForwardInfo().getAdminDistance(), AdminDistance::TE_AGENT);

  const auto& resolvedNhops = route->getForwardInfo().getNextHopSet();
  ASSERT_EQ(resolvedNhops.size(), 2);
  for (const auto& nh : resolvedNhops) {
    EXPECT_TRUE(nh.isResolved());
    EXPECT_EQ(nh.tunnelType(), TunnelType::SRV6_ENCAP);
    if (nh.intf() == InterfaceID(1)) {
      EXPECT_EQ(nh.srv6SegmentList(), sidList1);
      EXPECT_EQ(nh.tunnelId(), kSrv6Tunnel0);
    } else if (nh.intf() == InterfaceID(2)) {
      EXPECT_EQ(nh.srv6SegmentList(), sidList2);
      EXPECT_EQ(nh.tunnelId(), kSrv6Tunnel0);
    } else {
      FAIL() << "Unexpected interface on resolved next hop";
    }
  }
}

TEST(RibRouteTables, getVrfList) {
  RoutingInformationBase rib;

  // Initially no VRFs
  EXPECT_TRUE(rib.getVrfList().empty());

  // Add a single VRF
  rib.ensureVrf(RouterID(0));
  auto vrfList = rib.getVrfList();
  EXPECT_EQ(vrfList.size(), 1);
  EXPECT_EQ(vrfList[0], RouterID(0));

  // Add more VRFs
  rib.ensureVrf(RouterID(1));
  rib.ensureVrf(RouterID(2));
  vrfList = rib.getVrfList();
  EXPECT_EQ(vrfList.size(), 3);

  // Verify all VRFs are present
  std::set<RouterID> vrfSet(vrfList.begin(), vrfList.end());
  EXPECT_EQ(vrfSet.count(RouterID(0)), 1);
  EXPECT_EQ(vrfSet.count(RouterID(1)), 1);
  EXPECT_EQ(vrfSet.count(RouterID(2)), 1);

  // Ensure duplicate VRF does not add a new entry
  rib.ensureVrf(RouterID(1));
  vrfList = rib.getVrfList();
  EXPECT_EQ(vrfList.size(), 3);
}

TEST(Route, cycleDetectionPopulatesUpdateStatistics) {
  RoutingInformationBase rib;
  rib.ensureVrf(RouterID(0));

  // One v4 cycle: 10.0.0.0/24 -> 20.0.0.1, 20.0.0.0/24 -> 10.0.0.1
  // One v6 cycle: 2001::/64  -> 3001::1,  3001::/64  -> 2001::1
  auto stats = rib.update(
      nullptr,
      RouterID(0),
      ClientID::BGPD,
      AdminDistance::EBGP,
      {
          makeUnicastRoute(
              {IPAddress("10.0.0.0"), 24}, {IPAddress("20.0.0.1")}),
          makeUnicastRoute(
              {IPAddress("20.0.0.0"), 24}, {IPAddress("10.0.0.1")}),
          makeUnicastRoute({IPAddress("2001::"), 64}, {IPAddress("3001::1")}),
          makeUnicastRoute({IPAddress("3001::"), 64}, {IPAddress("2001::1")}),
      },
      {},
      false,
      "cycle detection test",
      noopFibUpdate,
      nullptr);

  EXPECT_EQ(stats.resolutionCyclesDetected, 2u);
}

TEST(Route, noCycleDetectedForNonCyclicRoutes) {
  RoutingInformationBase rib;
  rib.ensureVrf(RouterID(0));

  // Non-cyclic routes that still exercise the recursive resolution path:
  //   v4: 10.0.0.0/24 -> 1.1.1.5  (resolves via 1.1.1.0/24)
  //       1.1.1.0/24  -> 2.2.2.1  (leaf with no covering route)
  //   v6: 2001::/64   -> 3001::5  (resolves via 3001::/64)
  //       3001::/64   -> 4001::1  (leaf with no covering route)
  // No nexthop chain loops back, so the cycle counter must stay 0.
  auto stats = rib.update(
      nullptr,
      RouterID(0),
      ClientID::BGPD,
      AdminDistance::EBGP,
      {
          makeUnicastRoute({IPAddress("10.0.0.0"), 24}, {IPAddress("1.1.1.5")}),
          makeUnicastRoute({IPAddress("1.1.1.0"), 24}, {IPAddress("2.2.2.1")}),
          makeUnicastRoute({IPAddress("2001::"), 64}, {IPAddress("3001::5")}),
          makeUnicastRoute({IPAddress("3001::"), 64}, {IPAddress("4001::1")}),
      },
      {},
      false,
      "no cycle test",
      noopFibUpdate,
      nullptr);

  EXPECT_EQ(stats.resolutionCyclesDetected, 0u);
}

// Verify that the RouteUpdater release calls (D5) drop the manager refcount
// when a route carrying a clientNextHopSetID is deleted.
TEST(Route, clientIdReleasedOnDelete) {
  IPv4NetworkToRouteMap v4Routes;
  IPv6NetworkToRouteMap v6Routes;
  NextHopIDManager nhopIds;

  RouteNextHopSet nhop = makeNextHops({"1.1.1.10"});
  // Pre-allocate ID for the set (refcount 0 -> 1).
  auto allocResult = nhopIds.getOrAllocRouteNextHopSetID(nhop);
  auto setId = allocResult.nextHopIdSetIter->second.id;
  EXPECT_TRUE(nhopIds.getNextHopsIf(setId).has_value());

  // Build entry carrying the pre-allocated ID via the new constructor arg.
  RouteNextHopEntry entry(
      nhop,
      kDistance,
      std::optional<RouteCounterID>(std::nullopt),
      std::optional<cfg::AclLookupClass>(std::nullopt),
      std::optional<cfg::SwitchingMode>(std::nullopt),
      std::optional<RouteNextHopEntry::NextHopSet>(std::nullopt),
      std::optional<NextHopSetID>(std::nullopt),
      std::optional<NextHopSetID>(std::nullopt),
      std::optional<NextHopSetID>(setId));

  RouteV4::Prefix prefix{IPAddressV4("10.1.1.0"), 24};

  RibRouteUpdater u(&v4Routes, &v6Routes, &nhopIds, nullptr);
  u.update<RibRouteUpdater::RouteEntry, folly::CIDRNetwork>(
      kClientA, {{{prefix.network(), prefix.mask()}, entry}}, {}, false);
  // No allocation happened on insert (D8 not yet live); set still tracked.
  EXPECT_TRUE(nhopIds.getNextHopsIf(setId).has_value());

  // Delete via toDel — exercises delRouteImpl's new release path.
  u.update<RibRouteUpdater::RouteEntry, folly::CIDRNetwork>(
      kClientA, {}, {{prefix.network(), prefix.mask()}}, false);
  // Refcount went 1 -> 0; set is freed.
  EXPECT_FALSE(nhopIds.getNextHopsIf(setId).has_value());
}

// Verify removeAllRoutesFromClient (resetClientsRoutes=true) also releases.
TEST(Route, clientIdReleasedOnResetAllRoutes) {
  IPv4NetworkToRouteMap v4Routes;
  IPv6NetworkToRouteMap v6Routes;
  NextHopIDManager nhopIds;

  RouteNextHopSet nhop = makeNextHops({"2.2.2.20"});
  auto allocResult2 = nhopIds.getOrAllocRouteNextHopSetID(nhop);
  auto setId2 = allocResult2.nextHopIdSetIter->second.id;
  EXPECT_TRUE(nhopIds.getNextHopsIf(setId2).has_value());

  RouteNextHopEntry entry(
      nhop,
      kDistance,
      std::optional<RouteCounterID>(std::nullopt),
      std::optional<cfg::AclLookupClass>(std::nullopt),
      std::optional<cfg::SwitchingMode>(std::nullopt),
      std::optional<RouteNextHopEntry::NextHopSet>(std::nullopt),
      std::optional<NextHopSetID>(std::nullopt),
      std::optional<NextHopSetID>(std::nullopt),
      std::optional<NextHopSetID>(setId2));

  RouteV4::Prefix prefix{IPAddressV4("30.1.1.0"), 24};

  RibRouteUpdater u(&v4Routes, &v6Routes, &nhopIds, nullptr);
  u.update<RibRouteUpdater::RouteEntry, folly::CIDRNetwork>(
      kClientA, {{{prefix.network(), prefix.mask()}, entry}}, {}, false);
  EXPECT_TRUE(nhopIds.getNextHopsIf(setId2).has_value());

  // resetClientsRoutes=true triggers removeAllRoutesFromClientImpl.
  u.update<RibRouteUpdater::RouteEntry, folly::CIDRNetwork>(
      kClientA, {}, {}, true);
  EXPECT_FALSE(nhopIds.getNextHopsIf(setId2).has_value());
}

} // namespace facebook::fboss
