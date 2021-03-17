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
const ClientID kClientC = ClientID(1003);
const ClientID kClientD = ClientID(1004);
const ClientID kClientE = ClientID(1005);
} // namespace

using folly::IPAddress;
using folly::IPAddressV4;
using folly::IPAddressV6;

constexpr AdminDistance kDistance = AdminDistance::MAX_ADMIN_DISTANCE;

namespace {
// TODO(samank): move helpers into test fixture
template <typename AddressT>
std::shared_ptr<Route<AddressT>> getRoute(
    NetworkToRouteMap<AddressT>& routes,
    std::string prefixAsString) {
  auto prefix = RoutePrefix<AddressT>::fromString(prefixAsString);
  auto it = routes.exactMatch(prefix.network, prefix.mask);

  return it->value();
}

RouteNextHopSet makeNextHops(std::vector<std::string> ipsAsStrings) {
  RouteNextHopSet nhops;
  for (const std::string& ipAsString : ipsAsStrings) {
    nhops.emplace(UnresolvedNextHop(IPAddress(ipAsString), ECMP_WEIGHT));
  }
  return nhops;
}

template <typename AddrT>
void EXPECT_FWD_INFO(
    const std::shared_ptr<Route<AddrT>>& route,
    InterfaceID interfaceID,
    std::string ipAsString) {
  const auto& fwds = route->getForwardInfo().getNextHopSet();
  EXPECT_EQ(1, fwds.size());
  const auto& fwd = *fwds.begin();
  EXPECT_EQ(interfaceID, fwd.intf());
  EXPECT_EQ(IPAddress(ipAsString), fwd.addr());
}

template <typename AddrT>
void EXPECT_RESOLVED(const std::shared_ptr<Route<AddrT>>& route) {
  EXPECT_TRUE(route->isResolved());
  EXPECT_FALSE(route->isUnresolvable());
  EXPECT_FALSE(route->needResolve());
}

template <typename AddrT>
void EXPECT_ROUTES_MATCH(
    const NetworkToRouteMap<AddrT>* routesA,
    const NetworkToRouteMap<AddrT>* routesB) {
  EXPECT_EQ(routesA->size(), routesB->size());
  for (const auto& entryA : *routesA) {
    auto routeA = entryA.value();
    auto prefixA = routeA->prefix();

    auto iterB = routesB->exactMatch(prefixA.network, prefixA.mask);
    ASSERT_NE(routesB->end(), iterB);
    auto routeB = iterB->value();

    EXPECT_TRUE(routeB->isSame(routeA.get()));
  }
}

template <typename AddressT>
std::shared_ptr<Route<AddressT>> longestMatch(
    NetworkToRouteMap<AddressT>& routes,
    AddressT address) {
  auto it = routes.longestMatch(address, address.bitCount());
  CHECK(it != routes.end());
  return it->value();
}

} // namespace

namespace facebook::fboss {

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
  RibRouteUpdater updater(v4Routes, v6Routes);

  updater.addOrReplaceInterfaceRoute(
      folly::IPAddress("1.1.1.1"),
      24,
      folly::IPAddress("1.1.1.1"),
      InterfaceID(1));
  updater.addOrReplaceInterfaceRoute(
      folly::IPAddress("1::1"), 48, folly::IPAddress("1::1"), InterfaceID(1));

  updater.addOrReplaceInterfaceRoute(
      folly::IPAddress("2.2.2.2"),
      24,
      folly::IPAddress("2.2.2.2"),
      InterfaceID(2));
  updater.addOrReplaceInterfaceRoute(
      folly::IPAddress("2::1"), 48, folly::IPAddress("2::1"), InterfaceID(2));

  updater.addOrReplaceInterfaceRoute(
      folly::IPAddress("3.3.3.3"),
      24,
      folly::IPAddress("3.3.3.3"),
      InterfaceID(3));
  updater.addOrReplaceInterfaceRoute(
      folly::IPAddress("3::1"), 48, folly::IPAddress("3::1"), InterfaceID(3));

  updater.addOrReplaceInterfaceRoute(
      folly::IPAddress("4.4.4.4"),
      24,
      folly::IPAddress("4.4.4.4"),
      InterfaceID(4));
  updater.addOrReplaceInterfaceRoute(
      folly::IPAddress("4::1"), 48, folly::IPAddress("4::1"), InterfaceID(4));

  updater.addLinkLocalRoutes();

  updater.updateDone();
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
  u2.addOrReplaceRoute(
      r1.network, r1.mask, kClientA, RouteNextHopEntry(nhop1, kDistance));
  u2.addOrReplaceRoute(
      r2.network, r2.mask, kClientB, RouteNextHopEntry(nhop2, kDistance));
  u2.addOrReplaceRoute(
      r3.network, r3.mask, kClientA, RouteNextHopEntry(nhop1, kDistance));
  u2.addOrReplaceRoute(
      r4.network, r4.mask, kClientB, RouteNextHopEntry(nhop2, kDistance));
  u2.updateDone();

  // No routes for clientC
  EXPECT_EQ(0, u2.removeAllRoutesForClient(kClientC).size());
  // Remove routes
  EXPECT_EQ(2, u2.removeAllRoutesForClient(kClientA).size());
  EXPECT_EQ(2, u2.removeAllRoutesForClient(kClientB).size());
  // No more routes after removal
  EXPECT_EQ(0, u2.removeAllRoutesForClient(kClientA).size());
  EXPECT_EQ(0, u2.removeAllRoutesForClient(kClientB).size());
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
    // We only check next hops and not nextHopAddrs, since the latter was
    // deprecated
    EXPECT_GE(entry.nextHops.size(), 1);
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

    auto entry = route->getEntryForClient(ClientID(clientId));
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

    auto entry = route->getEntryForClient(ClientID(clientId));
    EXPECT_NE(nullptr, entry);
  }
}

TEST(Route, dropRoutes) {
  IPv4NetworkToRouteMap v4Routes;
  IPv6NetworkToRouteMap v6Routes;

  RibRouteUpdater u1(&v4Routes, &v6Routes);
  u1.addOrReplaceRoute(
      IPAddress("10.10.10.10"),
      32,
      kClientA,
      RouteNextHopEntry(RouteForwardAction::DROP, kDistance));
  u1.addOrReplaceRoute(
      IPAddress("2001::0"),
      128,
      kClientA,
      RouteNextHopEntry(RouteForwardAction::DROP, kDistance));
  // Check recursive resolution for drop routes
  RouteNextHopSet v4nexthops = makeNextHops({"10.10.10.10"});
  u1.addOrReplaceRoute(
      IPAddress("20.20.20.0"),
      24,
      kClientA,
      RouteNextHopEntry(v4nexthops, kDistance));
  RouteNextHopSet v6nexthops = makeNextHops({"2001::0"});
  u1.addOrReplaceRoute(
      IPAddress("2001:1::"),
      64,
      kClientA,
      RouteNextHopEntry(v6nexthops, kDistance));

  u1.updateDone();

  // Check routes
  auto r1 = getRoute(v4Routes, "10.10.10.10/32");
  EXPECT_RESOLVED(r1);
  EXPECT_FALSE(r1->isConnected());
  EXPECT_TRUE(r1->has(
      kClientA, RouteNextHopEntry(RouteForwardAction::DROP, kDistance)));
  EXPECT_EQ(
      r1->getForwardInfo(),
      RouteNextHopEntry(RouteForwardAction::DROP, kDistance));

  auto r2 = getRoute(v4Routes, "20.20.20.0/24");
  EXPECT_RESOLVED(r2);
  EXPECT_FALSE(r2->isConnected());
  EXPECT_FALSE(r2->has(
      kClientA, RouteNextHopEntry(RouteForwardAction::DROP, kDistance)));
  EXPECT_EQ(
      r2->getForwardInfo(),
      RouteNextHopEntry(RouteForwardAction::DROP, kDistance));

  auto r3 = getRoute(v6Routes, "2001::0/128");
  EXPECT_RESOLVED(r3);
  EXPECT_FALSE(r3->isConnected());
  EXPECT_TRUE(r3->has(
      kClientA, RouteNextHopEntry(RouteForwardAction::DROP, kDistance)));
  EXPECT_EQ(
      r3->getForwardInfo(),
      RouteNextHopEntry(RouteForwardAction::DROP, kDistance));

  auto r4 = getRoute(v6Routes, "2001:1::/64");
  EXPECT_RESOLVED(r4);
  EXPECT_FALSE(r4->isConnected());
  EXPECT_EQ(
      r4->getForwardInfo(),
      RouteNextHopEntry(RouteForwardAction::DROP, kDistance));
}

TEST(Route, toCPURoutes) {
  IPv4NetworkToRouteMap v4Routes;
  IPv6NetworkToRouteMap v6Routes;

  RibRouteUpdater u1(&v4Routes, &v6Routes);
  u1.addOrReplaceRoute(
      IPAddress("10.10.10.10"),
      32,
      kClientA,
      RouteNextHopEntry(RouteForwardAction::TO_CPU, kDistance));
  u1.addOrReplaceRoute(
      IPAddress("2001::0"),
      128,
      kClientA,
      RouteNextHopEntry(RouteForwardAction::TO_CPU, kDistance));
  // Check recursive resolution for to_cpu routes
  RouteNextHopSet v4nexthops = makeNextHops({"10.10.10.10"});
  u1.addOrReplaceRoute(
      IPAddress("20.20.20.0"),
      24,
      kClientA,
      RouteNextHopEntry(v4nexthops, kDistance));
  RouteNextHopSet v6nexthops = makeNextHops({"2001::0"});
  u1.addOrReplaceRoute(
      IPAddress("2001:1::"),
      64,
      kClientA,
      RouteNextHopEntry(v6nexthops, kDistance));

  u1.updateDone();

  // Check routes
  auto r1 = getRoute(v4Routes, "10.10.10.10/32");
  EXPECT_RESOLVED(r1);
  EXPECT_FALSE(r1->isConnected());
  EXPECT_TRUE(r1->has(
      kClientA, RouteNextHopEntry(RouteForwardAction::TO_CPU, kDistance)));
  EXPECT_EQ(
      r1->getForwardInfo(),
      RouteNextHopEntry(RouteForwardAction::TO_CPU, kDistance));

  auto r2 = getRoute(v4Routes, "20.20.20.0/24");
  EXPECT_RESOLVED(r2);
  EXPECT_FALSE(r2->isConnected());
  EXPECT_EQ(
      r2->getForwardInfo(),
      RouteNextHopEntry(RouteForwardAction::TO_CPU, kDistance));

  auto r3 = getRoute(v6Routes, "2001::0/128");
  EXPECT_RESOLVED(r3);
  EXPECT_FALSE(r3->isConnected());
  EXPECT_TRUE(r3->has(
      kClientA, RouteNextHopEntry(RouteForwardAction::TO_CPU, kDistance)));
  EXPECT_EQ(
      r3->getForwardInfo(),
      RouteNextHopEntry(RouteForwardAction::TO_CPU, kDistance));

  auto r5 = getRoute(v6Routes, "2001:1::/64");
  EXPECT_RESOLVED(r5);
  EXPECT_FALSE(r5->isConnected());
  EXPECT_EQ(
      r5->getForwardInfo(),
      RouteNextHopEntry(RouteForwardAction::TO_CPU, kDistance));
}

// Very basic test for serialization/deseralization of Routes
TEST(Route, serializeRoute) {
  ClientID clientId = ClientID(1);
  auto nxtHops = makeNextHops({"10.10.10.10", "11.11.11.11"});
  auto rt = std::make_shared<Route<IPAddressV4>>(
      RoutePrefixV4::fromString("1.2.3.4/32"));
  rt->update(clientId, RouteNextHopEntry(nxtHops, kDistance));

  // to folly dynamic
  folly::dynamic obj = rt->toFollyDynamic();
  // to string
  folly::json::serialization_opts serOpts;
  serOpts.allow_non_string_keys = true;
  std::string json = folly::json::serialize(obj, serOpts);
  // back to folly dynamic
  folly::dynamic obj2 = folly::parseJson(json, serOpts);
  // back to Route object
  auto rt2 = Route<IPAddressV4>::fromFollyDynamic(obj2);
  ASSERT_TRUE(rt2->has(clientId, RouteNextHopEntry(nxtHops, kDistance)));
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

  RibRouteUpdater u2(&v4Routes, &v6Routes);
  u2.addOrReplaceRoute(
      r1.network, r1.mask, kClientA, RouteNextHopEntry(nhop1, kDistance));
  u2.addOrReplaceRoute(
      r2.network, r2.mask, kClientA, RouteNextHopEntry(nhop2, kDistance));
  u2.addOrReplaceRoute(
      r3.network, r3.mask, kClientA, RouteNextHopEntry(nhop1, kDistance));
  u2.addOrReplaceRoute(
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
  auto nhts = facebook::fboss::util::fromRouteNextHopSet(nhs);
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
  auto newNhs = facebook::fboss::util::toRouteNextHopSet(nhts);
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
    NextHop nh = facebook::fboss::util::fromThrift(nht);
    EXPECT_EQ(folly::IPAddress("10.0.0.1"), nh.addr());
    EXPECT_EQ(std::nullopt, nh.intfID());
  }

  addr = facebook::network::toBinaryAddress(folly::IPAddress("face::1"));
  addr.ifName_ref() = "fboss10";
  *nht.address_ref() = addr;
  {
    NextHop nh = facebook::fboss::util::fromThrift(nht);
    EXPECT_EQ(folly::IPAddress("face::1"), nh.addr());
    EXPECT_EQ(std::nullopt, nh.intfID());
  }

  addr = facebook::network::toBinaryAddress(folly::IPAddress("fe80::1"));
  addr.ifName_ref() = "fboss10";
  *nht.address_ref() = addr;
  {
    NextHop nh = facebook::fboss::util::fromThrift(nht);
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

  RibRouteUpdater updater(&v4Routes, &v6Routes);
  updater.addOrReplaceRoute(
      folly::IPAddressV4("1.1.0.0"),
      16,
      kClientA,
      RouteNextHopEntry(nexthops, kDistance));

  updater.updateDone();

  EXPECT_EQ(1, v4Routes.size());
  EXPECT_EQ(0, v6Routes.size());

  const auto route = longestMatch(v4Routes, folly::IPAddressV4("1.1.2.2"));

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

  RibRouteUpdater updater(&v4Routes, &v6Routes);

  updater.addOrReplaceRoute(
      folly::IPAddressV4("1.1.0.0"), 16, kClientA, routeNextHopEntry);

  updater.updateDone();

  EXPECT_EQ(1, v4Routes.size());
  EXPECT_EQ(0, v6Routes.size());

  const auto route = longestMatch(v4Routes, folly::IPAddressV4("1.1.2.2"));

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

  RibRouteUpdater updater(&v4Routes, &v6Routes);

  EXPECT_THROW(
      {
        updater.addOrReplaceRoute(
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
    RibRouteUpdater ru(&v4Routes_, &v6Routes_);
    for (const auto& nnhs : networkAndNextHops) {
      folly::IPAddress net = nnhs.first;
      RouteNextHopSet nhs = nnhs.second;
      ru.addOrReplaceRoute(
          net, mask, kClientA, RouteNextHopEntry(nhs, kDistance));
    }
    ru.updateDone();

    for (const auto& nnhs : networkAndNextHops) {
      folly::IPAddress net = nnhs.first;
      auto pfx = folly::sformat("{}/{}", net.str(), mask);
      auto r = getRoute(v4Routes_, pfx);
      EXPECT_RESOLVED(r);
      EXPECT_FALSE(r->isConnected());
      resolvedRoutes.push_back(r);
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
    EXPECT_EQ(expFwd1, resolvedRoutes[0]->getForwardInfo().getNextHopSet());
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

  std::vector<std::shared_ptr<Route<folly::IPAddressV4>>> resolvedRoutes;
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
  EXPECT_EQ(route2ExpFwd, resolvedRoutes[1]->getForwardInfo().getNextHopSet());
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

} // namespace facebook::fboss
