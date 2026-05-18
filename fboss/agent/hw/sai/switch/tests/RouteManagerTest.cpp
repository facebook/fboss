/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/HwSwitch.h"
#include "fboss/agent/hw/sai/api/AddressUtil.h"
#include "fboss/agent/hw/sai/switch/SaiFdbManager.h"
#include "fboss/agent/hw/sai/switch/SaiManagerTable.h"
#include "fboss/agent/hw/sai/switch/SaiNeighborManager.h"
#include "fboss/agent/hw/sai/switch/SaiNextHopGroupManager.h"
#include "fboss/agent/hw/sai/switch/SaiNextHopManager.h"
#include "fboss/agent/hw/sai/switch/SaiRouteManager.h"
#include "fboss/agent/hw/sai/switch/SaiRouterInterfaceManager.h"
#include "fboss/agent/hw/sai/switch/SaiSrv6SidListManager.h"
#include "fboss/agent/hw/sai/switch/SaiSrv6TunnelManager.h"
#include "fboss/agent/hw/sai/switch/SaiSwitch.h"
#include "fboss/agent/hw/sai/switch/SaiSwitchManager.h"
#include "fboss/agent/hw/sai/switch/SaiVirtualRouterManager.h"
#include "fboss/agent/hw/sai/switch/tests/ManagerTestBase.h"
#include "fboss/agent/state/NdpEntry.h"
#include "fboss/agent/state/Route.h"
#include "fboss/agent/state/RouteNextHop.h"
#include "fboss/agent/state/Srv6Tunnel.h"
#include "fboss/agent/test/utils/NextHopIdTestUtils.h"
#include "fboss/agent/types.h"

#include <gflags/gflags.h>
#include <optional>
#include <set>

using namespace facebook::fboss;
class RouteManagerTest : public ManagerTestBase {
 public:
  void SetUp() override {
    FLAGS_enable_nexthop_id_manager = true;
    FLAGS_resolve_nexthops_from_id = true;
    setupStage = SetupStage::PORT | SetupStage::VLAN | SetupStage::INTERFACE |
        SetupStage::NEIGHBOR | SetupStage::SYSTEM_PORT;
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

TEST_F(RouteManagerTest, verifySwitchStateConstruction) {
  auto newState = saiPlatform->getHwSwitch()->getProgrammedState()->clone();
  auto fib4 = newState->getFibsInfoMap()
                  ->getFibContainer(RouterID(0))
                  ->getFibV4()
                  ->modify(RouterID(0), &newState);
  for (int i = 0; i < 10; i++) {
    TestRoute nhopRouteV4;
    auto ip4 = folly::IPAddressV4(folly::sformat("1.1.1.{}", i));
    folly::CIDRNetwork c4{ip4, 32};
    nhopRouteV4.destination = c4;
    nhopRouteV4.nextHopInterfaces.push_back(testInterfaces.at(i));
    auto r1 = makeRoute(nhopRouteV4);
    RouteV4::Prefix pfx1{ip4, 32};
    fib4->addNode(pfx1.str(), std::move(r1));

    TestRoute ecmpRouteV4;
    ip4 = folly::IPAddressV4(folly::sformat("1.{}.0.0", i));
    folly::CIDRNetwork c42{ip4, 16};
    ecmpRouteV4.destination = c42;
    ecmpRouteV4.nextHopInterfaces.push_back(testInterfaces.at(0));
    ecmpRouteV4.nextHopInterfaces.push_back(testInterfaces.at(1));
    ecmpRouteV4.nextHopInterfaces.push_back(testInterfaces.at(2));
    ecmpRouteV4.nextHopInterfaces.push_back(testInterfaces.at(3));
    auto r2 = makeRoute(ecmpRouteV4);
    RouteV4::Prefix pfx2{ip4, 16};
    fib4->addNode(pfx2.str(), std::move(r2));
  }
  assignNextHopIdsToAllRoutes(nextHopIDManager_.get(), newState);
  newState->publish();
  applyNewState(newState);
  auto saiSwitch = static_cast<SaiSwitch*>(saiPlatform->getHwSwitch());
  auto hwState = saiSwitch->constructSwitchStateWithFib();
  auto swState = saiPlatform->getHwSwitch()->getProgrammedState();
  auto hwThrift = hwState->getFibsInfoMap()->toThrift();
  auto swThrift = swState->getFibsInfoMap()->toThrift();
  // constructSwitchStateWithFib doesn't reconstruct NextHopID maps.
  // Clear them from swState for comparison.
  for (auto& [key, fibInfo] : swThrift) {
    fibInfo.idToNextHop()->clear();
    fibInfo.idToNextHopIdSet()->clear();
  }
  ASSERT_EQ(hwThrift, swThrift);
}

TEST_F(RouteManagerTest, addRoute) {
  auto r = makeRoute(tr1);
  saiManagerTable->routeManager().addRoute<folly::IPAddressV4>(
      r, RouterID(0), getProgrammedState());
}

TEST_F(RouteManagerTest, addRouteSameNextHops) {
  tr2.nextHopInterfaces = tr1.nextHopInterfaces;
  auto r1 = makeRoute(tr1);
  auto r2 = makeRoute(tr2);
  saiManagerTable->routeManager().addRoute<folly::IPAddressV4>(
      r1, RouterID(0), getProgrammedState());
  saiManagerTable->routeManager().addRoute<folly::IPAddressV4>(
      r2, RouterID(0), getProgrammedState());
}

TEST_F(RouteManagerTest, addRouteDifferentNextHops) {
  auto r1 = makeRoute(tr1);
  tr2.nextHopInterfaces.push_back(testInterfaces.at(1));
  tr2.nextHopInterfaces.push_back(testInterfaces.at(3));
  auto r2 = makeRoute(tr2);
  saiManagerTable->routeManager().addRoute<folly::IPAddressV4>(
      r1, RouterID(0), getProgrammedState());
  saiManagerTable->routeManager().addRoute<folly::IPAddressV4>(
      r2, RouterID(0), getProgrammedState());
}

TEST_F(RouteManagerTest, addRouteOneNextHop) {
  tr1.nextHopInterfaces = {testInterfaces.at(1)};
  auto r = makeRoute(tr1);
  saiManagerTable->routeManager().addRoute<folly::IPAddressV4>(
      r, RouterID(0), getProgrammedState());
}

TEST_F(RouteManagerTest, updateRouteOneNextHopNoUpdate) {
  tr1.nextHopInterfaces = {testInterfaces.at(1)};
  auto r = makeRoute(tr1);
  saiManagerTable->routeManager().addRoute<folly::IPAddressV4>(
      r, RouterID(0), getProgrammedState());
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
  r1->updateClassID(
      std::make_optional<cfg::AclLookupClass>(
          cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_0));
  saiManagerTable->routeManager().changeRoute<folly::IPAddressV4>(
      r, r1, RouterID(0), getProgrammedState());
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
  saiManagerTable->routeManager().addRoute<folly::IPAddressV4>(
      r, RouterID(0), getProgrammedState());
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

TEST_F(RouteManagerTest, addRemoteInterfaceRoute) {
  TestInterface intf = testRemoteInterfaces.at(1);
  RouteFields<folly::IPAddressV4>::Prefix destination(
      intf.subnet.first.asV4(), intf.subnet.second);
  ResolvedNextHop nh{
      intf.routerIp, InterfaceID(kSysPortOffset + intf.id), ECMP_WEIGHT};
  RouteNextHopEntry::NextHopSet swNextHops{nh};
  RouteNextHopEntry entry(swNextHops, AdminDistance::DIRECTLY_CONNECTED);
  auto r = std::make_shared<Route<folly::IPAddressV4>>(
      RouteV4::makeThrift(destination));
  r->update(ClientID::INTERFACE_ROUTE, entry);
  allocateRouteNextHopIds(nextHopIDManager_.get(), entry);
  r->setResolved(entry);
  r->setConnected();
  saiManagerTable->routeManager().addRoute<folly::IPAddressV4>(
      r, RouterID(0), getProgrammedState());
  auto saiEntry =
      saiManagerTable->routeManager().routeEntryFromSwRoute(RouterID(0), r);
  auto saiRouteHandle =
      saiManagerTable->routeManager().getRouteHandle(saiEntry);
  auto saiRoute = saiRouteHandle->route;
  EXPECT_EQ(
      GET_ATTR(Route, PacketAction, saiRoute->attributes()),
      SAI_PACKET_ACTION_DROP);
}

/*
 * Test for connected route with a next hop interface that doesn't exist
 * in the local NPU (multi-NPU scenario),
 * the route should be set to DROP.
 */
TEST_F(RouteManagerTest, addConnectedRouteNonLocalInterfaceNpu) {
  // Create a route with a non-existent interface ID
  // This simulates a multi-NPU scenario where the interface is on another NPU
  constexpr int nonExistentInterfaceId = 999;
  folly::IPAddress routerIp{"192.168.100.1"};
  folly::CIDRNetwork subnet{routerIp, 24};

  RouteFields<folly::IPAddressV4>::Prefix destination(
      subnet.first.asV4(), subnet.second);
  ResolvedNextHop nh{
      routerIp, InterfaceID(nonExistentInterfaceId), ECMP_WEIGHT};
  RouteNextHopEntry::NextHopSet swNextHops{nh};
  RouteNextHopEntry entry(swNextHops, AdminDistance::DIRECTLY_CONNECTED);
  auto r = std::make_shared<Route<folly::IPAddressV4>>(
      RouteV4::makeThrift(destination));
  r->update(ClientID::INTERFACE_ROUTE, entry);
  allocateRouteNextHopIds(nextHopIDManager_.get(), entry);
  r->setResolved(entry);
  r->setConnected();
  saiManagerTable->routeManager().addRoute<folly::IPAddressV4>(
      r, RouterID(0), getProgrammedState());
  auto saiEntry =
      saiManagerTable->routeManager().routeEntryFromSwRoute(RouterID(0), r);
  auto saiRouteHandle =
      saiManagerTable->routeManager().getRouteHandle(saiEntry);
  auto saiRoute = saiRouteHandle->route;
  // For NPU switches, connected routes with non-local interfaces should be DROP
  EXPECT_EQ(
      GET_ATTR(Route, PacketAction, saiRoute->attributes()),
      SAI_PACKET_ACTION_DROP);
}

TEST_F(RouteManagerTest, updateRouteOneNextHopUpdate) {
  tr1.nextHopInterfaces = {testInterfaces.at(1)};
  auto r = makeRoute(tr1);
  saiManagerTable->routeManager().addRoute<folly::IPAddressV4>(
      r, RouterID(0), getProgrammedState());
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
      r, r1, RouterID(0), getProgrammedState());

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
  saiManagerTable->routeManager().addRoute<folly::IPAddressV4>(
      r, RouterID(0), getProgrammedState());
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
  allocateRouteNextHopIds(nextHopIDManager_.get(), entry);
  r->setResolved(entry);
  r->setConnected();
  saiManagerTable->routeManager().addRoute<folly::IPAddressV4>(
      r, RouterID(0), getProgrammedState());
}

TEST_F(RouteManagerTest, getRoute) {
  auto r = makeRoute(tr1);
  saiManagerTable->routeManager().addRoute<folly::IPAddressV4>(
      r, RouterID(0), getProgrammedState());
  auto entry =
      saiManagerTable->routeManager().routeEntryFromSwRoute(RouterID(0), r);
  SaiRouteHandle* saiRouteHandle =
      saiManagerTable->routeManager().getRouteHandle(entry);
  EXPECT_TRUE(saiRouteHandle);
}

TEST_F(RouteManagerTest, removeRoute) {
  auto r = makeRoute(tr1);
  saiManagerTable->routeManager().addRoute<folly::IPAddressV4>(
      r, RouterID(0), getProgrammedState());
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
  saiManagerTable->routeManager().addRoute<folly::IPAddressV4>(
      r, RouterID(0), getProgrammedState());
  EXPECT_THROW(
      saiManagerTable->routeManager().addRoute<folly::IPAddressV4>(
          r, RouterID(0), getProgrammedState()),
      FbossError);
}

TEST_F(RouteManagerTest, getNonexistentRoute) {
  auto r1 = makeRoute(tr1);
  tr2.nextHopInterfaces = tr1.nextHopInterfaces;
  auto r2 = makeRoute(tr2);
  saiManagerTable->routeManager().addRoute<folly::IPAddressV4>(
      r1, RouterID(0), getProgrammedState());
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
  saiManagerTable->routeManager().addRoute<folly::IPAddressV4>(
      r1, RouterID(0), getProgrammedState());
  EXPECT_THROW(
      saiManagerTable->routeManager().removeRoute(r2, RouterID(0)), FbossError);
}

TEST_F(RouteManagerTest, updateNonexistentRoute) {
  auto r1 = makeRoute(tr1);
  tr2.nextHopInterfaces = tr1.nextHopInterfaces;
  auto r2 = makeRoute(tr2);
  EXPECT_THROW(
      saiManagerTable->routeManager().changeRoute<folly::IPAddressV4>(
          r1, r2, RouterID(0), getProgrammedState()),
      FbossError);
}

TEST_F(RouteManagerTest, updateNexthopToNexthopRoute) {
  auto r1 = makeRoute(tr1);
  saiManagerTable->routeManager().addRoute<folly::IPAddressV4>(
      r1, RouterID(0), getProgrammedState());
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
      r1, r2, RouterID(0), getProgrammedState());
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
  saiManagerTable->routeManager().addRoute<folly::IPAddressV4>(
      r1, RouterID(0), getProgrammedState());
  auto routeEntry =
      saiManagerTable->routeManager().routeEntryFromSwRoute(RouterID(0), r1);
  SaiRouteHandle* saiRouteHandle =
      saiManagerTable->routeManager().getRouteHandle(routeEntry);
  auto r2 = makeRoute(tr1);
  saiManagerTable->routeManager().changeRoute<folly::IPAddressV4>(
      r1, r2, RouterID(0), getProgrammedState());
  auto nexthopGroupHandle2 = saiRouteHandle->nextHopGroupHandle();
  auto count = nexthopGroupHandle2->nextHopGroupSize();
  EXPECT_EQ(count, 4);
  saiManagerTable->routeManager().changeRoute<folly::IPAddressV4>(
      r2, r1, RouterID(0), getProgrammedState());
  EXPECT_FALSE(saiRouteHandle->nextHopGroupHandle());
}

TEST_F(RouteManagerTest, updateRouteDifferentNextHops) {
  tr2.nextHopInterfaces = tr1.nextHopInterfaces;
  auto r1 = makeRoute(tr1);
  auto r2 = makeRoute(tr2);
  saiManagerTable->routeManager().addRoute<folly::IPAddressV4>(
      r1, RouterID(0), getProgrammedState());
  saiManagerTable->routeManager().addRoute<folly::IPAddressV4>(
      r2, RouterID(0), getProgrammedState());
  tr1.nextHopInterfaces.push_back(testInterfaces.at(4));
  tr1.nextHopInterfaces.push_back(testInterfaces.at(5));
  auto r3 = makeRoute(tr1);
  saiManagerTable->routeManager().changeRoute<folly::IPAddressV4>(
      r1, r3, RouterID(0), getProgrammedState());
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
  saiManagerTable->routeManager().addRoute<folly::IPAddressV4>(
      r1, RouterID(0), getProgrammedState());
  auto routeEntry =
      saiManagerTable->routeManager().routeEntryFromSwRoute(RouterID(0), r1);
  SaiRouteHandle* saiRouteHandle =
      saiManagerTable->routeManager().getRouteHandle(routeEntry);
  auto r2 = makeRoute(tr1);
  saiManagerTable->routeManager().changeRoute<folly::IPAddressV4>(
      r1, r2, RouterID(0), getProgrammedState());
  auto nexthopGroupHandle2 = saiRouteHandle->nextHopGroupHandle();

  auto count = nexthopGroupHandle2->nextHopGroupSize();
  EXPECT_EQ(count, 4);
  saiManagerTable->routeManager().changeRoute<folly::IPAddressV4>(
      r2, r1, RouterID(0), getProgrammedState());
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
  allocateRouteNextHopIds(nextHopIDManager_.get(), entry);
  r->setResolved(entry);
  r->setConnected();
  saiManagerTable->routeManager().addRoute<folly::IPAddressV4>(
      r, RouterID(0), getProgrammedState());
}

#if SAI_API_VERSION >= SAI_VERSION(1, 12, 0)
class Srv6RouteTest : public ManagerTestBase {
 public:
  void SetUp() override {
    setupStage = SetupStage::PORT | SetupStage::VLAN | SetupStage::INTERFACE;
    ManagerTestBase::SetUp();
    intf0 = testInterfaces[0];
    intf1 = testInterfaces[1];
    intf2 = testInterfaces[2];
    resolveArp(intf0.id, intf0.remoteHosts[0]);
    resolveArp(intf1.id, intf1.remoteHosts[0]);
    resolveArp(intf2.id, intf2.remoteHosts[0]);
  }

  std::shared_ptr<Srv6Tunnel> makeSrv6Tunnel(
      const std::string& tunnelId,
      uint32_t intfID) {
    auto tunnel = std::make_shared<Srv6Tunnel>(tunnelId);
    tunnel->setType(TunnelType::SRV6_ENCAP);
    tunnel->setUnderlayIntfId(InterfaceID(intfID));
    tunnel->setSrcIP(folly::IPAddressV6("2001:db8::1"));
    tunnel->setTTLMode(cfg::TunnelMode::PIPE);
    tunnel->setDscpMode(cfg::TunnelMode::UNIFORM);
    tunnel->setEcnMode(cfg::TunnelMode::UNIFORM);
    return tunnel;
  }

  ResolvedNextHop makeSrv6NextHop(
      const TestInterface& testInterface,
      const std::string& tunnelId) const {
    const auto& remote = testInterface.remoteHosts.at(0);
    return makeSrv6NextHop(testInterface, remote.ip, tunnelId);
  }

  ResolvedNextHop makeSrv6NextHop(
      const TestInterface& testInterface,
      const folly::IPAddress& nextHopIp,
      const std::string& tunnelId) const {
    return ResolvedNextHop{
        nextHopIp,
        InterfaceID(testInterface.id),
        ECMP_WEIGHT,
        std::nullopt, // labelForwardingAction
        std::nullopt, // disableTTLDecrement
        std::nullopt, // topologyInfo
        std::nullopt, // adjustedWeight
        srv6SegmentList(),
        TunnelType::SRV6_ENCAP,
        tunnelId};
  }

  std::vector<folly::IPAddressV6> srv6SegmentList() const {
    return {
        folly::IPAddressV6("2001:db8::10"), folly::IPAddressV6("2001:db8::20")};
  }

  std::shared_ptr<Route<folly::IPAddressV4>> makeSrv6Route(
      const folly::CIDRNetwork& destination,
      const ResolvedNextHop& swNextHop) const {
    RouteNextHopEntry::NextHopSet swNextHops{swNextHop};
    RouteNextHopEntry entry(swNextHops, AdminDistance::STATIC_ROUTE);
    RouteFields<folly::IPAddressV4>::Prefix prefix(
        destination.first.asV4(), destination.second);
    auto r = std::make_shared<Route<folly::IPAddressV4>>(
        Route<folly::IPAddressV4>::makeThrift(prefix));
    r->update(ClientID{42}, entry);
    allocateRouteNextHopIds(nextHopIDManager_.get(), entry);
    r->setResolved(entry);
    return r;
  }

  std::shared_ptr<Route<folly::IPAddressV4>> makeSrv6Route(
      const folly::CIDRNetwork& destination,
      const TestInterface& testInterface,
      const std::string& tunnelId) const {
    auto swNextHop = makeSrv6NextHop(testInterface, tunnelId);
    return makeSrv6Route(destination, swNextHop);
  }

  std::shared_ptr<Route<folly::IPAddressV6>> makeSrv6RouteV6(
      const folly::CIDRNetwork& destination,
      const ResolvedNextHop& swNextHop) const {
    RouteNextHopEntry::NextHopSet swNextHops{swNextHop};
    RouteNextHopEntry entry(swNextHops, AdminDistance::STATIC_ROUTE);
    RouteFields<folly::IPAddressV6>::Prefix prefix(
        destination.first.asV6(), destination.second);
    auto r = std::make_shared<Route<folly::IPAddressV6>>(
        Route<folly::IPAddressV6>::makeThrift(prefix));
    r->update(ClientID{42}, entry);
    allocateRouteNextHopIds(nextHopIDManager_.get(), entry);
    r->setResolved(entry);
    return r;
  }

  std::shared_ptr<Route<folly::IPAddressV4>> makeSrv6EcmpRoute(
      const folly::CIDRNetwork& destination,
      const std::vector<ResolvedNextHop>& nextHops) const {
    RouteNextHopEntry::NextHopSet swNextHops;
    for (const auto& nextHop : nextHops) {
      swNextHops.insert(nextHop);
    }
    RouteNextHopEntry entry(swNextHops, AdminDistance::STATIC_ROUTE);
    RouteFields<folly::IPAddressV4>::Prefix prefix(
        destination.first.asV4(), destination.second);
    auto r = std::make_shared<Route<folly::IPAddressV4>>(
        Route<folly::IPAddressV4>::makeThrift(prefix));
    r->update(ClientID{42}, entry);
    allocateRouteNextHopIds(nextHopIDManager_.get(), entry);
    r->setResolved(entry);
    return r;
  }

  std::shared_ptr<Route<folly::IPAddressV4>> makeSrv6EcmpRoute(
      const folly::CIDRNetwork& destination,
      const std::vector<
          std::pair<std::reference_wrapper<const TestInterface>, std::string>>&
          nextHops) const {
    std::vector<ResolvedNextHop> swNextHops;
    for (const auto& [intfRef, tunnelId] : nextHops) {
      swNextHops.push_back(makeSrv6NextHop(intfRef.get(), tunnelId));
    }
    return makeSrv6EcmpRoute(destination, swNextHops);
  }

  std::shared_ptr<Route<folly::IPAddressV6>> makeSrv6EcmpRouteV6(
      const folly::CIDRNetwork& destination,
      const std::vector<ResolvedNextHop>& nextHops) const {
    RouteNextHopEntry::NextHopSet swNextHops;
    for (const auto& nextHop : nextHops) {
      swNextHops.insert(nextHop);
    }
    RouteNextHopEntry entry(swNextHops, AdminDistance::STATIC_ROUTE);
    RouteFields<folly::IPAddressV6>::Prefix prefix(
        destination.first.asV6(), destination.second);
    auto r = std::make_shared<Route<folly::IPAddressV6>>(
        Route<folly::IPAddressV6>::makeThrift(prefix));
    r->update(ClientID{42}, entry);
    allocateRouteNextHopIds(nextHopIDManager_.get(), entry);
    r->setResolved(entry);
    return r;
  }

  void resolveNdp(
      const TestInterface& testInterface,
      const folly::IPAddressV6& ip) {
    resolveNdp(testInterface, ip, folly::MacAddress{"10:10:10:10:10:aa"});
  }

  void resolveNdp(
      const TestInterface& testInterface,
      const folly::IPAddressV6& ip,
      const folly::MacAddress& mac) {
    const auto& remote = testInterface.remoteHosts.at(0);
    auto ndpEntry = std::make_shared<NdpEntry>(
        ip,
        mac,
        PortDescriptor(PortID(remote.port.id)),
        InterfaceID(testInterface.id),
        state::NeighborEntryType::DYNAMIC_ENTRY);
    saiManagerTable->neighborManager().addNeighbor(ndpEntry);
    saiManagerTable->fdbManager().addFdbEntry(
        SaiPortDescriptor(PortID(remote.port.id)),
        InterfaceID(testInterface.id),
        mac,
        SAI_FDB_ENTRY_TYPE_STATIC,
        std::nullopt);
  }

  sai_object_id_t verifySrv6SidListAndNextHop(
      const ResolvedNextHop& swNextHop) const {
    auto rifHandle =
        saiManagerTable->routerInterfaceManager().getRouterInterfaceHandle(
            swNextHop.intfID().value());
    EXPECT_NE(rifHandle, nullptr);
    if (!rifHandle) {
      return SAI_NULL_OBJECT_ID;
    }

    const auto expectedSegments = toSaiIp6List(srv6SegmentList());
    SaiSrv6SidListTraits::AdapterHostKey sidListKey{
        SAI_SRV6_SIDLIST_TYPE_ENCAPS_RED,
        expectedSegments,
        rifHandle->adapterKey(),
        swNextHop.addr()};
    auto* sidListHandle =
        saiManagerTable->srv6SidListManager().getSrv6SidListHandle(sidListKey);
    EXPECT_NE(sidListHandle, nullptr);
    if (!sidListHandle) {
      return SAI_NULL_OBJECT_ID;
    }
    EXPECT_NE(sidListHandle->managedSidList->getSidList(), nullptr);
    if (!sidListHandle->managedSidList->getSidList()) {
      return SAI_NULL_OBJECT_ID;
    }

    const auto sidListId =
        sidListHandle->managedSidList->getSidList()->adapterKey();
    auto adapterHostKey = saiManagerTable->nextHopManager().getAdapterHostKey(
        swNextHop, sidListId);
    auto* srv6Key = std::get_if<SaiSrv6SidlistNextHopTraits::AdapterHostKey>(
        &adapterHostKey);
    EXPECT_NE(srv6Key, nullptr);
    if (!srv6Key) {
      return SAI_NULL_OBJECT_ID;
    }

    auto* managedNh =
        saiManagerTable->nextHopManager().getManagedNextHop(*srv6Key);
    EXPECT_NE(managedNh, nullptr);
    if (!managedNh) {
      return SAI_NULL_OBJECT_ID;
    }
    EXPECT_NE(managedNh->getSaiObject(), nullptr);
    if (!managedNh->getSaiObject()) {
      return SAI_NULL_OBJECT_ID;
    }
    EXPECT_EQ(
        GET_ATTR(
            Srv6SidlistNextHop,
            Srv6SidlistId,
            managedNh->getSaiObject()->attributes()),
        sidListId);

    auto gotSegments = saiApiTable->srv6Api().getAttribute(
        sidListId, SaiSrv6SidListTraits::Attributes::SegmentList{});
    EXPECT_EQ(gotSegments, expectedSegments);

    const auto& underlayNhOpt = managedNh->getUnderlayNextHop();
    EXPECT_TRUE(underlayNhOpt.has_value());
    if (!underlayNhOpt.has_value()) {
      return SAI_NULL_OBJECT_ID;
    }
    auto underlayIpNhop =
        std::get<std::shared_ptr<ManagedIpNextHop>>(*underlayNhOpt);
    EXPECT_NE(underlayIpNhop->getSaiObject(), nullptr);
    if (!underlayIpNhop->getSaiObject()) {
      return SAI_NULL_OBJECT_ID;
    }
    EXPECT_EQ(
        GET_ATTR(IpNextHop, Ip, underlayIpNhop->getSaiObject()->attributes()),
        swNextHop.addr());

    auto gotNextHopId = saiApiTable->srv6Api().getAttribute(
        sidListId, SaiSrv6SidListTraits::Attributes::NextHopId{});
    EXPECT_EQ(gotNextHopId, underlayIpNhop->getSaiObject()->adapterKey());
    return managedNh->getSaiObject()->adapterKey();
  }

  void verifySrv6NextHopAndSidList(
      const SaiRouteHandle* routeHandle,
      const ResolvedNextHop& swNextHop,
      sai_object_id_t expectedNextHopId) const {
    ASSERT_NE(routeHandle, nullptr);
    auto* routeSrv6NextHop =
        std::get_if<std::shared_ptr<ManagedRouteSrv6NextHop>>(
            &routeHandle->nexthopHandle_);
    ASSERT_NE(routeSrv6NextHop, nullptr);
    ASSERT_NE(*routeSrv6NextHop, nullptr);
    EXPECT_EQ((*routeSrv6NextHop)->adapterKey(), expectedNextHopId);
    EXPECT_EQ(
        GET_OPT_ATTR(Route, NextHopId, routeHandle->route->attributes()),
        expectedNextHopId);
    EXPECT_EQ(verifySrv6SidListAndNextHop(swNextHop), expectedNextHopId);
  }

  void verifySrv6NextHopGroupAndSidLists(
      const SaiRouteHandle* routeHandle,
      const std::vector<ResolvedNextHop>& swNextHops,
      sai_object_id_t expectedNextHopGroupId) const {
    ASSERT_NE(routeHandle, nullptr);
    auto nhgHandle = routeHandle->nextHopGroupHandle();
    ASSERT_NE(nhgHandle, nullptr);
    EXPECT_EQ(nhgHandle->adapterKey(), expectedNextHopGroupId);
    EXPECT_EQ(nhgHandle->nextHopGroupSize(), swNextHops.size());
    EXPECT_EQ(
        GET_OPT_ATTR(Route, NextHopId, routeHandle->route->attributes()),
        expectedNextHopGroupId);

    std::set<sai_object_id_t> nextHopIds;
    for (const auto& swNextHop : swNextHops) {
      nextHopIds.insert(verifySrv6SidListAndNextHop(swNextHop));
    }
    EXPECT_EQ(nextHopIds.size(), swNextHops.size());
  }

  TestInterface intf0;
  TestInterface intf1;
  TestInterface intf2;
};

TEST_F(Srv6RouteTest, addRouteWithSrv6NextHop) {
  auto swTunnel = makeSrv6Tunnel("srv6tunnel0", intf0.id);
  saiManagerTable->srv6TunnelManager().addSrv6Tunnel(swTunnel);

  folly::CIDRNetwork destination{folly::IPAddress("42.42.42.42"), 24};
  auto route = makeSrv6Route(destination, intf0, "srv6tunnel0");
  saiManagerTable->routeManager().addRoute<folly::IPAddressV4>(
      route, RouterID(0), getProgrammedState());

  // Verify route handle exists
  auto saiEntry =
      saiManagerTable->routeManager().routeEntryFromSwRoute(RouterID(0), route);
  auto* handle = saiManagerTable->routeManager().getRouteHandle(saiEntry);
  ASSERT_NE(handle, nullptr);

  // Verify the route's next hop handle is the SRv6 variant
  auto* srv6NextHopHandle =
      std::get_if<std::shared_ptr<ManagedRouteSrv6NextHop>>(
          &handle->nexthopHandle_);
  ASSERT_NE(srv6NextHopHandle, nullptr);
  EXPECT_NE(*srv6NextHopHandle, nullptr);

  // Verify the route SAI object has FORWARD action
  auto saiRoute = handle->route;
  EXPECT_EQ(
      GET_ATTR(Route, PacketAction, saiRoute->attributes()),
      SAI_PACKET_ACTION_FORWARD);
}

TEST_F(Srv6RouteTest, addRouteWithSrv6NextHopVerifySidList) {
  auto swTunnel = makeSrv6Tunnel("srv6tunnel0", intf0.id);
  saiManagerTable->srv6TunnelManager().addSrv6Tunnel(swTunnel);

  folly::CIDRNetwork destination{folly::IPAddress("42.42.42.42"), 24};
  auto route = makeSrv6Route(destination, intf0, "srv6tunnel0");
  saiManagerTable->routeManager().addRoute<folly::IPAddressV4>(
      route, RouterID(0), getProgrammedState());

  // Get the route handle and extract the SRv6 managed next hop
  auto saiEntry =
      saiManagerTable->routeManager().routeEntryFromSwRoute(RouterID(0), route);
  auto* handle = saiManagerTable->routeManager().getRouteHandle(saiEntry);
  ASSERT_NE(handle, nullptr);

  auto* srv6NextHopHandle =
      std::get_if<std::shared_ptr<ManagedRouteSrv6NextHop>>(
          &handle->nexthopHandle_);
  ASSERT_NE(srv6NextHopHandle, nullptr);
  ASSERT_NE(*srv6NextHopHandle, nullptr);

  // Look up the managed SRv6 next hop via adapter host key
  auto swNextHop = makeSrv6NextHop(intf0, "srv6tunnel0");
  auto srv6SidList =
      saiManagerTable->nextHopManager().createSrv6SidList(swNextHop);
  auto lookupSidListId = srv6SidList->adapterKey();
  auto adapterHostKey = saiManagerTable->nextHopManager().getAdapterHostKey(
      swNextHop, lookupSidListId);
  auto* srv6Key =
      std::get_if<SaiSrv6SidlistNextHopTraits::AdapterHostKey>(&adapterHostKey);
  ASSERT_NE(srv6Key, nullptr);

  auto* managedNh =
      saiManagerTable->nextHopManager().getManagedNextHop(*srv6Key);
  ASSERT_NE(managedNh, nullptr);

  // Verify SID list attributes via SAI API
  auto& sidListHandle = managedNh->getSrv6SidListHandle();
  ASSERT_NE(sidListHandle, nullptr);
  ASSERT_NE(sidListHandle->managedSidList->getSidList(), nullptr);
  auto sidListId = sidListHandle->managedSidList->getSidList()->adapterKey();

  auto gotSegments = saiApiTable->srv6Api().getAttribute(
      sidListId, SaiSrv6SidListTraits::Attributes::SegmentList{});
  auto expectedSegments = toSaiIp6List(
      {folly::IPAddressV6("2001:db8::10"), folly::IPAddressV6("2001:db8::20")});
  EXPECT_EQ(gotSegments, expectedSegments);

  auto gotType = saiApiTable->srv6Api().getAttribute(
      sidListId, SaiSrv6SidListTraits::Attributes::Type{});
  EXPECT_EQ(gotType, SAI_SRV6_SIDLIST_TYPE_ENCAPS_RED);

  // Verify NextHopId was set on the SID list to the underlay IP nhop
  ASSERT_NE(managedNh->getSaiObject(), nullptr);
  auto& underlayNhOpt = managedNh->getUnderlayNextHop();
  ASSERT_TRUE(underlayNhOpt.has_value());
  auto underlayIpNhop =
      std::get<std::shared_ptr<ManagedIpNextHop>>(*underlayNhOpt);
  ASSERT_NE(underlayIpNhop->getSaiObject(), nullptr);
  auto gotNextHopId = saiApiTable->srv6Api().getAttribute(
      sidListId, SaiSrv6SidListTraits::Attributes::NextHopId{});
  EXPECT_EQ(gotNextHopId, underlayIpNhop->getSaiObject()->adapterKey());
}

TEST_F(Srv6RouteTest, removeRouteWithSrv6NextHop) {
  auto swTunnel = makeSrv6Tunnel("srv6tunnel0", intf0.id);
  saiManagerTable->srv6TunnelManager().addSrv6Tunnel(swTunnel);

  folly::CIDRNetwork destination{folly::IPAddress("42.42.42.42"), 24};
  auto route = makeSrv6Route(destination, intf0, "srv6tunnel0");
  saiManagerTable->routeManager().addRoute<folly::IPAddressV4>(
      route, RouterID(0), getProgrammedState());

  auto saiEntry =
      saiManagerTable->routeManager().routeEntryFromSwRoute(RouterID(0), route);
  ASSERT_NE(saiManagerTable->routeManager().getRouteHandle(saiEntry), nullptr);

  saiManagerTable->routeManager().removeRoute(route, RouterID(0));
  EXPECT_EQ(saiManagerTable->routeManager().getRouteHandle(saiEntry), nullptr);
}

TEST_F(Srv6RouteTest, addRouteWithSrv6NextHopGroup) {
  // Create tunnels for each interface
  auto swTunnel0 = makeSrv6Tunnel("srv6tunnel0", intf0.id);
  auto swTunnel1 = makeSrv6Tunnel("srv6tunnel1", intf1.id);
  auto swTunnel2 = makeSrv6Tunnel("srv6tunnel2", intf2.id);
  saiManagerTable->srv6TunnelManager().addSrv6Tunnel(swTunnel0);
  saiManagerTable->srv6TunnelManager().addSrv6Tunnel(swTunnel1);
  saiManagerTable->srv6TunnelManager().addSrv6Tunnel(swTunnel2);

  folly::CIDRNetwork destination{folly::IPAddress("42.42.42.42"), 24};
  auto route = makeSrv6EcmpRoute(
      destination,
      {{std::cref(intf0), "srv6tunnel0"},
       {std::cref(intf1), "srv6tunnel1"},
       {std::cref(intf2), "srv6tunnel2"}});
  saiManagerTable->routeManager().addRoute<folly::IPAddressV4>(
      route, RouterID(0), getProgrammedState());

  // Verify route handle exists
  auto saiEntry =
      saiManagerTable->routeManager().routeEntryFromSwRoute(RouterID(0), route);
  auto* handle = saiManagerTable->routeManager().getRouteHandle(saiEntry);
  ASSERT_NE(handle, nullptr);

  // Verify the route uses a next hop group (not single next hop)
  auto nhgHandle = handle->nextHopGroupHandle();
  ASSERT_NE(nhgHandle, nullptr);
  EXPECT_EQ(nhgHandle->nextHopGroupSize(), 3);

  // Verify the route SAI object has FORWARD action
  auto saiRoute = handle->route;
  EXPECT_EQ(
      GET_ATTR(Route, PacketAction, saiRoute->attributes()),
      SAI_PACKET_ACTION_FORWARD);
}

TEST_F(Srv6RouteTest, addRouteWithSrv6NextHopGroupVerifySidLists) {
  auto swTunnel0 = makeSrv6Tunnel("srv6tunnel0", intf0.id);
  auto swTunnel1 = makeSrv6Tunnel("srv6tunnel1", intf1.id);
  saiManagerTable->srv6TunnelManager().addSrv6Tunnel(swTunnel0);
  saiManagerTable->srv6TunnelManager().addSrv6Tunnel(swTunnel1);

  folly::CIDRNetwork destination{folly::IPAddress("42.42.42.42"), 24};
  auto route = makeSrv6EcmpRoute(
      destination,
      {{std::cref(intf0), "srv6tunnel0"}, {std::cref(intf1), "srv6tunnel1"}});
  saiManagerTable->routeManager().addRoute<folly::IPAddressV4>(
      route, RouterID(0), getProgrammedState());

  // Verify next hop group exists with 2 members
  auto saiEntry =
      saiManagerTable->routeManager().routeEntryFromSwRoute(RouterID(0), route);
  auto* handle = saiManagerTable->routeManager().getRouteHandle(saiEntry);
  ASSERT_NE(handle, nullptr);
  auto nhgHandle = handle->nextHopGroupHandle();
  ASSERT_NE(nhgHandle, nullptr);
  EXPECT_EQ(nhgHandle->nextHopGroupSize(), 2);

  // Verify SID list for each next hop via SaiSrv6SidListManager
  auto segmentList = toSaiIp6List(
      {folly::IPAddressV6("2001:db8::10"), folly::IPAddressV6("2001:db8::20")});
  for (const auto& intf : {std::cref(intf0), std::cref(intf1)}) {
    auto rifHandle =
        saiManagerTable->routerInterfaceManager().getRouterInterfaceHandle(
            InterfaceID(intf.get().id));
    ASSERT_NE(rifHandle, nullptr);
    auto rifId = rifHandle->adapterKey();
    folly::IPAddress ip = intf.get().remoteHosts.at(0).ip;

    SaiSrv6SidListTraits::AdapterHostKey sidListKey{
        SAI_SRV6_SIDLIST_TYPE_ENCAPS_RED, segmentList, rifId, ip};
    auto* sidListHandle =
        saiManagerTable->srv6SidListManager().getSrv6SidListHandle(sidListKey);
    ASSERT_NE(sidListHandle, nullptr);
    ASSERT_NE(sidListHandle->managedSidList->getSidList(), nullptr);

    auto sidListId = sidListHandle->managedSidList->getSidList()->adapterKey();
    auto gotSegments = saiApiTable->srv6Api().getAttribute(
        sidListId, SaiSrv6SidListTraits::Attributes::SegmentList{});
    EXPECT_EQ(gotSegments, segmentList);

    auto gotType = saiApiTable->srv6Api().getAttribute(
        sidListId, SaiSrv6SidListTraits::Attributes::Type{});
    EXPECT_EQ(gotType, SAI_SRV6_SIDLIST_TYPE_ENCAPS_RED);
  }
}

TEST_F(Srv6RouteTest, addV4AndV6RoutesWithSameLinkLocalSrv6NextHop) {
  auto swTunnel = makeSrv6Tunnel("srv6tunnel0", intf0.id);
  saiManagerTable->srv6TunnelManager().addSrv6Tunnel(swTunnel);

  const folly::IPAddressV6 linkLocalNextHop{"fe80::1"};
  resolveNdp(intf0, linkLocalNextHop);
  auto swNextHop =
      makeSrv6NextHop(intf0, folly::IPAddress(linkLocalNextHop), "srv6tunnel0");

  auto v4Route0 =
      makeSrv6Route({folly::IPAddress("42.42.42.42"), 24}, swNextHop);
  auto v4Route1 =
      makeSrv6Route({folly::IPAddress("43.43.43.43"), 24}, swNextHop);
  auto v6Route0 =
      makeSrv6RouteV6({folly::IPAddress("2401:db8::"), 64}, swNextHop);
  auto v6Route1 =
      makeSrv6RouteV6({folly::IPAddress("2401:db8:1::"), 64}, swNextHop);
  saiManagerTable->routeManager().addRoute<folly::IPAddressV4>(
      v4Route0, RouterID(0), getProgrammedState());
  saiManagerTable->routeManager().addRoute<folly::IPAddressV4>(
      v4Route1, RouterID(0), getProgrammedState());
  saiManagerTable->routeManager().addRoute<folly::IPAddressV6>(
      v6Route0, RouterID(0), getProgrammedState());
  saiManagerTable->routeManager().addRoute<folly::IPAddressV6>(
      v6Route1, RouterID(0), getProgrammedState());

  auto* v4Handle0 = saiManagerTable->routeManager().getRouteHandle(
      saiManagerTable->routeManager().routeEntryFromSwRoute(
          RouterID(0), v4Route0));
  auto* v4Handle1 = saiManagerTable->routeManager().getRouteHandle(
      saiManagerTable->routeManager().routeEntryFromSwRoute(
          RouterID(0), v4Route1));
  auto* v6Handle0 = saiManagerTable->routeManager().getRouteHandle(
      saiManagerTable->routeManager().routeEntryFromSwRoute(
          RouterID(0), v6Route0));
  auto* v6Handle1 = saiManagerTable->routeManager().getRouteHandle(
      saiManagerTable->routeManager().routeEntryFromSwRoute(
          RouterID(0), v6Route1));
  ASSERT_NE(v4Handle0, nullptr);
  ASSERT_NE(v4Handle1, nullptr);
  ASSERT_NE(v6Handle0, nullptr);
  ASSERT_NE(v6Handle1, nullptr);

  const auto expectedNextHopId = v4Handle0->nextHopAdapterKey();
  EXPECT_EQ(v4Handle1->nextHopAdapterKey(), expectedNextHopId);
  EXPECT_EQ(v6Handle0->nextHopAdapterKey(), expectedNextHopId);
  EXPECT_EQ(v6Handle1->nextHopAdapterKey(), expectedNextHopId);
  verifySrv6NextHopAndSidList(v4Handle0, swNextHop, expectedNextHopId);
  verifySrv6NextHopAndSidList(v4Handle1, swNextHop, expectedNextHopId);
  verifySrv6NextHopAndSidList(v6Handle0, swNextHop, expectedNextHopId);
  verifySrv6NextHopAndSidList(v6Handle1, swNextHop, expectedNextHopId);
}

TEST_F(Srv6RouteTest, addV4AndV6RoutesWithSameLinkLocalSrv6NextHopGroup) {
  auto swTunnel0 = makeSrv6Tunnel("srv6tunnel0", intf0.id);
  auto swTunnel1 = makeSrv6Tunnel("srv6tunnel1", intf1.id);
  auto swTunnel2 = makeSrv6Tunnel("srv6tunnel2", intf2.id);
  saiManagerTable->srv6TunnelManager().addSrv6Tunnel(swTunnel0);
  saiManagerTable->srv6TunnelManager().addSrv6Tunnel(swTunnel1);
  saiManagerTable->srv6TunnelManager().addSrv6Tunnel(swTunnel2);

  const folly::IPAddressV6 linkLocalNextHop{"fe80::1"};
  resolveNdp(intf0, linkLocalNextHop, folly::MacAddress{"10:10:10:10:10:aa"});
  resolveNdp(intf1, linkLocalNextHop, folly::MacAddress{"10:10:10:10:10:aa"});
  resolveNdp(intf2, linkLocalNextHop, folly::MacAddress{"10:10:10:10:10:bb"});
  std::vector<ResolvedNextHop> swNextHops{
      makeSrv6NextHop(intf0, folly::IPAddress(linkLocalNextHop), "srv6tunnel0"),
      makeSrv6NextHop(intf1, folly::IPAddress(linkLocalNextHop), "srv6tunnel1"),
      makeSrv6NextHop(
          intf2, folly::IPAddress(linkLocalNextHop), "srv6tunnel2")};

  auto v4Route0 =
      makeSrv6EcmpRoute({folly::IPAddress("42.42.42.42"), 24}, swNextHops);
  auto v4Route1 =
      makeSrv6EcmpRoute({folly::IPAddress("43.43.43.43"), 24}, swNextHops);
  auto v6Route0 =
      makeSrv6EcmpRouteV6({folly::IPAddress("2401:db8::"), 64}, swNextHops);
  auto v6Route1 =
      makeSrv6EcmpRouteV6({folly::IPAddress("2401:db8:1::"), 64}, swNextHops);
  saiManagerTable->routeManager().addRoute<folly::IPAddressV4>(
      v4Route0, RouterID(0), getProgrammedState());
  saiManagerTable->routeManager().addRoute<folly::IPAddressV4>(
      v4Route1, RouterID(0), getProgrammedState());
  saiManagerTable->routeManager().addRoute<folly::IPAddressV6>(
      v6Route0, RouterID(0), getProgrammedState());
  saiManagerTable->routeManager().addRoute<folly::IPAddressV6>(
      v6Route1, RouterID(0), getProgrammedState());

  auto* v4Handle0 = saiManagerTable->routeManager().getRouteHandle(
      saiManagerTable->routeManager().routeEntryFromSwRoute(
          RouterID(0), v4Route0));
  auto* v4Handle1 = saiManagerTable->routeManager().getRouteHandle(
      saiManagerTable->routeManager().routeEntryFromSwRoute(
          RouterID(0), v4Route1));
  auto* v6Handle0 = saiManagerTable->routeManager().getRouteHandle(
      saiManagerTable->routeManager().routeEntryFromSwRoute(
          RouterID(0), v6Route0));
  auto* v6Handle1 = saiManagerTable->routeManager().getRouteHandle(
      saiManagerTable->routeManager().routeEntryFromSwRoute(
          RouterID(0), v6Route1));
  ASSERT_NE(v4Handle0, nullptr);
  ASSERT_NE(v4Handle1, nullptr);
  ASSERT_NE(v6Handle0, nullptr);
  ASSERT_NE(v6Handle1, nullptr);

  const auto expectedNextHopGroupId = v4Handle0->nextHopAdapterKey();
  EXPECT_EQ(v4Handle1->nextHopAdapterKey(), expectedNextHopGroupId);
  EXPECT_EQ(v6Handle0->nextHopAdapterKey(), expectedNextHopGroupId);
  EXPECT_EQ(v6Handle1->nextHopAdapterKey(), expectedNextHopGroupId);
  verifySrv6NextHopGroupAndSidLists(
      v4Handle0, swNextHops, expectedNextHopGroupId);
  verifySrv6NextHopGroupAndSidLists(
      v4Handle1, swNextHops, expectedNextHopGroupId);
  verifySrv6NextHopGroupAndSidLists(
      v6Handle0, swNextHops, expectedNextHopGroupId);
  verifySrv6NextHopGroupAndSidLists(
      v6Handle1, swNextHops, expectedNextHopGroupId);
}

TEST_F(Srv6RouteTest, removeRouteWithSrv6NextHopGroup) {
  auto swTunnel0 = makeSrv6Tunnel("srv6tunnel0", intf0.id);
  auto swTunnel1 = makeSrv6Tunnel("srv6tunnel1", intf1.id);
  saiManagerTable->srv6TunnelManager().addSrv6Tunnel(swTunnel0);
  saiManagerTable->srv6TunnelManager().addSrv6Tunnel(swTunnel1);

  folly::CIDRNetwork destination{folly::IPAddress("42.42.42.42"), 24};
  auto route = makeSrv6EcmpRoute(
      destination,
      {{std::cref(intf0), "srv6tunnel0"}, {std::cref(intf1), "srv6tunnel1"}});
  saiManagerTable->routeManager().addRoute<folly::IPAddressV4>(
      route, RouterID(0), getProgrammedState());

  auto saiEntry =
      saiManagerTable->routeManager().routeEntryFromSwRoute(RouterID(0), route);
  ASSERT_NE(saiManagerTable->routeManager().getRouteHandle(saiEntry), nullptr);

  saiManagerTable->routeManager().removeRoute(route, RouterID(0));
  EXPECT_EQ(saiManagerTable->routeManager().getRouteHandle(saiEntry), nullptr);
}

TEST_F(Srv6RouteTest, createSrv6SidList) {
  auto swNextHop = makeSrv6NextHop(intf0, "srv6tunnel0");
  auto sidList = saiManagerTable->nextHopManager().createSrv6SidList(swNextHop);
  ASSERT_NE(sidList, nullptr);

  // Verify SID list attributes
  auto sidListId = sidList->adapterKey();
  auto gotSegments = saiApiTable->srv6Api().getAttribute(
      sidListId, SaiSrv6SidListTraits::Attributes::SegmentList{});
  auto expectedSegments = toSaiIp6List(
      {folly::IPAddressV6("2001:db8::10"), folly::IPAddressV6("2001:db8::20")});
  EXPECT_EQ(gotSegments, expectedSegments);

  auto gotType = saiApiTable->srv6Api().getAttribute(
      sidListId, SaiSrv6SidListTraits::Attributes::Type{});
  EXPECT_EQ(gotType, SAI_SRV6_SIDLIST_TYPE_ENCAPS_RED);
}

TEST_F(Srv6RouteTest, createSrv6SidListThrowsOnEmptySegmentList) {
  const auto& remote = intf0.remoteHosts.at(0);
  ResolvedNextHop nhNoSrv6{remote.ip, InterfaceID(intf0.id), ECMP_WEIGHT};
  EXPECT_THROW(
      saiManagerTable->nextHopManager().createSrv6SidList(nhNoSrv6),
      FbossError);
}
#endif
