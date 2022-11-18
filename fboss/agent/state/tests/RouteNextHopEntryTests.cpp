/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "common/network/if/gen-cpp2/Address_types.h"
#include "fboss/agent/AddressUtil.h"
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/if/gen-cpp2/ctrl_types.h"
#include "fboss/agent/state/RouteNextHopEntry.h"
#include "fboss/agent/test/TestUtils.h"

#include <folly/IPAddress.h>
#include <gtest/gtest.h>
#include <algorithm>
#include <numeric>
#include <vector>

using namespace facebook::fboss;

using facebook::fboss::AdminDistance;
using facebook::fboss::ECMP_WEIGHT;
using facebook::fboss::InterfaceID;
using facebook::fboss::IpPrefix;
using facebook::fboss::MplsAction;
using facebook::fboss::NextHopThrift;
using facebook::fboss::UnicastRoute;

DECLARE_bool(wide_ecmp);

namespace {

const AdminDistance kDefaultAdminDistance = AdminDistance::EBGP;

const IpPrefix kDestPrefix = IpPrefix(
    apache::thrift::FRAGILE,
    facebook::network::toBinaryAddress(folly::IPAddress("fc00::")),
    7);

const folly::IPAddress nextHopAddr1 = folly::IPAddress("fe80::1");
const folly::IPAddress nextHopAddr2 =
    folly::IPAddress("2401:db00:e112:9103:1028::1b");
const folly::IPAddress nextHopAddr3 =
    folly::IPAddress("2001:db8:ffff:ffff:ffff:ffff:ffff:ffff");
const folly::IPAddress nextHopAddr4 =
    folly::IPAddress("2401:db00:e113:9103:1028::1b");
const folly::IPAddress nextHopAddr5 =
    folly::IPAddress("2401:db00:e114:9103:1028::1b");
const folly::IPAddress nextHopAddr6 =
    folly::IPAddress("2401:db00:e115:9103:1028::1b");
const folly::IPAddress nextHopAddr7 =
    folly::IPAddress("2401:db00:e116:9103:1028::1b");
const folly::IPAddress nextHopAddr8 =
    folly::IPAddress("2401:db00:e117:9103:1028::1b");

// These next-hops are in our internal representation.
// Unlike other variables, it's not const so that it can be sorted
// in unit tests.
std::vector<NextHop> nextHops = {
    ResolvedNextHop(nextHopAddr1, InterfaceID(1), ECMP_WEIGHT),
    UnresolvedNextHop(nextHopAddr2, ECMP_WEIGHT),
    UnresolvedNextHop(nextHopAddr3, ECMP_WEIGHT)};

// Any NextHopThrift received via Thrift is considered unresolved, with the
// sole exception of IPv6 link-local next-hops. For an IPv6 link-local
// next-hop to be considered resolved _prior_ to route resolution, it must
// have the field "3: optional string ifName" filled out in the BinaryAddress.
facebook::network::thrift::BinaryAddress createV6LinkLocalNextHop(
    const folly::IPAddress& linkLocalAddr) {
  CHECK(linkLocalAddr.isV6());

  auto linkLocalAddrAsBinaryAddress =
      facebook::network::toBinaryAddress(linkLocalAddr);

  linkLocalAddrAsBinaryAddress.ifName() = "fboss1";

  return linkLocalAddrAsBinaryAddress;
}

// These are the Thrift representations of nextHopAddr{1,2,3}.
std::vector<NextHopThrift> nextHopsThrift() {
  std::vector<NextHopThrift> nexthops;
  std::vector<folly::IPAddress> addrs{nextHopAddr1, nextHopAddr2, nextHopAddr3};
  for (const auto& addr : addrs) {
    NextHopThrift nexthop;
    *nexthop.address() = createV6LinkLocalNextHop(addr);
    *nexthop.weight() = static_cast<int32_t>(ECMP_WEIGHT);
    nexthops.emplace_back(std::move(nexthop));
  }
  return nexthops;
}

// These are the _deprecated_ Thrift representations of nextHopAddr{1,2,3}.
const std::vector<facebook::network::thrift::BinaryAddress>
    nextHopsBinaryAddress = {
        createV6LinkLocalNextHop(nextHopAddr1),
        facebook::network::toBinaryAddress(nextHopAddr2),
        facebook::network::toBinaryAddress(nextHopAddr3),
};

} // namespace

TEST(RouteNextHopEntry, FromNextHopsThrift) {
  // Note that we can't use UnicastRoute's constructor because it expects to be
  // passed both nextHopAddrs and nextHops
  UnicastRoute route;
  route.dest() = kDestPrefix;
  route.nextHops() = nextHopsThrift();
  std::optional<RouteCounterID> counterID("route.counter.0");
  std::optional<cfg::AclLookupClass> classID(
      cfg::AclLookupClass::DST_CLASS_L3_DPR);

  auto nextHopEntry =
      RouteNextHopEntry::from(route, kDefaultAdminDistance, counterID, classID);

  ASSERT_EQ(nextHopEntry.getAction(), RouteForwardAction::NEXTHOPS);
  ASSERT_EQ(nextHopEntry.getAdminDistance(), kDefaultAdminDistance);
  ASSERT_EQ(nextHopEntry.getCounterID(), counterID);
  ASSERT_EQ(nextHopEntry.getClassID(), classID);

  std::sort(nextHops.begin(), nextHops.end());
  auto nextHopEntryNextHops = nextHopEntry.getNextHopSet();
  ASSERT_TRUE(std::equal(
      nextHopEntryNextHops.begin(),
      nextHopEntryNextHops.end(),
      nextHops.begin()));
}

// The UnicastRoute.nextHopAddrs field has been deprecated in favor of
// UnicastRoutes.nextHops, which equips each next-hop with a weight
// in support of UCMP.
TEST(RouteNextHopEntry, FromBinaryAddresses) {
  // Note that we can't use UnicastRoute's constructor because it expects to be
  // passed both nextHopAddrs and nextHops
  UnicastRoute route;
  route.dest() = kDestPrefix;
  route.nextHopAddrs() = nextHopsBinaryAddress;
  std::optional<RouteCounterID> counterID("route.counter.0");
  std::optional<cfg::AclLookupClass> classID(
      cfg::AclLookupClass::DST_CLASS_L3_DPR);

  auto nextHopEntry =
      RouteNextHopEntry::from(route, kDefaultAdminDistance, counterID, classID);

  ASSERT_EQ(nextHopEntry.getAction(), RouteForwardAction::NEXTHOPS);
  ASSERT_EQ(nextHopEntry.getAdminDistance(), kDefaultAdminDistance);
  ASSERT_EQ(nextHopEntry.getCounterID(), counterID);
  ASSERT_EQ(nextHopEntry.getClassID(), classID);

  std::sort(nextHops.begin(), nextHops.end());
  auto nextHopEntryNextHops = nextHopEntry.getNextHopSet();
  ASSERT_TRUE(std::equal(
      nextHopEntryNextHops.begin(),
      nextHopEntryNextHops.end(),
      nextHops.begin()));
}

TEST(RouteNextHopEntry, OverrideDefaultAdminDistance) {
  UnicastRoute route;
  route.dest() = kDestPrefix;
  route.nextHops() = nextHopsThrift();
  route.adminDistance() = AdminDistance::IBGP;

  auto nextHopEntry = RouteNextHopEntry::from(
      route, kDefaultAdminDistance, std::nullopt, std::nullopt);

  ASSERT_EQ(nextHopEntry.getAdminDistance(), AdminDistance::IBGP);
}

TEST(RouteNextHopEntry, EmptyListIsDrop) {
  std::vector<NextHopThrift> noNextHops = {};

  // Note that we can't use UnicastRoute's constructor because it expects to be
  // passed both nextHopAddrs and nextHops
  UnicastRoute route;
  route.dest() = kDestPrefix;
  route.nextHops() = noNextHops;

  auto nextHopEntry = RouteNextHopEntry::from(
      route, kDefaultAdminDistance, std::nullopt, std::nullopt);

  ASSERT_EQ(nextHopEntry.getAction(), RouteForwardAction::DROP);
  ASSERT_EQ(nextHopEntry.getAdminDistance(), kDefaultAdminDistance);
  ASSERT_EQ(nextHopEntry.getNextHopSet().size(), 0);
}

// Total weight greater than 128. Should get normalized to 512
TEST(RouteNextHopEntry, NormalizedFixedSizeWideNextHop) {
  RouteNextHopSet nhops;

  FLAGS_ecmp_width = 512;
  FLAGS_wide_ecmp = true;

  for (const auto ucmpOptimized : {false, true}) {
    FLAGS_optimized_ucmp = ucmpOptimized;
    nhops.emplace(ResolvedNextHop(nextHopAddr1, InterfaceID(1), 55));
    nhops.emplace(ResolvedNextHop(nextHopAddr2, InterfaceID(2), 56));
    nhops.emplace(ResolvedNextHop(nextHopAddr3, InterfaceID(3), 57));

    auto normalizedNextHops =
        RouteNextHopEntry(nhops, kDefaultAdminDistance).normalizedNextHops();

    NextHopWeight totalWeight = std::accumulate(
        normalizedNextHops.begin(),
        normalizedNextHops.end(),
        0,
        [](NextHopWeight w, const NextHop& nh) { return w + nh.weight(); });
    EXPECT_EQ(totalWeight, FLAGS_ecmp_width);
  }
}

// Total weight greater than ecmp_width but can be reduced
// by proportionally reducing weights first and then fitting to 512
TEST(RouteNextHopEntry, ScaledDownNormalizedFixedSizeWideNextHop) {
  RouteNextHopSet nhops;

  FLAGS_ecmp_width = 512;
  FLAGS_wide_ecmp = true;

  for (const auto ucmpOptimized : {false, true}) {
    FLAGS_optimized_ucmp = ucmpOptimized;
    nhops.emplace(ResolvedNextHop(nextHopAddr1, InterfaceID(1), 550));
    nhops.emplace(ResolvedNextHop(nextHopAddr2, InterfaceID(2), 560));
    nhops.emplace(ResolvedNextHop(nextHopAddr3, InterfaceID(3), 570));

    auto normalizedNextHops =
        RouteNextHopEntry(nhops, kDefaultAdminDistance).normalizedNextHops();

    NextHopWeight totalWeight = std::accumulate(
        normalizedNextHops.begin(),
        normalizedNextHops.end(),
        0,
        [](NextHopWeight w, const NextHop& nh) { return w + nh.weight(); });
    EXPECT_EQ(totalWeight, FLAGS_ecmp_width);
  }
}

// Total width exceeds 512 after proportionaly reducing weights.
// should get normalized by reducing the high weight members
TEST(RouteNextHopEntry, ScaledDownFixedSizeWideNextHop) {
  RouteNextHopSet nhops;

  FLAGS_ecmp_width = 512;
  FLAGS_wide_ecmp = true;

  for (const auto ucmpOptimized : {false, true}) {
    FLAGS_optimized_ucmp = ucmpOptimized;
    nhops.emplace(ResolvedNextHop(nextHopAddr1, InterfaceID(1), 1));
    nhops.emplace(ResolvedNextHop(nextHopAddr2, InterfaceID(2), 1));
    nhops.emplace(ResolvedNextHop(nextHopAddr3, InterfaceID(3), 25235));

    auto normalizedNextHops =
        RouteNextHopEntry(nhops, kDefaultAdminDistance).normalizedNextHops();

    NextHopWeight totalWeight = std::accumulate(
        normalizedNextHops.begin(),
        normalizedNextHops.end(),
        0,
        [](NextHopWeight w, const NextHop& nh) { return w + nh.weight(); });
    // With optimized ucmp, the lowe weight paths should get removed
    EXPECT_EQ(totalWeight, ucmpOptimized ? 1 : FLAGS_ecmp_width);
  }
}

TEST(RouteNextHopEntry, NotNormalizedWideECmpEnabled) {
  RouteNextHopSet nhops;

  FLAGS_ecmp_width = 512;
  FLAGS_wide_ecmp = true;

  for (const auto ucmpOptimized : {false, true}) {
    FLAGS_optimized_ucmp = ucmpOptimized;
    nhops.emplace(ResolvedNextHop(nextHopAddr1, InterfaceID(1), 32));
    nhops.emplace(ResolvedNextHop(nextHopAddr2, InterfaceID(2), 33));
    nhops.emplace(ResolvedNextHop(nextHopAddr3, InterfaceID(3), 34));

    auto normalizedNextHops =
        RouteNextHopEntry(nhops, kDefaultAdminDistance).normalizedNextHops();

    NextHopWeight totalWeight = std::accumulate(
        normalizedNextHops.begin(),
        normalizedNextHops.end(),
        0,
        [](NextHopWeight w, const NextHop& nh) { return w + nh.weight(); });

    auto originalTotalWeight = std::accumulate(
        nhops.begin(), nhops.end(), 0, [](NextHopWeight w, const NextHop& nh) {
          return w + nh.weight();
        });

    EXPECT_EQ(totalWeight, originalTotalWeight);
  }
}

// optimized algorithm should remove the low weight and normalize remaining
TEST(RouteNextHopEntry, OptimizedUcmpDiscardLowWeights) {
  RouteNextHopSet nhops;

  FLAGS_ecmp_width = 64;
  FLAGS_optimized_ucmp = true;

  nhops.emplace(ResolvedNextHop(nextHopAddr1, InterfaceID(1), 1));
  nhops.emplace(ResolvedNextHop(nextHopAddr2, InterfaceID(2), 36));
  nhops.emplace(ResolvedNextHop(nextHopAddr3, InterfaceID(3), 36));
  nhops.emplace(ResolvedNextHop(nextHopAddr4, InterfaceID(4), 36));
  nhops.emplace(ResolvedNextHop(nextHopAddr5, InterfaceID(5), 36));
  nhops.emplace(ResolvedNextHop(nextHopAddr6, InterfaceID(6), 36));
  nhops.emplace(ResolvedNextHop(nextHopAddr7, InterfaceID(7), 36));
  nhops.emplace(ResolvedNextHop(nextHopAddr8, InterfaceID(8), 36));

  auto normalizedNextHops =
      RouteNextHopEntry(nhops, kDefaultAdminDistance).normalizedNextHops();

  std::map<folly::IPAddress, NextHopWeight> expectedWeights = {
      {nextHopAddr1, 0},
      {nextHopAddr2, 1},
      {nextHopAddr3, 1},
      {nextHopAddr4, 1},
      {nextHopAddr5, 1},
      {nextHopAddr6, 1},
      {nextHopAddr7, 1},
      {nextHopAddr8, 1}};
  for (const auto& nhop : normalizedNextHops) {
    EXPECT_EQ(nhop.weight(), expectedWeights[nhop.addr()]);
  }
}

// optimized algorithm should remove the multiple weights
// even if removing one increases max error deviation in
// intermediate step
TEST(RouteNextHopEntry, OptimizedUcmpDiscardMultipleLowWeights) {
  RouteNextHopSet nhops;

  FLAGS_ecmp_width = 64;
  FLAGS_optimized_ucmp = true;

  nhops.emplace(ResolvedNextHop(nextHopAddr1, InterfaceID(1), 50));
  nhops.emplace(ResolvedNextHop(nextHopAddr2, InterfaceID(2), 50));
  nhops.emplace(ResolvedNextHop(nextHopAddr3, InterfaceID(3), 1));
  nhops.emplace(ResolvedNextHop(nextHopAddr4, InterfaceID(4), 1));
  nhops.emplace(ResolvedNextHop(nextHopAddr5, InterfaceID(5), 1));
  nhops.emplace(ResolvedNextHop(nextHopAddr6, InterfaceID(6), 1));
  nhops.emplace(ResolvedNextHop(nextHopAddr7, InterfaceID(7), 1));
  nhops.emplace(ResolvedNextHop(nextHopAddr8, InterfaceID(8), 1));

  auto normalizedNextHops =
      RouteNextHopEntry(nhops, kDefaultAdminDistance).normalizedNextHops();

  // optimized algorithm should remove the low weight and normalize remaining
  std::map<folly::IPAddress, NextHopWeight> expectedWeights = {
      {nextHopAddr1, 1},
      {nextHopAddr2, 1},
      {nextHopAddr3, 0},
      {nextHopAddr4, 0},
      {nextHopAddr5, 0},
      {nextHopAddr6, 0},
      {nextHopAddr7, 0},
      {nextHopAddr8, 0}};
  for (const auto& nhop : normalizedNextHops) {
    EXPECT_EQ(nhop.weight(), expectedWeights[nhop.addr()]);
  }
}

TEST(RouteNextHopEntry, Thrift) {
  RouteNextHopEntry drop0(
      RouteNextHopEntry::Action::DROP,
      facebook::fboss::AdminDistance::STATIC_ROUTE,
      "counter_drop",
      cfg::AclLookupClass::CLASS_DROP);
  RouteNextHopEntry drop1(
      RouteNextHopEntry::Action::DROP,
      facebook::fboss::AdminDistance::STATIC_ROUTE,
      "counter_drop");
  RouteNextHopEntry drop2(
      RouteNextHopEntry::Action::DROP,
      facebook::fboss::AdminDistance::STATIC_ROUTE,
      std::nullopt,
      cfg::AclLookupClass::CLASS_DROP);

  RouteNextHopEntry cpu0(
      RouteNextHopEntry::Action::TO_CPU,
      facebook::fboss::AdminDistance::STATIC_ROUTE,
      "counter_cpu",
      cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_0);

  RouteNextHopSet nhops;
  std::vector<ResolvedNextHop> nexthopsVector{};
  nexthopsVector.emplace_back(nextHopAddr1, InterfaceID(1), 50);
  nexthopsVector.emplace_back(nextHopAddr2, InterfaceID(2), 50);
  nexthopsVector.emplace_back(nextHopAddr3, InterfaceID(3), 1);
  nexthopsVector.emplace_back(nextHopAddr4, InterfaceID(4), 1);
  nexthopsVector.emplace_back(nextHopAddr5, InterfaceID(5), 1);
  nexthopsVector.emplace_back(nextHopAddr6, InterfaceID(6), 1);
  nexthopsVector.emplace_back(nextHopAddr7, InterfaceID(7), 1);
  nexthopsVector.emplace_back(nextHopAddr8, InterfaceID(8), 1);

  nhops.insert(std::begin(nexthopsVector), std::end(nexthopsVector));
  RouteNextHopEntry nhops0(nhops, kDefaultAdminDistance);
  RouteNextHopEntry nhops1(
      ResolvedNextHop(nextHopAddr1, InterfaceID(1), 50),
      kDefaultAdminDistance,
      "counter0",
      cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_0);

  validateNodeSerialization<RouteNextHopEntry, true>(drop0);
  validateNodeSerialization<RouteNextHopEntry, true>(drop1);
  validateNodeSerialization<RouteNextHopEntry, true>(drop2);
  validateNodeSerialization<RouteNextHopEntry, true>(cpu0);
  validateNodeSerialization<RouteNextHopEntry, true>(nhops0);
  validateNodeSerialization<RouteNextHopEntry, true>(nhops1);
}
