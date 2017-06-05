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
#include "fboss/agent/test/TestUtils.h"
#include "fboss/agent/hw/mock/MockPlatform.h"
#include "fboss/agent/state/DeltaFunctions.h"
#include "fboss/agent/state/Interface.h"
#include "fboss/agent/state/InterfaceMap.h"
#include "fboss/agent/state/Route.h"
#include "fboss/agent/state/RouteDelta.h"
#include "fboss/agent/state/RouteTableRib.h"
#include "fboss/agent/state/RouteTable.h"
#include "fboss/agent/state/RouteTableMap.h"
#include "fboss/agent/state/RouteUpdater.h"
#include "fboss/agent/state/NodeMapDelta.h"
#include "fboss/agent/state/NodeMapDelta-defs.h"
#include "fboss/agent/state/StateDelta.h"
#include "fboss/agent/state/StateUtils.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/state/SwitchState-defs.h"
#include "fboss/agent/gen-cpp2/switch_config_types.h"

#include <gtest/gtest.h>

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
template<typename AddrT>
void EXPECT_FWD_INFO(std::shared_ptr<Route<AddrT>> rt,
                     InterfaceID intf, std::string ipStr) {
  const auto& fwds = rt->getForwardInfo().getNextHopSet();
  EXPECT_EQ(1, fwds.size());
  const auto& fwd = *fwds.begin();
  EXPECT_EQ(intf, fwd.intf());
  EXPECT_EQ(IPAddress(ipStr), fwd.addr());
}

template<typename AddrT>
void EXPECT_RESOLVED(std::shared_ptr<Route<AddrT>> rt) {
  ASSERT_NE(nullptr, rt);
  EXPECT_TRUE(rt->isResolved());
  EXPECT_FALSE(rt->isUnresolvable());
  EXPECT_FALSE(rt->needResolve());
}

//
// Tests
//
#define CLIENT_A ClientID(1001)
#define CLIENT_B ClientID(1002)
#define CLIENT_C ClientID(1003)
#define CLIENT_D ClientID(1004)
#define CLIENT_E ClientID(1005)

TEST(RouteUpdater, dedup) {
  auto platform = createMockPlatform();
  auto stateV0 = make_shared<SwitchState>();
  auto tablesV0 = stateV0->getRouteTables();

  cfg::SwitchConfig config;
  config.vlans.resize(2);
  config.vlans[0].id = 1;
  config.vlans[1].id = 2;

  config.interfaces.resize(2);
  config.interfaces[0].intfID = 1;
  config.interfaces[0].vlanID = 1;
  config.interfaces[0].routerID = 0;
  config.interfaces[0].__isset.mac = true;
  config.interfaces[0].mac = "00:00:00:00:00:11";
  config.interfaces[0].ipAddresses.resize(2);
  config.interfaces[0].ipAddresses[0] = "1.1.1.1/24";
  config.interfaces[0].ipAddresses[1] = "1::1/48";
  config.interfaces[1].intfID = 2;
  config.interfaces[1].vlanID = 2;
  config.interfaces[1].routerID = 0;
  config.interfaces[1].__isset.mac = true;
  config.interfaces[1].mac = "00:00:00:00:00:22";
  config.interfaces[1].ipAddresses.resize(2);
  config.interfaces[1].ipAddresses[0] = "2.2.2.2/24";
  config.interfaces[1].ipAddresses[1] = "2::1/48";

  auto stateV1 = publishAndApplyConfig(stateV0, &config, platform.get());
  ASSERT_NE(nullptr, stateV1);
  stateV1->publish();
  auto rid = RouterID(0);
  // 2 different nexthops
  RouteNextHops nhop1 = makeNextHops({"1.1.1.10"}); // resolved by intf 1
  RouteNextHops nhop2 = makeNextHops({"2.2.2.10"}); // resolved by intf 2
  // 4 prefixes
  RouteV4::Prefix r1{IPAddressV4("10.1.1.0"), 24};
  RouteV4::Prefix r2{IPAddressV4("20.1.1.0"), 24};
  RouteV6::Prefix r3{IPAddressV6("1001::0"), 48};
  RouteV6::Prefix r4{IPAddressV6("2001::0"), 48};

  const auto& tables1 = stateV1->getRouteTables();
  RouteUpdater u2(tables1);
  u2.addRoute(rid, r1.network, r1.mask, CLIENT_A, RouteNextHopEntry(nhop1));
  u2.addRoute(rid, r2.network, r2.mask, CLIENT_A, RouteNextHopEntry(nhop2));
  u2.addRoute(rid, r3.network, r3.mask, CLIENT_A, RouteNextHopEntry(nhop1));
  u2.addRoute(rid, r4.network, r4.mask, CLIENT_A, RouteNextHopEntry(nhop2));
  auto tables2 = u2.updateDone();
  ASSERT_NE(nullptr, tables2);
  tables2->publish();

  // Re-add the same routes; expect no change
  RouteUpdater u3(tables2);
  u3.addInterfaceAndLinkLocalRoutes(stateV1->getInterfaces());
  u3.addRoute(rid, r1.network, r1.mask, CLIENT_A, RouteNextHopEntry(nhop1));
  u3.addRoute(rid, r2.network, r2.mask, CLIENT_A, RouteNextHopEntry(nhop2));
  u3.addRoute(rid, r3.network, r3.mask, CLIENT_A, RouteNextHopEntry(nhop1));
  u3.addRoute(rid, r4.network, r4.mask, CLIENT_A, RouteNextHopEntry(nhop2));
  auto tables3 = u3.updateDone();
  EXPECT_EQ(nullptr, tables3);

  // Re-add the same routes, except for one difference.  Expect an update.
  RouteUpdater u4(tables2);
  u4.addInterfaceAndLinkLocalRoutes(stateV1->getInterfaces());
  u4.addRoute(rid, r1.network, r1.mask, CLIENT_A, RouteNextHopEntry(nhop1));
  u4.addRoute(rid, r2.network, r2.mask, CLIENT_A, RouteNextHopEntry(nhop1));
  u4.addRoute(rid, r3.network, r3.mask, CLIENT_A, RouteNextHopEntry(nhop1));
  u4.addRoute(rid, r4.network, r4.mask, CLIENT_A, RouteNextHopEntry(nhop2));
  auto tables4 = u4.updateDone();
  ASSERT_NE(nullptr, tables4);
  tables4->publish();

  // get all 4 routes from table2
  auto t2r1 = GET_ROUTE_V4(tables2, rid, r1);
  auto t2r2 = GET_ROUTE_V4(tables2, rid, r2);
  auto t2r3 = GET_ROUTE_V6(tables2, rid, r3);
  auto t2r4 = GET_ROUTE_V6(tables2, rid, r4);

  // get all 4 routes from table4
  auto t4r1 = GET_ROUTE_V4(tables4, rid, r1);
  auto t4r2 = GET_ROUTE_V4(tables4, rid, r2);
  auto t4r3 = GET_ROUTE_V6(tables4, rid, r3);
  auto t4r4 = GET_ROUTE_V6(tables4, rid, r4);

  EXPECT_EQ(t2r1, t4r1);
  EXPECT_NE(t2r2, t4r2);        // different routes
  EXPECT_EQ(t2r2->getGeneration() + 1, t4r2->getGeneration());
  EXPECT_EQ(t2r3, t4r3);
  EXPECT_EQ(t2r4, t4r4);
}

TEST(Route, resolve) {
  auto platform = createMockPlatform();
  auto stateV0 = make_shared<SwitchState>();
  auto tablesV0 = stateV0->getRouteTables();

  cfg::SwitchConfig config;
  config.vlans.resize(2);
  config.vlans[0].id = 1;
  config.vlans[1].id = 2;

  config.interfaces.resize(2);
  config.interfaces[0].intfID = 1;
  config.interfaces[0].vlanID = 1;
  config.interfaces[0].routerID = 0;
  config.interfaces[0].__isset.mac = true;
  config.interfaces[0].mac = "00:00:00:00:00:11";
  config.interfaces[0].ipAddresses.resize(2);
  config.interfaces[0].ipAddresses[0] = "1.1.1.1/24";
  config.interfaces[0].ipAddresses[1] = "1::1/48";
  config.interfaces[1].intfID = 2;
  config.interfaces[1].vlanID = 2;
  config.interfaces[1].routerID = 0;
  config.interfaces[1].__isset.mac = true;
  config.interfaces[1].mac = "00:00:00:00:00:22";
  config.interfaces[1].ipAddresses.resize(2);
  config.interfaces[1].ipAddresses[0] = "2.2.2.2/24";
  config.interfaces[1].ipAddresses[1] = "2::1/48";

  auto stateV1 = publishAndApplyConfig(stateV0, &config, platform.get());
  stateV1->publish();
  ASSERT_NE(nullptr, stateV1);

  auto rid = RouterID(0);
  // recursive lookup
  {
    RouteUpdater u1(stateV1->getRouteTables());
    RouteNextHops nexthops1 = makeNextHops({"1.1.1.10"}); // resolved by intf 1
    u1.addRoute(rid, IPAddress("1.1.3.0"), 24, CLIENT_A,
                RouteNextHopEntry(nexthops1));
    RouteNextHops nexthops2 = makeNextHops({"1.1.3.10"}); //rslvd. by '1.1.3/24'
    u1.addRoute(rid, IPAddress("8.8.8.0"), 24, CLIENT_A,
                RouteNextHopEntry(nexthops2));
    auto tables2 = u1.updateDone();
    ASSERT_NE(nullptr, tables2);
    tables2->publish();

    auto r21 = GET_ROUTE_V4(tables2, rid, "1.1.3.0/24");
    EXPECT_RESOLVED(r21);
    EXPECT_FALSE(r21->isConnected());

    auto r22 = GET_ROUTE_V4(tables2, rid, "8.8.8.0/24");
    EXPECT_RESOLVED(r22);
    EXPECT_FALSE(r22->isConnected());
    // r21 and r22 are different routes
    EXPECT_NE(r21, r22);
    EXPECT_NE(r21->prefix(), r22->prefix());
    // check the forwarding info
    RouteNextHopSet expFwd2;
    expFwd2.emplace(RouteNextHop::createForward(
                        IPAddress("1.1.1.10"), InterfaceID(1)));
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
    RouteUpdater u1(stateV1->getRouteTables());
    u1.addRoute(rid, IPAddress("30.0.0.0"), 8,
                CLIENT_A, RouteNextHopEntry(makeNextHops({"20.1.1.1"})));
    u1.addRoute(rid, IPAddress("20.0.0.0"), 8,
                CLIENT_A, RouteNextHopEntry(makeNextHops({"10.1.1.1"})));
    u1.addRoute(rid, IPAddress("10.0.0.0"), 8,
                CLIENT_A, RouteNextHopEntry(makeNextHops({"30.1.1.1"})));
    auto tables2 = u1.updateDone();
    ASSERT_NE(nullptr, tables2);
    tables2->publish();

    auto verifyPrefix = [&](std::string prefixStr) {
      auto route = GET_ROUTE_V4(tables2, rid, prefixStr);
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
    RouteUpdater u1(stateV1->getRouteTables());
    RouteNextHops nexthops1 = makeNextHops({"50.0.0.1"});
    u1.addRoute(rid, IPAddress("40.0.0.0"), 8, CLIENT_A,
                RouteNextHopEntry(nexthops1));

    auto tables2 = u1.updateDone();
    ASSERT_NE(nullptr, tables2);
    tables2->publish();

    // 40.0.0.0/8 should be unresolved
    auto r21 = GET_ROUTE_V4(tables2, rid, "40.0.0.0/8");
    EXPECT_FALSE(r21->isResolved());
    EXPECT_TRUE(r21->isUnresolvable());
    EXPECT_FALSE(r21->isConnected());
    EXPECT_FALSE(r21->needResolve());

    // Resolve 50.0.0.1 this should also resolve 40.0.0.0/8
    RouteUpdater u2(tables2);
    u2.addRoute(rid, IPAddress("50.0.0.0"), 8,
                CLIENT_A, RouteNextHopEntry(makeNextHops({"1.1.1.1"})));
    auto tables3 = u2.updateDone();
    ASSERT_NE(nullptr, tables3);
    tables3->publish();

    // 40.0.0.0/8 should be resolved
    auto rib3 = tables3->getRouteTableIf(rid)->getRibV4();
    auto r31 = GET_ROUTE_V4(tables3, rid, "40.0.0.0/8");
    EXPECT_RESOLVED(r31);
    EXPECT_FALSE(r31->isConnected());

    // 50.0.0.1/32 should be resolved
    auto r31NextHops = r31->getBestEntry().second->getNextHopSet();
    EXPECT_EQ(1, r31NextHops.size());
    auto r32 = rib3->longestMatch(r31NextHops.begin()->addr().asV4());
    EXPECT_RESOLVED(r32);
    EXPECT_FALSE(r32->isConnected());

    // 50.0.0.0/8 should be resolved
    auto r33 = GET_ROUTE_V4(tables3, rid, "50.0.0.0/8");
    EXPECT_RESOLVED(r33);
    EXPECT_FALSE(r33->isConnected());

  }
}

// Testing add and delete ECMP routes
TEST(Route, addDel) {
  auto platform = createMockPlatform();
  auto stateV0 = make_shared<SwitchState>();
  auto tablesV0 = stateV0->getRouteTables();

  cfg::SwitchConfig config;
  config.vlans.resize(2);
  config.vlans[0].id = 1;
  config.vlans[1].id = 2;

  config.interfaces.resize(2);
  config.interfaces[0].intfID = 1;
  config.interfaces[0].vlanID = 1;
  config.interfaces[0].routerID = 0;
  config.interfaces[0].__isset.mac = true;
  config.interfaces[0].mac = "00:00:00:00:00:11";
  config.interfaces[0].ipAddresses.resize(2);
  config.interfaces[0].ipAddresses[0] = "1.1.1.1/24";
  config.interfaces[0].ipAddresses[1] = "1::1/48";
  config.interfaces[1].intfID = 2;
  config.interfaces[1].vlanID = 2;
  config.interfaces[1].routerID = 0;
  config.interfaces[1].__isset.mac = true;
  config.interfaces[1].mac = "00:00:00:00:00:22";
  config.interfaces[1].ipAddresses.resize(2);
  config.interfaces[1].ipAddresses[0] = "2.2.2.2/24";
  config.interfaces[1].ipAddresses[1] = "2::1/48";

  auto stateV1 = publishAndApplyConfig(stateV0, &config, platform.get());
  ASSERT_NE(nullptr, stateV1);
  stateV1->publish();

  auto rid = RouterID(0);

  RouteNextHops nexthops = makeNextHops({"1.1.1.10",    // intf 1
                                         "2::2",        // intf 2
                                         "1.1.2.10"});  // un-resolvable
  RouteNextHops nexthops2 = makeNextHops({"1.1.3.10",   // un-resolvable
                                          "11:11::1"}); // un-resolvable

  RouteUpdater u1(stateV1->getRouteTables());
  u1.addRoute(rid, IPAddress("10.1.1.1"), 24, CLIENT_A,
              RouteNextHopEntry(nexthops));
  u1.addRoute(rid, IPAddress("2001::1"), 48, CLIENT_A,
              RouteNextHopEntry(nexthops));
  auto tables2 = u1.updateDone();
  ASSERT_NE(nullptr, tables2);
  tables2->publish();

  // v4 route
  auto r2 = GET_ROUTE_V4(tables2, rid, "10.1.1.0/24");
  EXPECT_RESOLVED(r2);
  EXPECT_FALSE(r2->isDrop());
  EXPECT_FALSE(r2->isToCPU());
  EXPECT_FALSE(r2->isConnected());
  // v6 route
  auto r2v6 = GET_ROUTE_V6(tables2, rid, "2001::0/48");
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
  expFwd2.emplace(RouteNextHop::createForward(
                      IPAddress("1.1.1.10"), InterfaceID(1)));
  expFwd2.emplace(RouteNextHop::createForward(
                      IPAddress("2::2"), InterfaceID(2)));
  EXPECT_EQ(expFwd2, fwd2);
  EXPECT_EQ(expFwd2, fwd2v6);

  // change the nexthops of the V4 route
  RouteUpdater u2(tables2);
  u2.addRoute(rid, IPAddress("10.1.1.1"), 24,
              CLIENT_A, RouteNextHopEntry(nexthops2));
  auto tables3 = u2.updateDone();
  ASSERT_NE(nullptr, tables3);
  tables3->publish();

  auto r3 = GET_ROUTE_V4(tables3, rid, "10.1.1.0/24");
  ASSERT_NE(nullptr, r3);
  EXPECT_FALSE(r3->isResolved());
  EXPECT_TRUE(r3->isUnresolvable());
  EXPECT_FALSE(r3->isConnected());
  EXPECT_FALSE(r3->needResolve());

  // re-add the same route does not cause change
  RouteUpdater u3(tables3);
  u3.addRoute(rid, IPAddress("10.1.1.1"), 24, CLIENT_A,
              RouteNextHopEntry(nexthops2));
  auto tables4 = u3.updateDone();
  EXPECT_EQ(nullptr, tables4);

  // now delete the V4 route
  RouteUpdater u4(tables3);
  u4.delRoute(rid, IPAddress("10.1.1.1"), 24, CLIENT_A);
  auto tables5 = u4.updateDone();
  ASSERT_NE(nullptr, tables5);
  tables5->publish();

  auto rib5 = tables5->getRouteTableIf(rid)->getRibV4();
  auto r5 = rib5->exactMatch({IPAddressV4("10.1.1.0"), 24});
  EXPECT_EQ(nullptr, r5);

  // change an old route to punt to CPU, add a new route to DROP
  RouteUpdater u5(tables3);
  u5.addRoute(rid, IPAddress("10.1.1.0"), 24,
              CLIENT_A, RouteNextHopEntry(RouteForwardAction::TO_CPU));
  u5.addRoute(rid, IPAddress("10.1.2.0"), 24,
              CLIENT_A, RouteNextHopEntry(RouteForwardAction::DROP));
  auto tables6 = u5.updateDone();

  auto r6_1 = GET_ROUTE_V4(tables6, rid, "10.1.1.0/24");
  EXPECT_RESOLVED(r6_1);
  EXPECT_FALSE(r6_1->isConnected());
  EXPECT_TRUE(r6_1->isToCPU());
  EXPECT_FALSE(r6_1->isDrop());
  EXPECT_EQ(RouteForwardAction::TO_CPU, r6_1->getForwardInfo().getAction());

  auto r6_2 = GET_ROUTE_V4(tables6, rid, "10.1.2.0/24");
  EXPECT_RESOLVED(r6_2);
  EXPECT_FALSE(r6_2->isConnected());
  EXPECT_FALSE(r6_2->isToCPU());
  EXPECT_TRUE(r6_2->isDrop());
  EXPECT_EQ(RouteForwardAction::DROP, r6_2->getForwardInfo().getAction());
}

// Test interface routes
TEST(Route, Interface) {
  auto platform = createMockPlatform();
  RouterID rid = RouterID(0);
  auto stateV0 = make_shared<SwitchState>();
  auto tablesV0 = stateV0->getRouteTables();

  cfg::SwitchConfig config;
  config.vlans.resize(2);
  config.vlans[0].id = 1;
  config.vlans[1].id = 2;

  config.interfaces.resize(2);
  config.interfaces[0].intfID = 1;
  config.interfaces[0].vlanID = 1;
  config.interfaces[0].routerID = 0;
  config.interfaces[0].__isset.mac = true;
  config.interfaces[0].mac = "00:00:00:00:00:11";
  config.interfaces[0].ipAddresses.resize(2);
  config.interfaces[0].ipAddresses[0] = "1.1.1.1/24";
  config.interfaces[0].ipAddresses[1] = "1::1/48";
  config.interfaces[1].intfID = 2;
  config.interfaces[1].vlanID = 2;
  config.interfaces[1].routerID = 0;
  config.interfaces[1].__isset.mac = true;
  config.interfaces[1].mac = "00:00:00:00:00:22";
  config.interfaces[1].ipAddresses.resize(2);
  config.interfaces[1].ipAddresses[0] = "2.2.2.2/24";
  config.interfaces[1].ipAddresses[1] = "2::1/48";

  auto stateV1 = publishAndApplyConfig(stateV0, &config, platform.get());
  ASSERT_NE(nullptr, stateV1);
  stateV1->publish();
  auto tablesV1 = stateV1->getRouteTables();
  EXPECT_NE(tablesV0, tablesV1);
  EXPECT_EQ(1, tablesV1->getGeneration());
  EXPECT_EQ(1, tablesV1->size());
  EXPECT_EQ(2, tablesV1->getRouteTableIf(rid)->getRibV4()->size());
  EXPECT_EQ(3, tablesV1->getRouteTableIf(rid)->getRibV6()->size());
  // verify the ipv4 interface route
  {
    auto rt = GET_ROUTE_V4(tablesV1, rid, "1.1.1.0/24");
    EXPECT_EQ(0, rt->getGeneration());
    EXPECT_RESOLVED(rt);
    EXPECT_TRUE(rt->isConnected());
    EXPECT_FALSE(rt->isToCPU());
    EXPECT_FALSE(rt->isDrop());
    EXPECT_EQ(RouteForwardAction::NEXTHOPS, rt->getForwardInfo().getAction());
    EXPECT_FWD_INFO(rt, InterfaceID(1), "1.1.1.1");
  }
  // verify the ipv6 interface route
  {
    auto rt = GET_ROUTE_V6(tablesV1, rid, "2::0/48");
    EXPECT_EQ(0, rt->getGeneration());
    EXPECT_RESOLVED(rt);
    EXPECT_TRUE(rt->isConnected());
    EXPECT_FALSE(rt->isToCPU());
    EXPECT_FALSE(rt->isDrop());
    EXPECT_EQ(RouteForwardAction::NEXTHOPS, rt->getForwardInfo().getAction());
    EXPECT_FWD_INFO(rt, InterfaceID(2), "2::1");
  }

  {
    // verify v6 link local route
    auto rt = GET_ROUTE_V6(tablesV1, rid, "fe80::/64");
    EXPECT_EQ(0, rt->getGeneration());
    EXPECT_RESOLVED(rt);
    EXPECT_FALSE(rt->isConnected());
    EXPECT_TRUE(rt->isToCPU());
    EXPECT_EQ(RouteForwardAction::TO_CPU, rt->getForwardInfo().getAction());
    const auto& fwds = rt->getForwardInfo().getNextHopSet();
    EXPECT_EQ(0, fwds.size());
  }

  // swap the interface addresses which causes route change
  config.interfaces[1].ipAddresses[0] = "1.1.1.1/24";
  config.interfaces[1].ipAddresses[1] = "1::1/48";
  config.interfaces[0].ipAddresses[0] = "2.2.2.2/24";
  config.interfaces[0].ipAddresses[1] = "2::1/48";

  auto stateV2 = publishAndApplyConfig(stateV1, &config, platform.get());
  ASSERT_NE(nullptr, stateV2);
  stateV2->publish();
  auto tablesV2 = stateV2->getRouteTables();
  EXPECT_NE(tablesV1, tablesV2);
  EXPECT_EQ(2, tablesV2->getGeneration());
  EXPECT_EQ(1, tablesV2->size());
  EXPECT_EQ(2, tablesV2->getRouteTableIf(rid)->getRibV4()->size());
  EXPECT_EQ(3, tablesV2->getRouteTableIf(rid)->getRibV6()->size());

  {
    auto rib4 = tablesV1->getRouteTableIf(rid)->getRibV4();
    auto rib6 = tablesV1->getRouteTableIf(rid)->getRibV6();
    auto rib4V2 = tablesV2->getRouteTableIf(rid)->getRibV4();
    auto rib6V2 = tablesV2->getRouteTableIf(rid)->getRibV6();
    EXPECT_NE(rib4, rib4V2);
    EXPECT_NE(rib6, rib6V2);
  }
  // verify the ipv4 route
  {
    auto rt = GET_ROUTE_V4(tablesV2, rid, "1.1.1.0/24");
    EXPECT_EQ(1, rt->getGeneration());
    EXPECT_FWD_INFO(rt, InterfaceID(2), "1.1.1.1");
  }
  // verify the ipv6 route
  {
    auto rt = GET_ROUTE_V6(tablesV2, rid, "2::0/48");
    EXPECT_EQ(1, rt->getGeneration());
    EXPECT_FWD_INFO(rt, InterfaceID(1), "2::1");
  }
}

// Test interface routes when we have more than one address per
// address family in an interface
TEST(Route, MultipleAddressInterface) {
  auto platform = createMockPlatform();
  auto rid = RouterID(0);
  auto stateV0 = make_shared<SwitchState>();
  auto tablesV0 = stateV0->getRouteTables();

  cfg::SwitchConfig config;
  config.vlans.resize(1);
  config.vlans[0].id = 1;

  config.interfaces.resize(1);
  config.interfaces[0].intfID = 1;
  config.interfaces[0].vlanID = 1;
  config.interfaces[0].routerID = 0;
  config.interfaces[0].__isset.mac = true;
  config.interfaces[0].mac = "00:00:00:00:00:11";
  config.interfaces[0].ipAddresses.resize(4);
  config.interfaces[0].ipAddresses[0] = "1.1.1.1/24";
  config.interfaces[0].ipAddresses[1] = "1.1.1.2/24";
  config.interfaces[0].ipAddresses[2] = "1::1/48";
  config.interfaces[0].ipAddresses[3] = "1::2/48";

  auto stateV1 = publishAndApplyConfig(stateV0, &config, platform.get());
  ASSERT_NE(nullptr, stateV1);
  stateV1->publish();
  auto tablesV1 = stateV1->getRouteTables();
  EXPECT_NE(tablesV0, tablesV1);
  EXPECT_EQ(1, tablesV1->getGeneration());
  EXPECT_EQ(1, tablesV1->size());
  EXPECT_EQ(1, tablesV1->getRouteTableIf(rid)->getRibV4()->size());
  EXPECT_EQ(2, tablesV1->getRouteTableIf(rid)->getRibV6()->size());
  // verify the ipv4 route
  {
    auto rt = GET_ROUTE_V4(tablesV1, rid, "1.1.1.0/24");
    EXPECT_EQ(0, rt->getGeneration());
    EXPECT_RESOLVED(rt);
    EXPECT_TRUE(rt->isConnected());
    EXPECT_FALSE(rt->isToCPU());
    EXPECT_FALSE(rt->isDrop());
    EXPECT_EQ(RouteForwardAction::NEXTHOPS, rt->getForwardInfo().getAction());
    EXPECT_FWD_INFO(rt, InterfaceID(1), "1.1.1.2");
  }
  // verify the ipv6 route
  {
    auto rt = GET_ROUTE_V6(tablesV1, rid, "1::0/48");
    EXPECT_EQ(0, rt->getGeneration());
    EXPECT_RESOLVED(rt);
    EXPECT_TRUE(rt->isConnected());
    EXPECT_FALSE(rt->isToCPU());
    EXPECT_FALSE(rt->isDrop());
    EXPECT_EQ(RouteForwardAction::NEXTHOPS, rt->getForwardInfo().getAction());
    EXPECT_FWD_INFO(rt, InterfaceID(1), "1::2");
  }
}

namespace TEMP {
struct Route {
  uint32_t vrf;
  IPAddress prefix;
  uint8_t len;
  Route(uint32_t vrf, IPAddress prefix, uint8_t len)
      : vrf(vrf), prefix(prefix), len(len) {}
  bool operator<(const Route& rt) const {
    if (vrf < rt.vrf) {
      return true;
    } else if (vrf > rt.vrf) {
      return false;
    }
    if (len < rt.len) {
      return true;
    } else if (len > rt.len) {
      return false;
    }
    return prefix < rt.prefix;
  }
  bool operator==(const Route& rt) const {
    return vrf == rt.vrf && len == rt.len && prefix == rt.prefix;
  }
};
}

void checkChangedRoute(const shared_ptr<RouteTableMap>& oldTables,
                       const shared_ptr<RouteTableMap>& newTables,
                       const std::set<TEMP::Route> changedIDs,
                       const std::set<TEMP::Route> addedIDs,
                       const std::set<TEMP::Route> removedIDs) {
  auto oldState = make_shared<SwitchState>();
  oldState->resetRouteTables(oldTables);
  auto newState = make_shared<SwitchState>();
  newState->resetRouteTables(newTables);

  std::set<TEMP::Route> foundChanged;
  std::set<TEMP::Route> foundAdded;
  std::set<TEMP::Route> foundRemoved;
  StateDelta delta(oldState, newState);

  for (auto const& rtDelta : delta.getRouteTablesDelta()) {
    RouterID id;
    if (!rtDelta.getOld()) {
      id = rtDelta.getNew()->getID();
    } else {
      id = rtDelta.getOld()->getID();
    }
    DeltaFunctions::forEachChanged(
        rtDelta.getRoutesV4Delta(),
        [&] (const shared_ptr<RouteV4>& oldRt,
             const shared_ptr<RouteV4>& newRt) {
          EXPECT_EQ(oldRt->prefix(), newRt->prefix());
          EXPECT_NE(oldRt, newRt);
          const auto prefix = newRt->prefix();
          auto ret = foundChanged.insert(
              TEMP::Route(id, IPAddress(prefix.network), prefix.mask));
          EXPECT_TRUE(ret.second);
        },
        [&] (const shared_ptr<RouteV4>& rt) {
          const auto prefix = rt->prefix();
          auto ret = foundAdded.insert(
              TEMP::Route(id, IPAddress(prefix.network), prefix.mask));
          EXPECT_TRUE(ret.second);
        },
        [&] (const shared_ptr<RouteV4>& rt) {
          const auto prefix = rt->prefix();
          auto ret = foundRemoved.insert(
              TEMP::Route(id, IPAddress(prefix.network), prefix.mask));
          EXPECT_TRUE(ret.second);
        });
    DeltaFunctions::forEachChanged(
        rtDelta.getRoutesV6Delta(),
        [&] (const shared_ptr<RouteV6>& oldRt,
             const shared_ptr<RouteV6>& newRt) {
          EXPECT_EQ(oldRt->prefix(), newRt->prefix());
          EXPECT_NE(oldRt, newRt);
          const auto prefix = newRt->prefix();
          auto ret = foundChanged.insert(
              TEMP::Route(id, IPAddress(prefix.network), prefix.mask));
          EXPECT_TRUE(ret.second);
        },
        [&] (const shared_ptr<RouteV6>& rt) {
          const auto prefix = rt->prefix();
          auto ret = foundAdded.insert(
              TEMP::Route(id, IPAddress(prefix.network), prefix.mask));
          EXPECT_TRUE(ret.second);
        },
        [&] (const shared_ptr<RouteV6>& rt) {
          const auto prefix = rt->prefix();
          auto ret = foundRemoved.insert(
              TEMP::Route(id, IPAddress(prefix.network), prefix.mask));
          EXPECT_TRUE(ret.second);
        });
  }

  EXPECT_EQ(changedIDs, foundChanged);
  EXPECT_EQ(addedIDs, foundAdded);
  EXPECT_EQ(removedIDs, foundRemoved);
}

void checkChangedRouteTable(const shared_ptr<RouteTableMap>& oldTables,
                            const shared_ptr<RouteTableMap>& newTables,
                            const std::set<uint32_t> changedIDs,
                            const std::set<uint32_t> addedIDs,
                            const std::set<uint32_t> removedIDs) {
  auto oldState = make_shared<SwitchState>();
  oldState->resetRouteTables(oldTables);
  auto newState = make_shared<SwitchState>();
  newState->resetRouteTables(newTables);

  std::set<uint32_t> foundChanged;
  std::set<uint32_t> foundAdded;
  std::set<uint32_t> foundRemoved;
  StateDelta delta(oldState, newState);
  DeltaFunctions::forEachChanged(
      delta.getRouteTablesDelta(),
      [&] (const shared_ptr<RouteTable>& oldTable,
           const shared_ptr<RouteTable>& newTable) {
        EXPECT_EQ(oldTable->getID(), newTable->getID());
        EXPECT_NE(oldTable, newTable);
        auto ret = foundChanged.insert(oldTable->getID());
        EXPECT_TRUE(ret.second);
      },
      [&] (const shared_ptr<RouteTable>& table) {
        auto ret = foundAdded.insert(table->getID());
        EXPECT_TRUE(ret.second);
      },
      [&] (const shared_ptr<RouteTable>& table) {
        auto ret = foundRemoved.insert(table->getID());
        EXPECT_TRUE(ret.second);
      });

  EXPECT_EQ(changedIDs, foundChanged);
  EXPECT_EQ(addedIDs, foundAdded);
  EXPECT_EQ(removedIDs, foundRemoved);
}

TEST(RouteTableMap, applyConfig) {
  auto platform = createMockPlatform();
  auto stateV0 = make_shared<SwitchState>();
  auto tablesV0 = stateV0->getRouteTables();

  cfg::SwitchConfig config;
  config.vlans.resize(2);
  config.vlans[0].id = 1;
  config.vlans[1].id = 2;
  config.interfaces.resize(2);
  config.interfaces[0].intfID = 1;
  config.interfaces[0].vlanID = 1;
  config.interfaces[0].routerID = 0;
  config.interfaces[0].__isset.mac = true;
  config.interfaces[0].mac = "00:00:00:00:00:11";
  config.interfaces[1].intfID = 2;
  config.interfaces[1].vlanID = 2;
  config.interfaces[1].routerID = 1;
  config.interfaces[1].__isset.mac = true;
  config.interfaces[1].mac = "00:00:00:00:00:22";

  auto stateV1 = publishAndApplyConfig(stateV0, &config, platform.get());
  ASSERT_NE(nullptr, stateV1);
  stateV1->publish();
  auto tablesV1 = stateV1->getRouteTables();
  EXPECT_EQ(tablesV0, tablesV1);
  EXPECT_EQ(0, tablesV1->getGeneration());
  EXPECT_EQ(0, tablesV1->size());

  config.interfaces[0].ipAddresses.resize(4);
  config.interfaces[0].ipAddresses[0] = "1.1.1.1/24";
  config.interfaces[0].ipAddresses[1] = "1.1.1.2/24";
  config.interfaces[0].ipAddresses[2] = "1.1.1.10/24";
  config.interfaces[0].ipAddresses[3] = "::1/48";
  config.interfaces[1].ipAddresses.resize(2);
  config.interfaces[1].ipAddresses[0] = "1.1.1.1/24";
  config.interfaces[1].ipAddresses[1] = "::1/48";

  auto stateV2 = publishAndApplyConfig(stateV1, &config, platform.get());
  ASSERT_NE(nullptr, stateV2);
  stateV2->publish();
  auto tablesV2 = stateV2->getRouteTables();
  EXPECT_NE(tablesV1, tablesV2);
  EXPECT_EQ(1, tablesV2->getGeneration());
  EXPECT_EQ(2, tablesV2->size());
  EXPECT_NE(nullptr, tablesV2->getRouteTable(RouterID(0)));
  EXPECT_NE(nullptr, tablesV2->getRouteTable(RouterID(1)));

  checkChangedRouteTable(tablesV1, tablesV2, {}, {0,1}, {});
  checkChangedRoute(tablesV1, tablesV2,
                    {},
                    {
                      TEMP::Route{0, IPAddress("1.1.1.0"), 24},
                      TEMP::Route{0, IPAddress("::0"), 48},
                      TEMP::Route{0, IPAddress("fe80::"), 64},
                      TEMP::Route{1, IPAddress("1.1.1.0"), 24},
                      TEMP::Route{1, IPAddress("::0"), 48},
                      TEMP::Route{1, IPAddress("fe80::"), 64},
                    },
                    {});

  // change an interface address
  config.interfaces[0].ipAddresses[3] = "11::11/48";

  auto stateV3 = publishAndApplyConfig(stateV2, &config, platform.get());
  ASSERT_NE(nullptr, stateV3);
  stateV3->publish();
  auto tablesV3 = stateV3->getRouteTables();
  EXPECT_NE(tablesV2, tablesV3);
  EXPECT_EQ(2, tablesV3->getGeneration());
  EXPECT_EQ(2, tablesV3->size());
  EXPECT_NE(nullptr, tablesV2->getRouteTable(RouterID(0)));
  EXPECT_NE(nullptr, tablesV2->getRouteTable(RouterID(1)));

  checkChangedRouteTable(tablesV2, tablesV3, {0}, {}, {});
  checkChangedRoute(tablesV2, tablesV3,
                    {},
                    {TEMP::Route{0, IPAddress("11::0"), 48}},
                    {TEMP::Route{0, IPAddress("::0"), 48}});

  // move one interface to cause same route prefix conflict
  config.interfaces[1].routerID = 0;
  EXPECT_THROW(
    publishAndApplyConfig(stateV3, &config, platform.get()), FbossError);

  // add a new interface in a new VRF
  config.vlans.resize(3);
  config.vlans[2].id = 3;
  config.interfaces.resize(3);
  config.interfaces[2].intfID = 3;
  config.interfaces[2].vlanID = 3;
  config.interfaces[2].routerID = 2;
  config.interfaces[2].__isset.mac = true;
  config.interfaces[2].mac = "00:00:00:00:00:33";
  config.interfaces[2].ipAddresses.resize(2);
  config.interfaces[2].ipAddresses[0] = "1.1.1.1/24";
  config.interfaces[2].ipAddresses[1] = "::1/48";
  // and move one interface to another vrf and fix the address conflict
  config.interfaces[1].routerID = 0;
  config.interfaces[1].ipAddresses.resize(2);
  config.interfaces[1].ipAddresses[0] = "2.2.2.1/24";
  config.interfaces[1].ipAddresses[1] = "1::2/48";

  auto stateV4 = publishAndApplyConfig(stateV3, &config, platform.get());
  ASSERT_NE(nullptr, stateV4);
  stateV4->publish();
  auto tablesV4 = stateV4->getRouteTables();
  EXPECT_NE(tablesV3, tablesV4);
  EXPECT_EQ(3, tablesV4->getGeneration());
  EXPECT_EQ(2, tablesV4->size());
  EXPECT_NE(nullptr, tablesV4->getRouteTable(RouterID(0)));
  EXPECT_EQ(nullptr, tablesV4->getRouteTableIf(RouterID(1)));
  EXPECT_NE(nullptr, tablesV4->getRouteTable(RouterID(2)));

  checkChangedRouteTable(tablesV3, tablesV4, {0}, {2}, {1});
  checkChangedRoute(tablesV3, tablesV4,
                    {},
                    {
                      TEMP::Route{0, IPAddress("2.2.2.0"), 24},
                      TEMP::Route{0, IPAddress("1::0"), 48},
                      TEMP::Route{2, IPAddress("1.1.1.0"), 24},
                      TEMP::Route{2, IPAddress("::0"), 48},
                      TEMP::Route{2, IPAddress("fe80::"), 64},
                    },
                    {
                      TEMP::Route{1, IPAddress("1.1.1.0"), 24},
                      TEMP::Route{1, IPAddress("::0"), 48},
                      TEMP::Route{1, IPAddress("fe80::"), 64},
                    });

  // re-apply the same configure generates no change
  EXPECT_EQ(nullptr, publishAndApplyConfig(stateV4, &config, platform.get()));
}

TEST(Route, changedRoutesPostUpdate) {
  auto platform = createMockPlatform();
  auto stateV0 = make_shared<SwitchState>();
  auto tablesV0 = stateV0->getRouteTables();

  cfg::SwitchConfig config;
  config.vlans.resize(1);
  config.vlans[0].id = 1;

  config.interfaces.resize(1);
  config.interfaces[0].intfID = 1;
  config.interfaces[0].vlanID = 1;
  config.interfaces[0].routerID = 0;
  config.interfaces[0].__isset.mac = true;
  config.interfaces[0].mac = "00:00:00:00:00:11";
  config.interfaces[0].ipAddresses.resize(2);
  config.interfaces[0].ipAddresses[0] = "1.1.1.1/24";
  config.interfaces[0].ipAddresses[1] = "1::1/48";

  auto stateV1 = publishAndApplyConfig(stateV0, &config, platform.get());
  ASSERT_NE(nullptr, stateV1);
  stateV1->publish();
  auto rid = RouterID(0);
  RouteNextHops nexthops = makeNextHops({"1.1.1.10", // resolved by intf 1
                                         "2::2"});  // resolved by intf 2

  auto numChangedRoutes = [=] (const RTMapDelta& delta) {
    auto cnt = 0;
    for (auto itr = delta.begin(); itr != delta.end(); ++itr) {
      const auto& v4Delta = itr->getRoutesV4Delta();
      const auto& v6Delta = itr->getRoutesV6Delta();
      for (auto v4Itr = v4Delta.begin(); v4Itr != v4Delta.end();
          ++v4Itr, ++cnt);
      for (auto v6Itr = v6Delta.begin(); v6Itr != v6Delta.end();
          ++v6Itr, ++cnt);
    }
    return cnt;
  };
  // Add a couple of routes
  auto tables1 = stateV1->getRouteTables();
  stateV1->publish();
  RouteUpdater u1(tables1);
  u1.addRoute(rid, IPAddress("10.1.1.0"), 24,
              CLIENT_A, RouteNextHopEntry(nexthops));
  u1.addRoute(rid, IPAddress("2001::0"), 48,
              CLIENT_A, RouteNextHopEntry(nexthops));
  auto tables2 = u1.updateDone();
  ASSERT_NE(nullptr, tables2);
  auto t2 = tables2->getRouteTableIf(rid);
  ASSERT_NE(nullptr, t2);
  // v4 route
  auto rib2v4 = t2->getRibV4();
  RouteV4::Prefix p2{IPAddressV4("10.1.1.0"), 24};
  auto r2 = rib2v4->exactMatch(p2);
  EXPECT_RESOLVED(r2);
  EXPECT_FALSE(r2->isConnected());
  // v6 route
  auto rib2v6 = t2->getRibV6();
  RouteV6::Prefix p2v6{IPAddressV6("2001::0"), 48};
  auto r2v6 = rib2v6->exactMatch(p2v6);
  EXPECT_RESOLVED(r2v6);
  EXPECT_FALSE(r2v6->isConnected());
  auto stateV2 = stateV1->clone();
  stateV2->resetRouteTables(tables2);
  StateDelta delta12(stateV1, stateV2);
  EXPECT_EQ(2, numChangedRoutes(delta12.getRouteTablesDelta()));
  checkChangedRouteTable(tables1, tables2, {0}, {}, {});
  checkChangedRoute(tables1, tables2,
                    {},
                    { TEMP::Route{0, IPAddress("10.1.1.0"), 24},
                      TEMP::Route{0, IPAddress("2001::0"), 48},
                    },
                    {});
  stateV2->publish();
  // Add 2 more routes
  RouteUpdater u2(stateV2->getRouteTables());
  u2.addRoute(rid, IPAddress("10.10.1.0"), 24,
              CLIENT_A, RouteNextHopEntry(nexthops));
  u2.addRoute(rid, IPAddress("2001:10::0"), 48,
              CLIENT_A, RouteNextHopEntry(nexthops));
  auto tables3 = u2.updateDone();
  ASSERT_NE(nullptr, tables3);

  // v4 route
  auto r3 = GET_ROUTE_V4(tables3, rid, "10.10.1.0/24");
  EXPECT_RESOLVED(r3);
  EXPECT_FALSE(r3->isConnected());
  // v6 route
  auto r3v6 = GET_ROUTE_V6(tables3, rid, "2001:10::0/48");
  EXPECT_RESOLVED(r3v6);
  EXPECT_FALSE(r3v6->isConnected());

  auto stateV3 = stateV2->clone();
  stateV3->resetRouteTables(tables3);
  StateDelta delta23(stateV2, stateV3);
  EXPECT_EQ(2, numChangedRoutes(delta23.getRouteTablesDelta()));
  checkChangedRouteTable(tables2, tables3, {0}, {}, {});
  checkChangedRoute(tables2, tables3,
                    {},
                    { TEMP::Route{0, IPAddress("10.10.1.0"), 24},
                      TEMP::Route{0, IPAddress("2001:10::0"), 48},
                    },
                    {});
  stateV3->publish();
}

TEST(Route, PruneAddedRoutes) {
  // start with one interface (21)
  // Add two routes (r1prefix, r2prefix)
  // Prune one of them (prefix1)
  // Check that the pruning happened correctly
  auto platform = createMockPlatform();
  auto state0 = make_shared<SwitchState>();
  // state0 = the empty config

  cfg::SwitchConfig config;
  config.vlans.resize(1);

  config.vlans[0].id = 21;

  config.interfaces.resize(1);
  config.interfaces[0].intfID = 21;
  config.interfaces[0].vlanID = 21;
  config.interfaces[0].routerID = 0;
  config.interfaces[0].__isset.mac = true;
  config.interfaces[0].mac = "fa:ce:b0:0c:21:00";
  config.interfaces[0].ipAddresses.resize(2);
  config.interfaces[0].ipAddresses[0] = "10.0.21.1/24";
  config.interfaces[0].ipAddresses[1] = "face:b00c:0:21::1/64";

  // state0
  //  ... apply interfaces config
  // state1
  auto state1 = publishAndApplyConfig(state0, &config, platform.get());

  ASSERT_NE(nullptr, state1);

  auto state2 = state1;
  // state1
  //  ... add route for prefix1
  //  ... add route for prefix2
  // state2
  auto rid0 = RouterID(0);
  auto tables1 = state1->getRouteTables();
  RouteUpdater u1(tables1);

  auto r1prefix = IPAddressV4("20.0.1.51");
  auto r1prefixLen = 24;
  auto r1nexthops = makeNextHops({"10.0.21.51", "30.0.21.51" /* unresolved */});

  u1.addRoute(rid0, r1prefix, r1prefixLen,
              CLIENT_A, RouteNextHopEntry(r1nexthops));

  auto r2prefix = IPAddressV6("facf:b00c::52");
  auto r2prefixLen = 96;
  auto r2nexthops =
      makeNextHops({"30.0.21.52" /* unresolved */, "face:b00c:0:21::52"});
  u1.addRoute(rid0, r2prefix, r2prefixLen,
              CLIENT_A, RouteNextHopEntry(r2nexthops));

  auto tables2 = u1.updateDone();

  SwitchState::modify(&state2);
  state2->resetRouteTables(tables2);
  state2->publish();

  std::shared_ptr<SwitchState> state3{state2};
  // state2
  //  ... revert route for prefix1
  // state3
  RouteV4::Prefix prefix1{IPAddressV4("20.0.1.51"), 24};

  auto newRouteEntry =
      state3->getRouteTables()->getRouteTable(rid0)->getRibV4()->longestMatch(
          prefix1.network);
  ASSERT_NE(nullptr, newRouteEntry);
  ASSERT_EQ(state2, state3);
  SwitchState::revertNewRouteEntry(
      rid0, newRouteEntry, std::shared_ptr<RouteV4>(), &state3);
  // Make sure that state3 changes as a result of pruning
  ASSERT_NE(state2, state3);
  auto remainingRouteEntry =
      state3->getRouteTables()->getRouteTable(rid0)->getRibV4()->longestMatch(
          prefix1.network);
  ASSERT_EQ(nullptr, remainingRouteEntry);
}

// Test that pruning of changed routes happens correctly.
TEST(Route, PruneChangedRoutes) {
  // start with one interface (21)
  // Add two routes
  // Change one of them
  // Prune the changed one
  // Check that the pruning happened correctly
  auto platform = createMockPlatform();
  auto state0 = make_shared<SwitchState>();
  // state0 = empty state

  cfg::SwitchConfig config;
  config.vlans.resize(1);

  config.vlans[0].id = 21;

  config.interfaces.resize(1);
  config.interfaces[0].intfID = 21;
  config.interfaces[0].vlanID = 21;
  config.interfaces[0].routerID = 0;
  config.interfaces[0].__isset.mac = true;
  config.interfaces[0].mac = "fa:ce:b0:0c:21:00";
  config.interfaces[0].ipAddresses.resize(2);
  config.interfaces[0].ipAddresses[0] = "10.0.21.1/24";
  config.interfaces[0].ipAddresses[1] = "face:b00c:0:21::1/64";

  // state0
  //  ... add interface 21
  // state1
  auto state1 = publishAndApplyConfig(state0, &config, platform.get());

  ASSERT_NE(nullptr, state1);

  auto state2 = state1;
  // state1
  //  ... Add route for prefix41
  //  ... Add route for prefix42 (TO_CPU)
  // state2
  auto rid0 = RouterID(0);
  auto tables1 = state1->getRouteTables();
  RouteUpdater u1(tables1);

  RouteV4::Prefix prefix41{IPAddressV4("20.0.21.41"), 32};
  auto nexthops41 = makeNextHops({"10.0.21.41", "face:b00c:0:21::41"});
  u1.addRoute(rid0, prefix41.network, prefix41.mask,
              CLIENT_A, RouteNextHopEntry(nexthops41));

  RouteV6::Prefix prefix42{IPAddressV6("facf:b00c:0:21::42"), 96};
  u1.addRoute(rid0, prefix42.network, prefix41.mask,
              CLIENT_A, RouteNextHopEntry(TO_CPU));

  auto tables2 = u1.updateDone();
  SwitchState::modify(&state2);
  state2->resetRouteTables(tables2);
  state2->publish();

  auto oldEntry =
      state2->getRouteTables()->getRouteTable(rid0)->getRibV6()->longestMatch(
          prefix42.network);

  auto state3 = state2;
  // state2
  //  ... Make route for prefix42 resolve to actual nexthops
  // state3

  RouteUpdater u2(state2->getRouteTables());
  auto nexthops42 = makeNextHops({"10.0.21.42", "face:b00c:0:21::42"});
  u2.addRoute(rid0, prefix42.network, prefix41.mask,
              CLIENT_A, RouteNextHopEntry(nexthops42));
  auto tables3 = u2.updateDone();

  SwitchState::modify(&state3);
  state3->resetRouteTables(tables3);
  state3->publish();

  auto newEntry =
      state3->getRouteTables()->getRouteTable(rid0)->getRibV6()->longestMatch(
          prefix42.network);

  auto state4 = state3;
  // state3
  //  ... revert route for prefix42
  // state4
  ASSERT_EQ(state3, state4);
  SwitchState::revertNewRouteEntry(rid0, newEntry, oldEntry, &state4);
  ASSERT_NE(state3, state4);

  auto revertedEntry =
      state4->getRouteTables()->getRouteTable(rid0)->getRibV6()->longestMatch(
          prefix42.network);
  ASSERT(revertedEntry->isToCPU());
}

// Utility function for creating a nexthops list of size n,
// starting with the prefix.  For prefix "1.1.1.", first
// IP in the list will be 1.1.1.10
RouteNextHops newNextHops(int n, std::string prefix) {
  RouteNextHops h;
  for (int i=0; i < n; i++) {
    auto ipStr = prefix + std::to_string(i+10);
    h.emplace(RouteNextHop::createNextHop(IPAddress(ipStr)));
  }
  return h;
}

// Test adding and deleting per-client nexthops lists
TEST(Route, modRoutes) {
  auto stateV1 = make_shared<SwitchState>();
  stateV1->publish();
  auto tables1 = stateV1->getRouteTables();
  auto rid = RouterID(0);
  RouteUpdater u1(tables1);

  RouteV4::Prefix prefix10{IPAddressV4("10.10.10.10"), 32};
  RouteV4::Prefix prefix99{IPAddressV4("99.99.99.99"), 32};

  RouteNextHops nexthops1 = newNextHops(3, "1.1.1.");
  RouteNextHops nexthops2 = newNextHops(3, "2.2.2.");
  RouteNextHops nexthops3 = newNextHops(3, "3.3.3.");

  u1.addRoute(rid, IPAddress("10.10.10.10"), 32,
              CLIENT_A, RouteNextHopEntry(nexthops1));
  u1.addRoute(rid, IPAddress("10.10.10.10"), 32,
              CLIENT_B, RouteNextHopEntry(nexthops2));
  u1.addRoute(rid, IPAddress("99.99.99.99"), 32,
              CLIENT_A, RouteNextHopEntry(nexthops3));
  tables1 = u1.updateDone();
  tables1->publish();

  RouteUpdater u2(tables1);
  auto t1rt10 = tables1->getRouteTable(rid)->getRibV4()->exactMatch(prefix10);
  auto t1rt99 = tables1->getRouteTable(rid)->getRibV4()->exactMatch(prefix99);
  // Table1 has route 10 with two nexthop sets, and route 99 with one set
  EXPECT_TRUE(t1rt10->has(CLIENT_A, RouteNextHopEntry(nexthops1)));
  EXPECT_TRUE(t1rt10->has(CLIENT_B, RouteNextHopEntry(nexthops2)));
  EXPECT_TRUE(t1rt99->has(CLIENT_A, RouteNextHopEntry(nexthops3)));

  u2.delRoute(rid, IPAddress("10.10.10.10"), 32, CLIENT_A);
  auto tables2 = u2.updateDone();
  auto t2rt10 = tables2->getRouteTable(rid)->getRibV4()->exactMatch(prefix10);
  auto t2rt99 = tables2->getRouteTable(rid)->getRibV4()->exactMatch(prefix99);
  // Table2 should only be missing the 10.10.10.10 route for client CLIENT_A
  EXPECT_FALSE(t2rt10->has(CLIENT_A, RouteNextHopEntry(nexthops1)));
  EXPECT_TRUE(t2rt10->has(CLIENT_B, RouteNextHopEntry(nexthops2)));
  EXPECT_TRUE(t2rt99->has(CLIENT_A, RouteNextHopEntry(nexthops3)));
  EXPECT_EQ(t2rt10->getEntryForClient(CLIENT_A), nullptr);
  EXPECT_NE(t2rt10->getEntryForClient(CLIENT_B), nullptr);

  // Delete the second client/nexthop pair from table2.
  // The route & prefix should disappear altogether.
  RouteUpdater u3(tables2);
  u3.delRoute(rid, IPAddress("10.10.10.10"), 32, CLIENT_B);
  auto tables3 = u3.updateDone();
  auto t3rt10 = tables3->getRouteTable(rid)->getRibV4()->exactMatch(prefix10);
  EXPECT_EQ(t3rt10, nullptr);
}

// Test adding empty nextHops lists
TEST(Route, disallowEmptyNexthops) {
  auto stateV1 = make_shared<SwitchState>();
  stateV1->publish();
  auto tables1 = stateV1->getRouteTables();
  auto rid = RouterID(0);
  RouteUpdater u1(tables1);

  // It's illegal to add an empty nextHops list to a route

  // Test the case where the empty list is the first to be added to the Route
  ASSERT_THROW(u1.addRoute(rid, IPAddress("5.5.5.5"), 32,
                           CLIENT_A,
                           RouteNextHopEntry(newNextHops(0, "20.20.20."))),
               FbossError);

  // Test the case where the empty list is the second to be added to the Route
  u1.addRoute(rid, IPAddress("10.10.10.10"), 32,
              CLIENT_A, RouteNextHopEntry(newNextHops(3, "10.10.10.")));
  ASSERT_THROW(u1.addRoute(rid, IPAddress("10.10.10.10"), 32,
                           CLIENT_B,
                           RouteNextHopEntry(newNextHops(0, "20.20.20."))),
               FbossError);

}

// Test deleting routes
TEST(Route, delRoutes) {
  auto stateV1 = make_shared<SwitchState>();
  stateV1->publish();
  auto tables1 = stateV1->getRouteTables();
  auto rid = RouterID(0);
  RouteUpdater u1(tables1);

  RouteV4::Prefix prefix10{IPAddressV4("10.10.10.10"), 32};
  RouteV4::Prefix prefix22{IPAddressV4("22.22.22.22"), 32};

  u1.addRoute(rid, IPAddress("10.10.10.10"), 32,
              CLIENT_A, RouteNextHopEntry(newNextHops(3, "1.1.1.")));
  u1.addRoute(rid, IPAddress("22.22.22.22"), 32,
              CLIENT_B, RouteNextHopEntry(TO_CPU));
  tables1 = u1.updateDone();

  // Both routes should be present
  auto ribV4 = tables1->getRouteTable(rid)->getRibV4();
  EXPECT_TRUE(nullptr != ribV4->exactMatch(prefix10));
  EXPECT_TRUE(nullptr != ribV4->exactMatch(prefix22));

  // delRoute() should work for the route with TO_CPU.
  RouteUpdater u2(tables1);
  u2.delRoute(rid, IPAddress("22.22.22.22"), 32, CLIENT_B);
  auto tables2 = u2.updateDone();

  // Route for 10.10.10.10 should still be there,
  // but route for 22.22.22.22 should be gone
  ribV4 = tables2->getRouteTable(rid)->getRibV4();
  EXPECT_TRUE(nullptr != ribV4->exactMatch(prefix10));
  EXPECT_TRUE(nullptr == ribV4->exactMatch(prefix22));
}

// Test equality of RouteNextHopsMulti.
TEST(Route, equality) {

  // Create two identical RouteNextHopsMulti, and compare
  RouteNextHopsMulti nhm1;
  nhm1.update(CLIENT_A, RouteNextHopEntry(newNextHops(3, "1.1.1.")));
  nhm1.update(CLIENT_B, RouteNextHopEntry(newNextHops(3, "2.2.2.")));

  RouteNextHopsMulti nhm2;
  nhm2.update(CLIENT_A, RouteNextHopEntry(newNextHops(3, "1.1.1.")));
  nhm2.update(CLIENT_B, RouteNextHopEntry(newNextHops(3, "2.2.2.")));

  EXPECT_TRUE(nhm1 == nhm2);

  // Delete data for CLIENT_C.  But there wasn't any.  Two objs still equal
  nhm1.delEntryForClient(CLIENT_C);
  EXPECT_TRUE(nhm1 == nhm2);

  // Delete obj1's CLIENT_B.  Now, objs should be NOT equal
  nhm1.delEntryForClient(CLIENT_B);
  EXPECT_FALSE(nhm1 == nhm2);

  // Now replace obj1's CLIENT_B list with a shorter list.
  // Objs should be NOT equal.
  nhm1.update(CLIENT_B, RouteNextHopEntry(newNextHops(2, "2.2.2.")));
  EXPECT_FALSE(nhm1 == nhm2);

  // Now replace obj1's CLIENT_B list with the original list.
  // But construct the list in opposite order.
  // Objects should still be equal, despite the order of construction.
  RouteNextHops nextHopsRev;
  nextHopsRev.emplace(RouteNextHop::createNextHop(IPAddress("2.2.2.12")));
  nextHopsRev.emplace(RouteNextHop::createNextHop(IPAddress("2.2.2.11")));
  nextHopsRev.emplace(RouteNextHop::createNextHop(IPAddress("2.2.2.10")));
  nhm1.update(CLIENT_B, RouteNextHopEntry(nextHopsRev));
  EXPECT_TRUE(nhm1 == nhm2);
}

// Test that a copy of a RouteNextHopsMulti is a deep copy, and that the
// resulting objects can be modified independently.
TEST(Route, deepCopy) {

  // Create two identical RouteNextHopsMulti, and compare
  RouteNextHopsMulti nhm1;
  auto origHops = newNextHops(3, "1.1.1.");
  nhm1.update(CLIENT_A, RouteNextHopEntry(origHops));
  nhm1.update(CLIENT_B, RouteNextHopEntry(newNextHops(3, "2.2.2.")));

  // Copy it
  RouteNextHopsMulti nhm2 = nhm1;

  // The two should be identical
  EXPECT_TRUE(nhm1 == nhm2);

  // Now modify the underlying nexthop list.
  // Should be changed in nhm1, but not nhm2.
  auto newHops = newNextHops(4, "10.10.10.");
  nhm1.update(CLIENT_A, RouteNextHopEntry(newHops));

  EXPECT_FALSE(nhm1 == nhm2);

  EXPECT_TRUE(nhm1.isSame(CLIENT_A, newHops));
  EXPECT_TRUE(nhm2.isSame(CLIENT_A, origHops));
}

// Test serialization of RouteNextHopsMulti.
TEST(Route, serializeROuteNextHopsMulti) {

  // This function tests [de]serialization of:
  // RouteNextHopsMulti
  // RouteNextHopEntry
  // RouteNextHop

  // test for new format to new format
  {
    RouteNextHopsMulti nhm1;
    nhm1.update(CLIENT_A, RouteNextHopEntry(newNextHops(3, "1.1.1.")));
    nhm1.update(CLIENT_B, RouteNextHopEntry(newNextHops(1, "2.2.2.")));
    nhm1.update(CLIENT_C, RouteNextHopEntry(newNextHops(4, "3.3.3.")));
    nhm1.update(CLIENT_D, RouteNextHopEntry(RouteForwardAction::DROP));
    nhm1.update(CLIENT_E, RouteNextHopEntry(RouteForwardAction::TO_CPU));

    auto serialized = nhm1.toFollyDynamic();

    auto nhm2 = RouteNextHopsMulti::fromFollyDynamic(serialized);

    EXPECT_TRUE(nhm1 == nhm2);
  }

  // test for deserialize from old format
  {
    RouteNextHopsMulti nhm1;
    nhm1.update(CLIENT_A, RouteNextHopEntry(newNextHops(3, "1.1.1.")));
    nhm1.update(CLIENT_B, RouteNextHopEntry(newNextHops(1, "2.2.2.")));
    nhm1.update(CLIENT_C, RouteNextHopEntry(newNextHops(4, "3.3.3.")));

    auto serialized = nhm1.toFollyDynamicOld();

    auto nhm2 = RouteNextHopsMulti::fromFollyDynamicOld(serialized);

    EXPECT_TRUE(nhm1 == nhm2);
  }
}

// Test priority ranking of nexthop lists within a RouteNextHopsMulti.
TEST(Route, listRanking) {

  auto list00 = newNextHops(3, "0.0.0.");
  auto list07 = newNextHops(3, "7.7.7.");
  auto list10 = newNextHops(3, "10.10.10.");
  auto list20 = newNextHops(3, "20.20.20.");
  auto list30 = newNextHops(3, "30.30.30.");

  RouteNextHopsMulti nhm;
  nhm.update(ClientID(20), RouteNextHopEntry(list20));
  nhm.update(ClientID(10), RouteNextHopEntry(list10));
  nhm.update(ClientID(30), RouteNextHopEntry(list30));
  EXPECT_TRUE(nhm.getBestEntry().second->getNextHopSet() == list10);

  nhm.update(ClientID(0), RouteNextHopEntry(list00));
  nhm.update(ClientID(7), RouteNextHopEntry(list07));
  EXPECT_TRUE(nhm.getBestEntry().second->getNextHopSet() == list00);

  nhm.delEntryForClient(ClientID(0));
  EXPECT_TRUE(nhm.getBestEntry().second->getNextHopSet() == list07);

  nhm.delEntryForClient(ClientID(10));
  EXPECT_TRUE(nhm.getBestEntry().second->getNextHopSet() == list07);

  nhm.delEntryForClient(ClientID(7));
  EXPECT_TRUE(nhm.getBestEntry().second->getNextHopSet() == list20);

  nhm.delEntryForClient(ClientID(20));
  EXPECT_TRUE(nhm.getBestEntry().second->getNextHopSet() == list30);

  nhm.delEntryForClient(ClientID(30));
  EXPECT_THROW(nhm.getBestEntry().second->getNextHopSet(), FbossError);
}

bool stringStartsWith(std::string s1, std::string prefix) {
  return s1.compare(0, prefix.size(), prefix) == 0;
}

void assertClientsNotPresent(std::shared_ptr<RouteTableMap>& tables,
                             RouterID rid, RouteV4::Prefix prefix,
                             std::vector<int16_t> clientIds) {
  for (int16_t clientId : clientIds) {
    const auto& route =
      tables->getRouteTable(rid)->getRibV4()->exactMatch(prefix);
    auto entry = route->getEntryForClient(ClientID(clientId));
    EXPECT_EQ(nullptr, entry);
  }
}

void assertClientsPresent(std::shared_ptr<RouteTableMap>& tables, RouterID rid,
                          RouteV4::Prefix prefix,
                          std::vector<int16_t> clientIds) {
  for (int16_t clientId : clientIds) {
    const auto& route =
      tables->getRouteTable(rid)->getRibV4()->exactMatch(prefix);
    auto entry = route->getEntryForClient(ClientID(clientId));
    EXPECT_NE(nullptr, entry);
  }
}

void expectFwdInfo(std::shared_ptr<RouteTableMap>& tables, RouterID rid,
                   RouteV4::Prefix prefix, std::string ipPrefix) {

  const auto& fwd = tables->getRouteTable(rid)->getRibV4()
    ->exactMatch(prefix)->getForwardInfo();
  const auto& nhops = fwd.getNextHopSet();
  // Expect the fwd'ing info to be 3 IPs all starting with 'ipPrefix'
  EXPECT_EQ(3, nhops.size());
  for (auto const& it : nhops) {
    EXPECT_TRUE(stringStartsWith(it.addr().str(), ipPrefix));
  }
}

std::shared_ptr<RouteTableMap>&
addNextHopsForClient(std::shared_ptr<RouteTableMap>& tables, RouterID rid,
                     RouteV4::Prefix prefix, int16_t clientId,
                     std::string ipPrefix) {
      RouteUpdater u(tables);
      u.addRoute(rid, prefix.network, prefix.mask,
                 ClientID(clientId),
                 RouteNextHopEntry(newNextHops(3, ipPrefix)));
      tables = u.updateDone();
      tables->publish();
      return tables;
}

std::shared_ptr<RouteTableMap>&
deleteNextHopsForClient(std::shared_ptr<RouteTableMap>& tables, RouterID rid,
                        RouteV4::Prefix prefix, int16_t clientId) {
      RouteUpdater u(tables);
      u.delRoute(rid, prefix.network, prefix.mask, ClientID(clientId));
      tables = u.updateDone();
      tables->publish();
      return tables;
}

// Add and remove per-client NextHop lists to the same route, and make sure
// the highest-priority client is the one that determines the forwarding info
TEST(Route, fwdInfoRanking) {
  auto stateV1 = make_shared<SwitchState>();
  stateV1->publish();
  auto tables = stateV1->getRouteTables();
  auto rid = RouterID(0);

  // We'll be adding and removing a bunch of nexthops for this Network & Mask
  auto network = IPAddressV4("22.22.22.22");
  uint8_t mask = 32;
  RouteV4::Prefix prefix{network, mask};


  // Add client 30, plus an interface for resolution.
  RouteUpdater u1(tables);
  // This is the route all the others will resolve to.
  u1.addRoute(rid, IPAddress("10.10.0.0"), 16,
              StdClientIds2ClientID(StdClientIds::INTERFACE_ROUTE),
              RouteNextHopEntry(
                  RouteNextHop::createInterfaceNextHop(
                      IPAddress("10.10.0.1"), InterfaceID(9))));
  u1.addRoute(rid, network, mask,
              ClientID(30), RouteNextHopEntry(newNextHops(3, "10.10.30.")));
  tables = u1.updateDone();
  tables->publish();

  // Expect fwdInfo based on client 30
  assertClientsPresent(tables, rid, prefix, {30});
  assertClientsNotPresent(tables, rid, prefix, {10, 20, 40, 50, 999});
  expectFwdInfo(tables, rid, prefix, "10.10.30.");

  // Add client 20
  tables = addNextHopsForClient(tables, rid, prefix, 20, "10.10.20.");

  // Expect fwdInfo based on client 20
  assertClientsPresent(tables, rid, prefix, {20, 30});
  assertClientsNotPresent(tables, rid, prefix, {10, 40, 50, 999});
  expectFwdInfo(tables, rid, prefix, "10.10.20.");

  // Add client 40
  tables = addNextHopsForClient(tables, rid, prefix, 40, "10.10.40.");

  // Expect fwdInfo still based on client 20
  assertClientsPresent(tables, rid, prefix, {20, 30, 40});
  assertClientsNotPresent(tables, rid, prefix, {10, 50, 999});
  expectFwdInfo(tables, rid, prefix, "10.10.20.");

  // Add client 10
  tables = addNextHopsForClient(tables, rid, prefix, 10, "10.10.10.");

  // Expect fwdInfo based on client 10
  assertClientsPresent(tables, rid, prefix, {10, 20, 30, 40});
  assertClientsNotPresent(tables, rid, prefix, {50, 999});
  expectFwdInfo(tables, rid, prefix, "10.10.10.");

  // Remove client 20
  tables = deleteNextHopsForClient(tables, rid, prefix, 20);

  // Winner should still be 10
  assertClientsPresent(tables, rid, prefix, {10, 30, 40});
  assertClientsNotPresent(tables, rid, prefix, {20, 50, 999});
  expectFwdInfo(tables, rid, prefix, "10.10.10.");

  // Remove client 10
  tables = deleteNextHopsForClient(tables, rid, prefix, 10);

  // Winner should now be 30
  assertClientsPresent(tables, rid, prefix, {30, 40});
  assertClientsNotPresent(tables, rid, prefix, {10, 20, 50, 999});
  expectFwdInfo(tables, rid, prefix, "10.10.30.");

  // Remove client 30
  tables = deleteNextHopsForClient(tables, rid, prefix, 30);

  // Winner should now be 40
  assertClientsPresent(tables, rid, prefix, {40});
  assertClientsNotPresent(tables, rid, prefix, {10, 20, 30, 50, 999});
  expectFwdInfo(tables, rid, prefix, "10.10.40.");
}

TEST(Route, dropRoutes) {
  auto stateV1 = make_shared<SwitchState>();
  stateV1->publish();
  auto tables1 = stateV1->getRouteTables();
  auto rid = RouterID(0);
  RouteUpdater u1(tables1);
  u1.addRoute(rid, IPAddress("10.10.10.10"), 32,
              CLIENT_A, RouteNextHopEntry(DROP));
  u1.addRoute(rid, IPAddress("2001::0"), 128,
              CLIENT_A, RouteNextHopEntry(DROP));
  // Check recursive resolution for drop routes
  RouteNextHops v4nexthops = makeNextHops({"10.10.10.10"});
  u1.addRoute(rid, IPAddress("20.20.20.0"), 24,
              CLIENT_A, RouteNextHopEntry(v4nexthops));
  RouteNextHops v6nexthops = makeNextHops({"2001::0"});
  u1.addRoute(rid, IPAddress("2001:1::"), 64,
              CLIENT_A, RouteNextHopEntry(v6nexthops));

  auto tables2 = u1.updateDone();
  ASSERT_NE(nullptr, tables2);

  // Check routes
  auto r1 = GET_ROUTE_V4(tables2, rid, "10.10.10.10/32");
  EXPECT_RESOLVED(r1);
  EXPECT_FALSE(r1->isConnected());
  EXPECT_TRUE(r1->has(CLIENT_A, RouteNextHopEntry(DROP)));
  EXPECT_EQ(r1->getForwardInfo(), RouteNextHopEntry(DROP));

  auto r2 = GET_ROUTE_V4(tables2, rid, "20.20.20.0/24");
  EXPECT_RESOLVED(r2);
  EXPECT_FALSE(r2->isConnected());
  EXPECT_FALSE(r2->has(CLIENT_A, RouteNextHopEntry(DROP)));
  EXPECT_EQ(r2->getForwardInfo(), RouteNextHopEntry(DROP));

  auto r3 = GET_ROUTE_V6(tables2, rid, "2001::0/128");
  EXPECT_RESOLVED(r3);
  EXPECT_FALSE(r3->isConnected());
  EXPECT_TRUE(r3->has(CLIENT_A, RouteNextHopEntry(DROP)));
  EXPECT_EQ(r3->getForwardInfo(), RouteNextHopEntry(DROP));

  auto r4 = GET_ROUTE_V6(tables2, rid, "2001:1::/64");
  EXPECT_RESOLVED(r4);
  EXPECT_FALSE(r4->isConnected());
  EXPECT_EQ(r4->getForwardInfo(), RouteNextHopEntry(DROP));
}

TEST(Route, toCPURoutes) {
  auto stateV1 = make_shared<SwitchState>();
  stateV1->publish();
  auto tables1 = stateV1->getRouteTables();
  auto rid = RouterID(0);
  RouteUpdater u1(tables1);
  u1.addRoute(rid, IPAddress("10.10.10.10"), 32,
              CLIENT_A, RouteNextHopEntry(TO_CPU));
  u1.addRoute(rid, IPAddress("2001::0"), 128,
              CLIENT_A, RouteNextHopEntry(TO_CPU));
  // Check recursive resolution for to_cpu routes
  RouteNextHops v4nexthops = makeNextHops({"10.10.10.10"});
  u1.addRoute(rid, IPAddress("20.20.20.0"), 24 ,
              CLIENT_A, RouteNextHopEntry(v4nexthops));
  RouteNextHops v6nexthops = makeNextHops({"2001::0"});
  u1.addRoute(rid, IPAddress("2001:1::"), 64,
              CLIENT_A, RouteNextHopEntry(v6nexthops));

  auto tables2 = u1.updateDone();
  ASSERT_NE(nullptr, tables2);

  // Check routes
  auto r1 = GET_ROUTE_V4(tables2, rid, "10.10.10.10/32");
  EXPECT_RESOLVED(r1);
  EXPECT_FALSE(r1->isConnected());
  EXPECT_TRUE(r1->has(CLIENT_A, RouteNextHopEntry(TO_CPU)));
  EXPECT_EQ(r1->getForwardInfo(), RouteNextHopEntry(TO_CPU));

  auto r2 = GET_ROUTE_V4(tables2, rid, "20.20.20.0/24");
  EXPECT_RESOLVED(r2);
  EXPECT_FALSE(r2->isConnected());
  EXPECT_EQ(r2->getForwardInfo(), RouteNextHopEntry(TO_CPU));

  auto r3 = GET_ROUTE_V6(tables2, rid, "2001::0/128");
  EXPECT_RESOLVED(r3);
  EXPECT_FALSE(r3->isConnected());
  EXPECT_TRUE(r3->has(CLIENT_A, RouteNextHopEntry(TO_CPU)));
  EXPECT_EQ(r3->getForwardInfo(), RouteNextHopEntry(TO_CPU));

  auto r5 = GET_ROUTE_V6(tables2, rid, "2001:1::/64");
  EXPECT_RESOLVED(r5);
  EXPECT_FALSE(r5->isConnected());
  EXPECT_EQ(r5->getForwardInfo(), RouteNextHopEntry(TO_CPU));
}

// Very basic test for serialization/deseralization of Routes
TEST(Route, serializeRoute) {

  ClientID clientId = ClientID(1);
  auto nxtHops = makeNextHops({"10.10.10.10", "11.11.11.11"});
  Route<IPAddressV4> rt(makePrefixV4("1.2.3.4/32"));
  rt.update(clientId, RouteNextHopEntry(nxtHops));

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
  ASSERT_TRUE(rt2->has(clientId, RouteNextHopEntry(nxtHops)));
}

// Test utility functions for converting RouteNextHops to thrift and back
TEST(RouteTypes, toFromRouteNextHops) {
  RouteNextHops nhs;

  // Non v4 link-local address without interface scoping
  nhs.emplace(RouteNextHop::createNextHop(
                  folly::IPAddress("10.0.0.1"), folly::none));

  // v4 link-local address with/without interface scoping
  nhs.emplace(RouteNextHop::createNextHop(
                  folly::IPAddress("169.254.0.1"), folly::none));
  nhs.emplace(RouteNextHop::createNextHop(
                  folly::IPAddress("169.254.0.2"), InterfaceID(2)));

  // Non v6 link-local address without interface scoping
  nhs.emplace(RouteNextHop::createNextHop(
                  folly::IPAddress("face:b00c::1"), folly::none));

  // v6 link-local address with interface scoping
  nhs.emplace(RouteNextHop::createNextHop(
                  folly::IPAddress("fe80::1"), InterfaceID(4)));

  // Conver to thrift object
  auto nhAddrs = util::fromRouteNextHops(nhs);
  ASSERT_EQ(5, nhAddrs.size());

  auto verify = [&](const std::string& ipaddr,
                    folly::Optional<InterfaceID> intf){
    auto bAddr = facebook::network::toBinaryAddress(folly::IPAddress(ipaddr));
    if (intf.hasValue()) {
      bAddr.__isset.ifName = true;
      bAddr.ifName = util::createTunIntfName(intf.value());
    }
    bool found = false;
    for (const auto& entry : nhAddrs) {
      if (entry == bAddr) {
        if (intf.hasValue()) {
          EXPECT_TRUE(entry.__isset.ifName);
          EXPECT_EQ(bAddr.ifName, entry.ifName);
        }
        found = true;
        break;
      }
    }
    EXPECT_TRUE(found);
  };

  verify("10.0.0.1", folly::none);
  verify("169.254.0.1", folly::none);
  verify("169.254.0.2", InterfaceID(2));
  verify("face:b00c::1", folly::none);
  verify("fe80::1", InterfaceID(4));

  // Convert back to RouteNextHops
  auto newNhs = util::toRouteNextHops(nhAddrs);
  EXPECT_EQ(nhs, newNhs);

  //
  // Some error cases
  //

  facebook::network::thrift::BinaryAddress addr;

  addr = facebook::network::toBinaryAddress(folly::IPAddress("10.0.0.1"));
  addr.__isset.ifName = true;
  addr.ifName = "fboss10";
  EXPECT_THROW(RouteNextHop::fromThrift(addr), FbossError);

  addr = facebook::network::toBinaryAddress(folly::IPAddress("face::1"));
  addr.__isset.ifName = true;
  addr.ifName = "fboss10";
  EXPECT_THROW(RouteNextHop::fromThrift(addr), FbossError);
}
