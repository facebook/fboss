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
RibRouteUpdater::RouteEntry makeIntfRoute(
    const folly::IPAddress& nw,
    uint8_t mask,
    const folly::IPAddress& nhop,
    InterfaceID intf) {
  ResolvedNextHop resolvedNextHop(nhop, intf, UCMP_DEFAULT_WEIGHT);
  return {{nw, mask}, {resolvedNextHop, AdminDistance::DIRECTLY_CONNECTED}};
}

void configRoutes(
    IPv4NetworkToRouteMap* v4Routes,
    IPv6NetworkToRouteMap* v6Routes) {
  RibRouteUpdater updater(v4Routes, v6Routes);

  updater.update(
      ClientID::INTERFACE_ROUTE,
      {
          makeIntfRoute(
              folly::IPAddress("1.1.1.1"),
              24,
              folly::IPAddress("1.1.1.1"),
              InterfaceID(1)),
          makeIntfRoute(
              folly::IPAddress("1::1"),
              48,
              folly::IPAddress("1::1"),
              InterfaceID(1)),

          makeIntfRoute(
              folly::IPAddress("2.2.2.2"),
              24,
              folly::IPAddress("2.2.2.2"),
              InterfaceID(2)),
          makeIntfRoute(
              folly::IPAddress("2::1"),
              48,
              folly::IPAddress("2::1"),
              InterfaceID(2)),

          makeIntfRoute(
              folly::IPAddress("3.3.3.3"),
              24,
              folly::IPAddress("3.3.3.3"),
              InterfaceID(3)),
          makeIntfRoute(
              folly::IPAddress("3::1"),
              48,
              folly::IPAddress("3::1"),
              InterfaceID(3)),

          makeIntfRoute(
              folly::IPAddress("4.4.4.4"),
              24,
              folly::IPAddress("4.4.4.4"),
              InterfaceID(4)),
          makeIntfRoute(
              folly::IPAddress("4::1"),
              48,
              folly::IPAddress("4::1"),
              InterfaceID(4)),
      },
      {},
      true);

  updater.update(
      ClientID::LINKLOCAL_ROUTE,
      {{{folly::IPAddress("fe80::"), 64},
        {RouteForwardAction::TO_CPU, AdminDistance::DIRECTLY_CONNECTED}}},
      {},
      true);
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
  u2.update(
      kClientA,
      {
          {{r1.network, r1.mask}, RouteNextHopEntry(nhop1, kDistance)},
          {{r3.network, r3.mask}, RouteNextHopEntry(nhop1, kDistance)},
      },
      {},
      false);
  u2.update(
      kClientB,
      {
          {{r2.network, r2.mask}, RouteNextHopEntry(nhop2, kDistance)},
          {{r4.network, r4.mask}, RouteNextHopEntry(nhop2, kDistance)},
      },
      {},
      false);
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
  u2.update(
      kClientA,
      {
          {{r1.network, r1.mask}, RouteNextHopEntry(nhop1, kDistance)},
          {{r2.network, r2.mask}, RouteNextHopEntry(nhop2, kDistance)},
          {{r3.network, r3.mask}, RouteNextHopEntry(nhop1, kDistance)},
          {{r4.network, r4.mask}, RouteNextHopEntry(nhop2, kDistance)},
      },
      {},
      false);

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

} // namespace facebook::fboss
