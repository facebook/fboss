/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/AddressUtil.h"
#include "fboss/agent/ApplyThriftConfig.h"
#include "fboss/agent/FbossError.h"
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/hw/mock/MockPlatform.h"
#include "fboss/agent/state/DeltaFunctions.h"
#include "fboss/agent/state/Interface.h"
#include "fboss/agent/state/InterfaceMap.h"
#include "fboss/agent/state/NodeMapDelta-defs.h"
#include "fboss/agent/state/NodeMapDelta.h"
#include "fboss/agent/state/Route.h"
#include "fboss/agent/state/RouteDelta.h"
#include "fboss/agent/state/RouteTable.h"
#include "fboss/agent/state/RouteTableMap.h"
#include "fboss/agent/state/RouteTableRib.h"
#include "fboss/agent/state/RouteUpdater.h"
#include "fboss/agent/state/StateDelta.h"
#include "fboss/agent/state/StateUtils.h"
#include "fboss/agent/state/SwitchState-defs.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/test/TestUtils.h"

#include <folly/logging/xlog.h>
#include <gtest/gtest.h>
#include <optional>

using namespace facebook::fboss;
using folly::IPAddress;
using folly::IPAddressV4;
using folly::IPAddressV6;
using std::make_shared;
using std::shared_ptr;
using ::testing::Return;

//
// Helper functions
//

template <typename AddrT>
void EXPECT_NODEMAP_MATCH(const std::shared_ptr<RouteTableRib<AddrT>>& rib) {
  const auto& radixTree = rib->routesRadixTree();
  EXPECT_EQ(rib->size(), radixTree.size());
  for (const auto& route : *(rib->routes())) {
    auto match =
        radixTree.exactMatch(route->prefix().network, route->prefix().mask);
    ASSERT_NE(match, radixTree.end());
    // should be the same shared_ptr
    EXPECT_EQ(route, match->value());
  }
}

void EXPECT_NODEMAP_MATCH(const std::shared_ptr<RouteTableMap>& routeTables) {
  for (const auto& rt : *routeTables) {
    if (rt->getRibV4()) {
      EXPECT_NODEMAP_MATCH<IPAddressV4>(rt->getRibV4());
    }
    if (rt->getRibV6()) {
      EXPECT_NODEMAP_MATCH<IPAddressV6>(rt->getRibV6());
    }
  }
}

//
// Tests
//
#define CLIENT_A ClientID(1001)
#define CLIENT_B ClientID(1002)
#define CLIENT_C ClientID(1003)
#define CLIENT_D ClientID(1004)
#define CLIENT_E ClientID(1005)

constexpr AdminDistance DISTANCE = AdminDistance::MAX_ADMIN_DISTANCE;

std::shared_ptr<SwitchState> applyInitConfig() {
  auto platform = createMockPlatform();
  auto stateV0 = make_shared<SwitchState>();
  auto tablesV0 = stateV0->getRouteTables();

  cfg::SwitchConfig config;
  config.vlans_ref()->resize(4);
  *config.vlans_ref()[0].id_ref() = 1;
  *config.vlans_ref()[1].id_ref() = 2;
  *config.vlans_ref()[2].id_ref() = 3;
  *config.vlans_ref()[3].id_ref() = 4;

  config.interfaces_ref()->resize(4);
  *config.interfaces_ref()[0].intfID_ref() = 1;
  *config.interfaces_ref()[0].vlanID_ref() = 1;
  *config.interfaces_ref()[0].routerID_ref() = 0;
  config.interfaces_ref()[0].mac_ref() = "00:00:00:00:00:11";
  config.interfaces_ref()[0].ipAddresses_ref()->resize(2);
  config.interfaces_ref()[0].ipAddresses_ref()[0] = "1.1.1.1/24";
  config.interfaces_ref()[0].ipAddresses_ref()[1] = "1::1/48";

  *config.interfaces_ref()[1].intfID_ref() = 2;
  *config.interfaces_ref()[1].vlanID_ref() = 2;
  *config.interfaces_ref()[1].routerID_ref() = 0;
  config.interfaces_ref()[1].mac_ref() = "00:00:00:00:00:22";
  config.interfaces_ref()[1].ipAddresses_ref()->resize(2);
  config.interfaces_ref()[1].ipAddresses_ref()[0] = "2.2.2.2/24";
  config.interfaces_ref()[1].ipAddresses_ref()[1] = "2::1/48";

  *config.interfaces_ref()[2].intfID_ref() = 3;
  *config.interfaces_ref()[2].vlanID_ref() = 3;
  *config.interfaces_ref()[2].routerID_ref() = 0;
  config.interfaces_ref()[2].mac_ref() = "00:00:00:00:00:33";
  config.interfaces_ref()[2].ipAddresses_ref()->resize(2);
  config.interfaces_ref()[2].ipAddresses_ref()[0] = "3.3.3.3/24";
  config.interfaces_ref()[2].ipAddresses_ref()[1] = "3::1/48";

  *config.interfaces_ref()[3].intfID_ref() = 4;
  *config.interfaces_ref()[3].vlanID_ref() = 4;
  *config.interfaces_ref()[3].routerID_ref() = 0;
  config.interfaces_ref()[3].mac_ref() = "00:00:00:00:00:44";
  config.interfaces_ref()[3].ipAddresses_ref()->resize(2);
  config.interfaces_ref()[3].ipAddresses_ref()[0] = "4.4.4.4/24";
  config.interfaces_ref()[3].ipAddresses_ref()[1] = "4::1/48";

  auto stateV1 = publishAndApplyConfig(stateV0, &config, platform.get());
  stateV1->publish();
  return stateV1;
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

// Test equality of RouteNextHopsMulti.
TEST(Route, equality) {
  // Create two identical RouteNextHopsMulti, and compare
  RouteNextHopsMulti nhm1;
  nhm1.update(CLIENT_A, RouteNextHopEntry(newNextHops(3, "1.1.1."), DISTANCE));
  nhm1.update(CLIENT_B, RouteNextHopEntry(newNextHops(3, "2.2.2."), DISTANCE));

  RouteNextHopsMulti nhm2;
  nhm2.update(CLIENT_A, RouteNextHopEntry(newNextHops(3, "1.1.1."), DISTANCE));
  nhm2.update(CLIENT_B, RouteNextHopEntry(newNextHops(3, "2.2.2."), DISTANCE));

  EXPECT_TRUE(nhm1 == nhm2);

  // Delete data for CLIENT_C.  But there wasn't any.  Two objs still equal
  nhm1.delEntryForClient(CLIENT_C);
  EXPECT_TRUE(nhm1 == nhm2);

  // Delete obj1's CLIENT_B.  Now, objs should be NOT equal
  nhm1.delEntryForClient(CLIENT_B);
  EXPECT_FALSE(nhm1 == nhm2);

  // Now replace obj1's CLIENT_B list with a shorter list.
  // Objs should be NOT equal.
  nhm1.update(CLIENT_B, RouteNextHopEntry(newNextHops(2, "2.2.2."), DISTANCE));
  EXPECT_FALSE(nhm1 == nhm2);

  // Now replace obj1's CLIENT_B list with the original list.
  // But construct the list in opposite order.
  // Objects should still be equal, despite the order of construction.
  RouteNextHopSet nextHopsRev;
  nextHopsRev.emplace(
      UnresolvedNextHop(IPAddress("2.2.2.12"), UCMP_DEFAULT_WEIGHT));
  nextHopsRev.emplace(
      UnresolvedNextHop(IPAddress("2.2.2.11"), UCMP_DEFAULT_WEIGHT));
  nextHopsRev.emplace(
      UnresolvedNextHop(IPAddress("2.2.2.10"), UCMP_DEFAULT_WEIGHT));
  nhm1.update(CLIENT_B, RouteNextHopEntry(nextHopsRev, DISTANCE));
  EXPECT_TRUE(nhm1 == nhm2);
}

// Test that a copy of a RouteNextHopsMulti is a deep copy, and that the
// resulting objects can be modified independently.
TEST(Route, deepCopy) {
  // Create two identical RouteNextHopsMulti, and compare
  RouteNextHopsMulti nhm1;
  auto origHops = newNextHops(3, "1.1.1.");
  nhm1.update(CLIENT_A, RouteNextHopEntry(origHops, DISTANCE));
  nhm1.update(CLIENT_B, RouteNextHopEntry(newNextHops(3, "2.2.2."), DISTANCE));

  // Copy it
  RouteNextHopsMulti nhm2 = nhm1;

  // The two should be identical
  EXPECT_TRUE(nhm1 == nhm2);

  // Now modify the underlying nexthop list.
  // Should be changed in nhm1, but not nhm2.
  auto newHops = newNextHops(4, "10.10.10.");
  nhm1.update(CLIENT_A, RouteNextHopEntry(newHops, DISTANCE));

  EXPECT_FALSE(nhm1 == nhm2);

  EXPECT_TRUE(nhm1.isSame(CLIENT_A, RouteNextHopEntry(newHops, DISTANCE)));
  EXPECT_TRUE(nhm2.isSame(CLIENT_A, RouteNextHopEntry(origHops, DISTANCE)));
}

// Test serialization of RouteNextHopsMulti.
TEST(Route, serializeRouteNextHopsMulti) {
  // This function tests [de]serialization of:
  // RouteNextHopsMulti
  // RouteNextHopEntry
  // NextHop

  // test for new format to new format
  RouteNextHopsMulti nhm1;
  nhm1.update(CLIENT_A, RouteNextHopEntry(newNextHops(3, "1.1.1."), DISTANCE));
  nhm1.update(CLIENT_B, RouteNextHopEntry(newNextHops(1, "2.2.2."), DISTANCE));
  nhm1.update(CLIENT_C, RouteNextHopEntry(newNextHops(4, "3.3.3."), DISTANCE));
  nhm1.update(CLIENT_D, RouteNextHopEntry(RouteForwardAction::DROP, DISTANCE));
  nhm1.update(
      CLIENT_E, RouteNextHopEntry(RouteForwardAction::TO_CPU, DISTANCE));

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

// Very basic test for serialization/deseralization of Routes
TEST(Route, serializeRoute) {
  ClientID clientId = ClientID(1);
  auto nxtHops = makeNextHops({"10.10.10.10", "11.11.11.11"});
  Route<IPAddressV4> rt(makePrefixV4("1.2.3.4/32"));
  rt.update(clientId, RouteNextHopEntry(nxtHops, DISTANCE));

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
  ASSERT_TRUE(rt2->has(clientId, RouteNextHopEntry(nxtHops, DISTANCE)));
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
  auto nhts = util::fromRouteNextHopSet(nhs);
  ASSERT_EQ(4, nhts.size());

  auto verify = [&](const std::string& ipaddr,
                    std::optional<InterfaceID> intf) {
    auto bAddr = facebook::network::toBinaryAddress(folly::IPAddress(ipaddr));
    if (intf.has_value()) {
      bAddr.ifName_ref() = util::createTunIntfName(intf.value());
    }
    bool found = false;
    for (const auto& entry : nhts) {
      if (*entry.address_ref() == bAddr) {
        if (intf.has_value()) {
          EXPECT_TRUE(entry.address_ref()->ifName_ref());
          EXPECT_EQ(*bAddr.ifName_ref(), *entry.address_ref()->ifName_ref());
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
  auto newNhs = util::toRouteNextHopSet(nhts);
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
    NextHop nh = util::fromThrift(nht);
    EXPECT_EQ(folly::IPAddress("10.0.0.1"), nh.addr());
    EXPECT_EQ(std::nullopt, nh.intfID());
  }

  addr = facebook::network::toBinaryAddress(folly::IPAddress("face::1"));
  addr.ifName_ref() = "fboss10";
  *nht.address_ref() = addr;
  {
    NextHop nh = util::fromThrift(nht);
    EXPECT_EQ(folly::IPAddress("face::1"), nh.addr());
    EXPECT_EQ(std::nullopt, nh.intfID());
  }

  addr = facebook::network::toBinaryAddress(folly::IPAddress("fe80::1"));
  addr.ifName_ref() = "fboss10";
  *nht.address_ref() = addr;
  {
    NextHop nh = util::fromThrift(nht);
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

TEST(Route, nodeMapMatchesRadixTree) {
  auto stateV1 = applyInitConfig();
  ASSERT_NE(nullptr, stateV1);
  auto tables1 = stateV1->getRouteTables();

  auto rid = RouterID(0);
  RouteNextHopSet nhop1 = makeNextHops({"1.1.1.10"}); // resolved by intf 1
  RouteNextHopSet nhop2 = makeNextHops({"2.2.2.10"}); // resolved by intf 2
  RouteNextHopSet nonResolvedHops = makeNextHops({"1.1.3.10"}); // Non-resolved

  RouteV4::Prefix r1{IPAddressV4("10.1.1.0"), 24};
  RouteV4::Prefix r2{IPAddressV4("20.1.1.0"), 24};
  RouteV6::Prefix r3{IPAddressV6("1001::0"), 48};
  RouteV6::Prefix r4{IPAddressV6("2001::0"), 48};
  RouteV4::Prefix r5{IPAddressV4("8.8.8.0"), 24};
  RouteV4::Prefix r6{IPAddressV4("1.1.3.0"), 24};

  // add route case
  RouteUpdater u1(tables1);
  u1.addRoute(
      rid, r1.network, r1.mask, CLIENT_A, RouteNextHopEntry(nhop1, DISTANCE));
  u1.addRoute(
      rid, r2.network, r2.mask, CLIENT_A, RouteNextHopEntry(nhop2, DISTANCE));
  u1.addRoute(
      rid, r3.network, r3.mask, CLIENT_A, RouteNextHopEntry(nhop1, DISTANCE));
  u1.addRoute(
      rid, r4.network, r4.mask, CLIENT_A, RouteNextHopEntry(nhop2, DISTANCE));
  u1.addRoute(
      rid,
      r5.network,
      r5.mask,
      CLIENT_A,
      RouteNextHopEntry(nonResolvedHops, DISTANCE));
  auto tables2 = u1.updateDone();
  ASSERT_NE(nullptr, tables2);
  // check every node in nodeMap_ also matches node in rib_, and every node
  // should be published after the routeTable is published
  EXPECT_NODEMAP_MATCH(tables2);
  // make sure new change won't affect the initial state
  EXPECT_NO_ROUTE(tables1, rid, r5.str());

  // del route case
  RouteUpdater u2(tables2);
  u2.delRoute(rid, r2.network, r2.mask, CLIENT_A);
  auto tables3 = u2.updateDone();
  ASSERT_NE(nullptr, tables3);
  EXPECT_NODEMAP_MATCH(tables3);

  // update route case, resolve previous nonResolved route with prefix r5
  RouteUpdater u3(tables3);
  u3.addRoute(
      rid, r6.network, r6.mask, CLIENT_A, RouteNextHopEntry(nhop1, DISTANCE));
  auto tables4 = u3.updateDone();
  EXPECT_NODEMAP_MATCH(tables4);
  ASSERT_NE(nullptr, tables4);
  // make sure new change won't affect the initial state
  EXPECT_NO_ROUTE(tables1, rid, r6.str());
  // both r5 and r6 should be unpublished now
  ASSERT_FALSE(GET_ROUTE_V4(tables4, rid, r5)->isPublished());
  ASSERT_FALSE(GET_ROUTE_V4(tables4, rid, r6)->isPublished());
}

TEST(Route, nexthopFromThriftAndDynamic) {
  IPAddressV6 ip{"fe80::1"};
  NextHopThrift nexthop;
  nexthop.address_ref()->addr.append(
      reinterpret_cast<const char*>(ip.bytes()),
      folly::IPAddressV6::byteCount());
  nexthop.address_ref()->ifName_ref() = "fboss100";
  MplsAction action;
  *action.action_ref() = MplsActionCode::PUSH;
  action.pushLabels_ref() = MplsLabelStack{501, 502, 503};
  EXPECT_EQ(util::fromThrift(nexthop).toThrift(), nexthop);
  EXPECT_EQ(
      util::nextHopFromFollyDynamic(util::fromThrift(nexthop).toFollyDynamic()),
      util::fromThrift(nexthop));
}

TEST(RouteNextHopEntry, toUnicastRouteDrop) {
  folly::CIDRNetwork nw{folly::IPAddress{"1::1"}, 64};
  auto unicastRoute = util::toUnicastRoute(
      nw, RouteNextHopEntry(RouteForwardAction::DROP, DISTANCE));

  EXPECT_EQ(
      nw,
      folly::CIDRNetwork(
          facebook::network::toIPAddress(*unicastRoute.dest_ref()->ip_ref()),
          *unicastRoute.dest_ref()->prefixLength_ref()));
  EXPECT_EQ(RouteForwardAction::DROP, unicastRoute.action_ref());
  EXPECT_EQ(0, unicastRoute.nextHops_ref()->size());
}

TEST(RouteNextHopEntry, toUnicastRouteCPU) {
  folly::CIDRNetwork nw{folly::IPAddress{"1.1.1.1"}, 24};
  auto unicastRoute = util::toUnicastRoute(
      nw, RouteNextHopEntry(RouteForwardAction::TO_CPU, DISTANCE));

  EXPECT_EQ(
      nw,
      folly::CIDRNetwork(
          facebook::network::toIPAddress(*unicastRoute.dest_ref()->ip_ref()),
          *unicastRoute.dest_ref()->prefixLength_ref()));
  EXPECT_EQ(RouteForwardAction::TO_CPU, unicastRoute.action_ref());
  EXPECT_EQ(0, unicastRoute.nextHops_ref()->size());
}

TEST(RouteNextHopEntry, toUnicastRouteNhopsNonEcmp) {
  folly::CIDRNetwork nw{folly::IPAddress{"1::1"}, 64};
  RouteNextHopSet nhops = makeNextHops({"1.1.1.10"});
  auto unicastRoute =
      util::toUnicastRoute(nw, RouteNextHopEntry(nhops, DISTANCE));

  EXPECT_EQ(
      nw,
      folly::CIDRNetwork(
          facebook::network::toIPAddress(*unicastRoute.dest_ref()->ip_ref()),
          *unicastRoute.dest_ref()->prefixLength_ref()));
  EXPECT_EQ(RouteForwardAction::NEXTHOPS, unicastRoute.action_ref());
  EXPECT_EQ(1, unicastRoute.nextHops_ref()->size());
}

TEST(RouteNextHopEntry, toUnicastRouteNhopsEcmp) {
  folly::CIDRNetwork nw{folly::IPAddress{"1::1"}, 64};
  RouteNextHopSet nhops = makeNextHops({"1.1.1.10", "2::2"});
  auto unicastRoute =
      util::toUnicastRoute(nw, RouteNextHopEntry(nhops, DISTANCE));

  EXPECT_EQ(
      nw,
      folly::CIDRNetwork(
          facebook::network::toIPAddress(*unicastRoute.dest_ref()->ip_ref()),
          *unicastRoute.dest_ref()->prefixLength_ref()));
  EXPECT_EQ(RouteForwardAction::NEXTHOPS, unicastRoute.action_ref());
  EXPECT_EQ(2, unicastRoute.nextHops_ref()->size());
}
