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
#include "fboss/agent/if/gen-cpp2/ctrl_types.h"
#include "fboss/agent/state/RouteNextHopEntry.h"

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

  linkLocalAddrAsBinaryAddress.ifName_ref() = "fboss1";

  return linkLocalAddrAsBinaryAddress;
}

// These are the Thrift representations of nextHopAddr{1,2,3}.
std::vector<NextHopThrift> nextHopsThrift() {
  std::vector<NextHopThrift> nexthops;
  std::vector<folly::IPAddress> addrs{nextHopAddr1, nextHopAddr2, nextHopAddr3};
  for (const auto& addr : addrs) {
    NextHopThrift nexthop;
    *nexthop.address_ref() = createV6LinkLocalNextHop(addr);
    *nexthop.weight_ref() = static_cast<int32_t>(ECMP_WEIGHT);
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
  route.dest_ref() = kDestPrefix;
  route.nextHops_ref() = nextHopsThrift();
  std::optional<RouteCounterID> counterID("route.counter.0");

  auto nextHopEntry =
      RouteNextHopEntry::from(route, kDefaultAdminDistance, counterID);

  ASSERT_EQ(nextHopEntry.getAction(), RouteForwardAction::NEXTHOPS);
  ASSERT_EQ(nextHopEntry.getAdminDistance(), kDefaultAdminDistance);
  ASSERT_EQ(nextHopEntry.getCounterID(), counterID);

  std::sort(nextHops.begin(), nextHops.end());
  ASSERT_TRUE(std::equal(
      nextHopEntry.getNextHopSet().begin(),
      nextHopEntry.getNextHopSet().end(),
      nextHops.begin()));
}

// The UnicastRoute.nextHopAddrs field has been deprecated in favor of
// UnicastRoutes.nextHops, which equips each next-hop with a weight
// in support of UCMP.
TEST(RouteNextHopEntry, FromBinaryAddresses) {
  // Note that we can't use UnicastRoute's constructor because it expects to be
  // passed both nextHopAddrs and nextHops
  UnicastRoute route;
  route.dest_ref() = kDestPrefix;
  route.nextHopAddrs_ref() = nextHopsBinaryAddress;
  std::optional<RouteCounterID> counterID("route.counter.0");

  auto nextHopEntry =
      RouteNextHopEntry::from(route, kDefaultAdminDistance, counterID);

  ASSERT_EQ(nextHopEntry.getAction(), RouteForwardAction::NEXTHOPS);
  ASSERT_EQ(nextHopEntry.getAdminDistance(), kDefaultAdminDistance);
  ASSERT_EQ(nextHopEntry.getCounterID(), counterID);

  std::sort(nextHops.begin(), nextHops.end());
  ASSERT_TRUE(std::equal(
      nextHopEntry.getNextHopSet().begin(),
      nextHopEntry.getNextHopSet().end(),
      nextHops.begin()));
}

TEST(RouteNextHopEntry, OverrideDefaultAdminDistance) {
  UnicastRoute route;
  route.dest_ref() = kDestPrefix;
  route.nextHops_ref() = nextHopsThrift();
  route.adminDistance_ref() = AdminDistance::IBGP;

  auto nextHopEntry =
      RouteNextHopEntry::from(route, kDefaultAdminDistance, std::nullopt);

  ASSERT_EQ(nextHopEntry.getAdminDistance(), AdminDistance::IBGP);
}

TEST(RouteNextHopEntry, EmptyListIsDrop) {
  std::vector<NextHopThrift> noNextHops = {};

  // Note that we can't use UnicastRoute's constructor because it expects to be
  // passed both nextHopAddrs and nextHops
  UnicastRoute route;
  route.dest_ref() = kDestPrefix;
  route.nextHops_ref() = noNextHops;

  auto nextHopEntry =
      RouteNextHopEntry::from(route, kDefaultAdminDistance, std::nullopt);

  ASSERT_EQ(nextHopEntry.getAction(), RouteForwardAction::DROP);
  ASSERT_EQ(nextHopEntry.getAdminDistance(), kDefaultAdminDistance);
  ASSERT_EQ(nextHopEntry.getNextHopSet().size(), 0);
}

// Total weight greater than 128. Should get normalized to 512
TEST(RouteNextHopEntry, NormalizedFixedSizeWideNextHop) {
  RouteNextHopSet nhops;

  FLAGS_ecmp_width = 512;
  FLAGS_wide_ecmp = true;

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

// Total weight greater than ecmp_width but can be reduced
// by proportionally reducing weights first and then fitting to 512
TEST(RouteNextHopEntry, ScaledDownNormalizedFixedSizeWideNextHop) {
  RouteNextHopSet nhops;

  FLAGS_ecmp_width = 512;
  FLAGS_wide_ecmp = true;

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

// Total width exceeds 512 after proportionaly reducing weights.
// should get normalized by reducing the high weight members
TEST(RouteNextHopEntry, ScaledDownFixedSizeWideNextHop) {
  RouteNextHopSet nhops;

  FLAGS_ecmp_width = 512;
  FLAGS_wide_ecmp = true;

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
  EXPECT_EQ(totalWeight, FLAGS_ecmp_width);
}

TEST(RouteNextHopEntry, NotNormalizedWideECmpEnabled) {
  RouteNextHopSet nhops;

  FLAGS_ecmp_width = 512;
  FLAGS_wide_ecmp = true;

  nhops.emplace(ResolvedNextHop(nextHopAddr1, InterfaceID(1), 32));
  nhops.emplace(ResolvedNextHop(nextHopAddr2, InterfaceID(2), 32));
  nhops.emplace(ResolvedNextHop(nextHopAddr3, InterfaceID(3), 32));

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
