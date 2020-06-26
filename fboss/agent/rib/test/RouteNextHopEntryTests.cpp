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
#include "fboss/agent/rib/RouteNextHop.h"
#include "fboss/agent/rib/RouteNextHopEntry.h"
#include "fboss/agent/rib/RouteTypes.h"

#include <folly/IPAddress.h>
#include <gtest/gtest.h>
#include <algorithm>
#include <vector>

using namespace facebook::fboss::rib;

using facebook::fboss::AdminDistance;
using facebook::fboss::InterfaceID;
using facebook::fboss::IpPrefix;
using facebook::fboss::MplsAction;
using facebook::fboss::NextHopThrift;
using facebook::fboss::UnicastRoute;

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

  auto nextHopEntry = RouteNextHopEntry::from(route, kDefaultAdminDistance);

  ASSERT_EQ(nextHopEntry.getAction(), RouteForwardAction::NEXTHOPS);
  ASSERT_EQ(nextHopEntry.getAdminDistance(), kDefaultAdminDistance);

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

  auto nextHopEntry = RouteNextHopEntry::from(route, kDefaultAdminDistance);

  ASSERT_EQ(nextHopEntry.getAction(), RouteForwardAction::NEXTHOPS);
  ASSERT_EQ(nextHopEntry.getAdminDistance(), kDefaultAdminDistance);

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

  auto nextHopEntry = RouteNextHopEntry::from(route, kDefaultAdminDistance);

  ASSERT_EQ(nextHopEntry.getAdminDistance(), AdminDistance::IBGP);
}

TEST(RouteNextHopEntry, EmptyListIsDrop) {
  std::vector<NextHopThrift> noNextHops = {};

  // Note that we can't use UnicastRoute's constructor because it expects to be
  // passed both nextHopAddrs and nextHops
  UnicastRoute route;
  route.dest_ref() = kDestPrefix;
  route.nextHops_ref() = noNextHops;

  auto nextHopEntry = RouteNextHopEntry::from(route, kDefaultAdminDistance);

  ASSERT_EQ(nextHopEntry.getAction(), RouteForwardAction::DROP);
  ASSERT_EQ(nextHopEntry.getAdminDistance(), kDefaultAdminDistance);
  ASSERT_EQ(nextHopEntry.getNextHopSet().size(), 0);
}
