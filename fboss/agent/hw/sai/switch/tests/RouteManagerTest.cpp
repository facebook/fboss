/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/hw/sai/switch/SaiManagerTable.h"
#include "fboss/agent/hw/sai/switch/SaiNextHopGroupManager.h"
#include "fboss/agent/hw/sai/switch/SaiRouteManager.h"
#include "fboss/agent/hw/sai/switch/SaiSwitchManager.h"
#include "fboss/agent/hw/sai/switch/SaiVirtualRouterManager.h"
#include "fboss/agent/hw/sai/switch/tests/ManagerTestBase.h"
#include "fboss/agent/state/Route.h"
#include "fboss/agent/types.h"

#include <optional>

using namespace facebook::fboss;
class RouteManagerTest : public ManagerTestBase {
 public:
  void SetUp() override {
    setupStage = SetupStage::PORT | SetupStage::VLAN | SetupStage::INTERFACE |
        SetupStage::NEIGHBOR;
    ManagerTestBase::SetUp();
    d1 = {folly::IPAddress{"42.42.42.42"}, 24};
    d2 = {folly::IPAddress{"43.43.43.43"}, 24};
    tr1.destination = d1;
    tr2.destination = d2;
    tr1.nextHopInterfaces.push_back(testInterfaces.at(0));
    tr1.nextHopInterfaces.push_back(testInterfaces.at(1));
    tr1.nextHopInterfaces.push_back(testInterfaces.at(2));
    tr1.nextHopInterfaces.push_back(testInterfaces.at(3));
  }

  folly::CIDRNetwork d1;
  folly::CIDRNetwork d2;
  TestRoute tr1;
  TestRoute tr2;
};

TEST_F(RouteManagerTest, addRoute) {
  auto r = makeRoute(tr1);
  saiManagerTable->routeManager().addRoute<folly::IPAddressV4>(r, RouterID(0));
}

TEST_F(RouteManagerTest, addRouteSameNextHops) {
  tr2.nextHopInterfaces = tr1.nextHopInterfaces;
  auto r1 = makeRoute(tr1);
  auto r2 = makeRoute(tr2);
  saiManagerTable->routeManager().addRoute<folly::IPAddressV4>(r1, RouterID(0));
  saiManagerTable->routeManager().addRoute<folly::IPAddressV4>(r2, RouterID(0));
}

TEST_F(RouteManagerTest, addRouteDifferentNextHops) {
  auto r1 = makeRoute(tr1);
  tr2.nextHopInterfaces.push_back(testInterfaces.at(1));
  tr2.nextHopInterfaces.push_back(testInterfaces.at(3));
  auto r2 = makeRoute(tr2);
  saiManagerTable->routeManager().addRoute<folly::IPAddressV4>(r1, RouterID(0));
  saiManagerTable->routeManager().addRoute<folly::IPAddressV4>(r2, RouterID(0));
}

TEST_F(RouteManagerTest, addRouteOneNextHop) {
  tr1.nextHopInterfaces = {testInterfaces.at(1)};
  auto r = makeRoute(tr1);
  saiManagerTable->routeManager().addRoute<folly::IPAddressV4>(r, RouterID(0));
}

TEST_F(RouteManagerTest, updateRouteOneNextHopNoUpdate) {
  tr1.nextHopInterfaces = {testInterfaces.at(1)};
  auto r = makeRoute(tr1);
  saiManagerTable->routeManager().addRoute<folly::IPAddressV4>(r, RouterID(0));
  auto switchId = saiManagerTable->switchManager().getSwitchSaiId();
  auto vrfId = saiManagerTable->virtualRouterManager()
                   .getVirtualRouterHandle(RouterID(0))
                   ->virtualRouter->adapterKey();
  folly::CIDRNetwork network{
      folly::IPAddress(r->prefix().network()), r->prefix().mask()};
  SaiRouteTraits::RouteEntry entry(switchId, vrfId, network);
  auto handle = saiManagerTable->routeManager().getRouteHandle(entry);
  auto nexthopHandle = handle->nexthopHandle_;
  auto r1 = makeRoute(tr1);
  r1->updateClassID(std::make_optional<cfg::AclLookupClass>(
      cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_0));
  saiManagerTable->routeManager().changeRoute<folly::IPAddressV4>(
      r, r1, RouterID(0));
  auto handle1 = saiManagerTable->routeManager().getRouteHandle(entry);
  auto nexthopHandle1 = handle1->nexthopHandle_;
  EXPECT_EQ(nexthopHandle, nexthopHandle1);
}

TEST_F(RouteManagerTest, addToCpuRoute) {
  TestInterface intf = testInterfaces.at(1);
  RouteFields<folly::IPAddressV4>::Prefix destination(
      intf.subnet.first.asV4(), intf.subnet.second);
  auto r = std::make_shared<Route<folly::IPAddressV4>>(
      Route<folly::IPAddressV4>::makeThrift(destination));
  RouteNextHopEntry entry(
      RouteForwardAction::TO_CPU, AdminDistance::STATIC_ROUTE);
  r->update(ClientID{42}, entry);
  r->setResolved(entry);
  saiManagerTable->routeManager().addRoute<folly::IPAddressV4>(r, RouterID(0));
  auto saiEntry =
      saiManagerTable->routeManager().routeEntryFromSwRoute(RouterID(0), r);
  auto saiRouteHandle =
      saiManagerTable->routeManager().getRouteHandle(saiEntry);
  auto saiRoute = saiRouteHandle->route;
  // sai_object_id_t of cpu port is 0 in FakeSai
  EXPECT_EQ(
      GET_ATTR(Route, PacketAction, saiRoute->attributes()),
      SAI_PACKET_ACTION_FORWARD);
  EXPECT_EQ(GET_OPT_ATTR(Route, NextHopId, saiRoute->attributes()), 0);
}

TEST_F(RouteManagerTest, updateRouteOneNextHopUpdate) {
  tr1.nextHopInterfaces = {testInterfaces.at(1)};
  auto r = makeRoute(tr1);
  saiManagerTable->routeManager().addRoute<folly::IPAddressV4>(r, RouterID(0));
  auto switchId = saiManagerTable->switchManager().getSwitchSaiId();
  auto vrfId = saiManagerTable->virtualRouterManager()
                   .getVirtualRouterHandle(RouterID(0))
                   ->virtualRouter->adapterKey();
  folly::CIDRNetwork network{
      folly::IPAddress(r->prefix().network()), r->prefix().mask()};
  SaiRouteTraits::RouteEntry entry(switchId, vrfId, network);
  auto handle = saiManagerTable->routeManager().getRouteHandle(entry);
  auto nexthopHandle = handle->nexthopHandle_;

  RouteNextHopEntry routeNextHopEntry(
      RouteForwardAction::TO_CPU, AdminDistance::STATIC_ROUTE);

  auto r1 = makeRoute(tr1);
  r1->setResolved(routeNextHopEntry);
  saiManagerTable->routeManager().changeRoute<folly::IPAddressV4>(
      r, r1, RouterID(0));

  auto handle1 = saiManagerTable->routeManager().getRouteHandle(entry);
  auto nexthopHandle1 = handle1->nexthopHandle_;
  EXPECT_NE(nexthopHandle, nexthopHandle1);
}

TEST_F(RouteManagerTest, addDropRoute) {
  TestInterface intf = testInterfaces.at(1);
  RouteFields<folly::IPAddressV4>::Prefix destination(
      intf.subnet.first.asV4(), intf.subnet.second);
  auto r = std::make_shared<Route<folly::IPAddressV4>>(
      RouteV4::makeThrift(destination));
  RouteNextHopEntry entry(
      RouteForwardAction::DROP, AdminDistance::STATIC_ROUTE);
  r->update(ClientID{42}, entry);
  r->setResolved(entry);
  saiManagerTable->routeManager().addRoute<folly::IPAddressV4>(r, RouterID(0));
}

TEST_F(RouteManagerTest, addSubnetRoute) {
  TestInterface intf = testInterfaces.at(1);
  RouteFields<folly::IPAddressV4>::Prefix destination(
      intf.subnet.first.asV4(), intf.subnet.second);
  ResolvedNextHop nh{intf.routerIp, InterfaceID(intf.id), ECMP_WEIGHT};
  RouteNextHopEntry::NextHopSet swNextHops{nh};
  RouteNextHopEntry entry(swNextHops, AdminDistance::DIRECTLY_CONNECTED);
  auto r = std::make_shared<Route<folly::IPAddressV4>>(
      RouteV4::makeThrift(destination));
  r->update(ClientID::INTERFACE_ROUTE, entry);
  r->setResolved(entry);
  r->setConnected();
  saiManagerTable->routeManager().addRoute<folly::IPAddressV4>(r, RouterID(0));
}

TEST_F(RouteManagerTest, getRoute) {
  auto r = makeRoute(tr1);
  saiManagerTable->routeManager().addRoute<folly::IPAddressV4>(r, RouterID(0));
  auto entry =
      saiManagerTable->routeManager().routeEntryFromSwRoute(RouterID(0), r);
  SaiRouteHandle* saiRouteHandle =
      saiManagerTable->routeManager().getRouteHandle(entry);
  EXPECT_TRUE(saiRouteHandle);
}

TEST_F(RouteManagerTest, removeRoute) {
  auto r = makeRoute(tr1);
  saiManagerTable->routeManager().addRoute<folly::IPAddressV4>(r, RouterID(0));
  auto entry =
      saiManagerTable->routeManager().routeEntryFromSwRoute(RouterID(0), r);
  SaiRouteHandle* saiRouteHandle =
      saiManagerTable->routeManager().getRouteHandle(entry);
  EXPECT_TRUE(saiRouteHandle);
  saiManagerTable->routeManager().removeRoute(r, RouterID(0));
  saiRouteHandle = saiManagerTable->routeManager().getRouteHandle(entry);
  EXPECT_FALSE(saiRouteHandle);
}

TEST_F(RouteManagerTest, addDupRoute) {
  auto r = makeRoute(tr1);
  saiManagerTable->routeManager().addRoute<folly::IPAddressV4>(r, RouterID(0));
  EXPECT_THROW(
      saiManagerTable->routeManager().addRoute<folly::IPAddressV4>(
          r, RouterID(0)),
      FbossError);
}

TEST_F(RouteManagerTest, getNonexistentRoute) {
  auto r1 = makeRoute(tr1);
  tr2.nextHopInterfaces = tr1.nextHopInterfaces;
  auto r2 = makeRoute(tr2);
  saiManagerTable->routeManager().addRoute<folly::IPAddressV4>(r1, RouterID(0));
  auto entry =
      saiManagerTable->routeManager().routeEntryFromSwRoute(RouterID(0), r2);
  SaiRouteHandle* saiRouteHandle =
      saiManagerTable->routeManager().getRouteHandle(entry);
  EXPECT_FALSE(saiRouteHandle);
}

TEST_F(RouteManagerTest, removeNonexistentRoute) {
  auto r1 = makeRoute(tr1);
  tr2.nextHopInterfaces = tr1.nextHopInterfaces;
  auto r2 = makeRoute(tr2);
  saiManagerTable->routeManager().addRoute<folly::IPAddressV4>(r1, RouterID(0));
  EXPECT_THROW(
      saiManagerTable->routeManager().removeRoute(r2, RouterID(0)), FbossError);
}

TEST_F(RouteManagerTest, updateNonexistentRoute) {
  auto r1 = makeRoute(tr1);
  tr2.nextHopInterfaces = tr1.nextHopInterfaces;
  auto r2 = makeRoute(tr2);
  EXPECT_THROW(
      saiManagerTable->routeManager().changeRoute<folly::IPAddressV4>(
          r1, r2, RouterID(0)),
      FbossError);
}

TEST_F(RouteManagerTest, updateNexthopToNexthopRoute) {
  auto r1 = makeRoute(tr1);
  saiManagerTable->routeManager().addRoute<folly::IPAddressV4>(r1, RouterID(0));
  auto entry =
      saiManagerTable->routeManager().routeEntryFromSwRoute(RouterID(0), r1);
  SaiRouteHandle* saiRouteHandle =
      saiManagerTable->routeManager().getRouteHandle(entry);
  auto nexthopGroupHandle1 = saiRouteHandle->nextHopGroupHandle();
  auto count = nexthopGroupHandle1->nextHopGroupSize();
  EXPECT_EQ(count, 4);
  tr1.nextHopInterfaces.clear();
  tr1.nextHopInterfaces.push_back(testInterfaces.at(4));
  tr1.nextHopInterfaces.push_back(testInterfaces.at(5));
  auto r2 = makeRoute(tr1);
  saiManagerTable->routeManager().changeRoute<folly::IPAddressV4>(
      r1, r2, RouterID(0));
  auto nexthopGroupHandle2 = saiRouteHandle->nextHopGroupHandle();
  count = nexthopGroupHandle2->nextHopGroupSize();
  EXPECT_EQ(count, 2);
  EXPECT_NE(nexthopGroupHandle1, nexthopGroupHandle2);
}

TEST_F(RouteManagerTest, updateDropRouteToNextHopRoute) {
  RouteFields<folly::IPAddressV4>::Prefix destination(
      tr1.destination.first.asV4(), tr1.destination.second);
  auto r1 = std::make_shared<Route<folly::IPAddressV4>>(
      RouteV4::makeThrift(destination));
  RouteNextHopEntry entry(
      RouteForwardAction::DROP, AdminDistance::STATIC_ROUTE);
  r1->update(ClientID{42}, entry);
  r1->setResolved(entry);
  saiManagerTable->routeManager().addRoute<folly::IPAddressV4>(r1, RouterID(0));
  auto routeEntry =
      saiManagerTable->routeManager().routeEntryFromSwRoute(RouterID(0), r1);
  SaiRouteHandle* saiRouteHandle =
      saiManagerTable->routeManager().getRouteHandle(routeEntry);
  auto r2 = makeRoute(tr1);
  saiManagerTable->routeManager().changeRoute<folly::IPAddressV4>(
      r1, r2, RouterID(0));
  auto nexthopGroupHandle2 = saiRouteHandle->nextHopGroupHandle();
  auto count = nexthopGroupHandle2->nextHopGroupSize();
  EXPECT_EQ(count, 4);
  saiManagerTable->routeManager().changeRoute<folly::IPAddressV4>(
      r2, r1, RouterID(0));
  EXPECT_FALSE(saiRouteHandle->nextHopGroupHandle());
}

TEST_F(RouteManagerTest, updateRouteDifferentNextHops) {
  tr2.nextHopInterfaces = tr1.nextHopInterfaces;
  auto r1 = makeRoute(tr1);
  auto r2 = makeRoute(tr2);
  saiManagerTable->routeManager().addRoute<folly::IPAddressV4>(r1, RouterID(0));
  saiManagerTable->routeManager().addRoute<folly::IPAddressV4>(r2, RouterID(0));
  tr1.nextHopInterfaces.push_back(testInterfaces.at(4));
  tr1.nextHopInterfaces.push_back(testInterfaces.at(5));
  auto r3 = makeRoute(tr1);
  saiManagerTable->routeManager().changeRoute<folly::IPAddressV4>(
      r1, r3, RouterID(0));
  auto routeEntry1 =
      saiManagerTable->routeManager().routeEntryFromSwRoute(RouterID(0), r1);
  SaiRouteHandle* saiRouteHandle1 =
      saiManagerTable->routeManager().getRouteHandle(routeEntry1);
  auto nexthopGroupHandle1 = saiRouteHandle1->nextHopGroupHandle();
  auto count = nexthopGroupHandle1->nextHopGroupSize();
  EXPECT_EQ(count, 6);
  auto routeEntry2 =
      saiManagerTable->routeManager().routeEntryFromSwRoute(RouterID(0), r2);
  SaiRouteHandle* saiRouteHandle2 =
      saiManagerTable->routeManager().getRouteHandle(routeEntry2);
  auto nexthopGroupHandle2 = saiRouteHandle2->nextHopGroupHandle();
  count = nexthopGroupHandle2->nextHopGroupSize();
  EXPECT_EQ(count, 4);
}

TEST_F(RouteManagerTest, updateCpuRoutetoNextHopRoute) {
  RouteFields<folly::IPAddressV4>::Prefix destination(
      tr1.destination.first.asV4(), tr1.destination.second);
  auto r1 = std::make_shared<Route<folly::IPAddressV4>>(
      RouteV4::makeThrift(destination));
  RouteNextHopEntry entry(
      RouteForwardAction::TO_CPU, AdminDistance::STATIC_ROUTE);
  r1->update(ClientID{42}, entry);
  r1->setResolved(entry);
  saiManagerTable->routeManager().addRoute<folly::IPAddressV4>(r1, RouterID(0));
  auto routeEntry =
      saiManagerTable->routeManager().routeEntryFromSwRoute(RouterID(0), r1);
  SaiRouteHandle* saiRouteHandle =
      saiManagerTable->routeManager().getRouteHandle(routeEntry);
  auto r2 = makeRoute(tr1);
  saiManagerTable->routeManager().changeRoute<folly::IPAddressV4>(
      r1, r2, RouterID(0));
  auto nexthopGroupHandle2 = saiRouteHandle->nextHopGroupHandle();

  auto count = nexthopGroupHandle2->nextHopGroupSize();
  EXPECT_EQ(count, 4);
  saiManagerTable->routeManager().changeRoute<folly::IPAddressV4>(
      r2, r1, RouterID(0));
  EXPECT_FALSE(saiRouteHandle->nextHopGroupHandle());
}

/*
 * Test for ToMe routes doesn't want to do all the setup, because
 * setting up the router interfaces will result in creating ToMeRoutes
 * which conflicts with this more targeted test.
 */
class ToMeRouteTest : public ManagerTestBase {
 public:
  void SetUp() override {
    setupStage = SetupStage::BLANK;
    ManagerTestBase::SetUp();
  }
};

TEST_F(ToMeRouteTest, toMeRoutes) {
  auto swInterface = makeInterface(testInterfaces.at(0));
  auto toMeRoutes =
      saiManagerTable->routeManager().makeInterfaceToMeRoutes(swInterface);
  EXPECT_EQ(toMeRoutes.size(), 1);
  const auto& toMeRoute = toMeRoutes.at(0);
  EXPECT_EQ(
      GET_ATTR(Route, PacketAction, toMeRoute->attributes()),
      SAI_PACKET_ACTION_FORWARD);

  auto switchId = saiManagerTable->switchManager().getSwitchSaiId();
  PortSaiId cpuPortId{saiApiTable->switchApi().getAttribute(
      switchId, SaiSwitchTraits::Attributes::CpuPort{})};
  EXPECT_EQ(GET_OPT_ATTR(Route, NextHopId, toMeRoute->attributes()), cpuPortId);
}

TEST_F(ToMeRouteTest, toMeRoutesSlash32) {
  // Make the interface a /32
  TestInterface& testIntf = testInterfaces.at(0);
  testIntf.subnet.second = 32;

  // Add ToMe routes
  auto swInterface = makeInterface(testIntf);
  auto toMeRoutes =
      saiManagerTable->routeManager().makeInterfaceToMeRoutes(swInterface);
  EXPECT_EQ(toMeRoutes.size(), 1);

  // Add the connected /32 route
  RouteFields<folly::IPAddressV4>::Prefix destination(
      testIntf.subnet.first.asV4(), testIntf.subnet.second);
  ResolvedNextHop nh{testIntf.routerIp, InterfaceID(testIntf.id), ECMP_WEIGHT};
  RouteNextHopEntry::NextHopSet swNextHops{nh};
  RouteNextHopEntry entry(swNextHops, AdminDistance::DIRECTLY_CONNECTED);
  auto r = std::make_shared<Route<folly::IPAddressV4>>(
      RouteV4::makeThrift(destination));
  r->update(ClientID::INTERFACE_ROUTE, entry);
  r->setResolved(entry);
  r->setConnected();
  saiManagerTable->routeManager().addRoute<folly::IPAddressV4>(r, RouterID(0));
}
