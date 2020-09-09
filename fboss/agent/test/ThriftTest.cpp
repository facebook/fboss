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
#include "fboss/agent/SwSwitch.h"
#include "fboss/agent/ThriftHandler.h"
#include "fboss/agent/hw/mock/MockPlatform.h"
#include "fboss/agent/state/Route.h"
#include "fboss/agent/state/RouteUpdater.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/test/HwTestHandle.h"
#include "fboss/agent/test/TestUtils.h"

#include <folly/IPAddress.h>
#include <gtest/gtest.h>
#include <thrift/lib/cpp/util/EnumUtils.h>

using namespace facebook::fboss;
using apache::thrift::TEnumTraits;
using cfg::PortSpeed;
using facebook::network::toBinaryAddress;
using folly::IPAddress;
using folly::IPAddressV4;
using folly::IPAddressV6;
using folly::StringPiece;
using std::shared_ptr;
using std::unique_ptr;
using testing::UnorderedElementsAreArray;

namespace {

unique_ptr<HwTestHandle> setupTestHandle() {
  auto state = testStateA();
  auto handle = createTestHandle(state);
  handle->getSw()->initialConfigApplied(std::chrono::steady_clock::now());
  return handle;
}

IpPrefix ipPrefix(StringPiece ip, int length) {
  IpPrefix result;
  result.ip = toBinaryAddress(IPAddress(ip));
  result.prefixLength = length;
  return result;
}

} // unnamed namespace

TEST(ThriftTest, getInterfaceDetail) {
  auto handle = setupTestHandle();
  auto sw = handle->getSw();

  ThriftHandler handler(sw);

  // Query the two interfaces configured by testStateA()
  InterfaceDetail info;
  handler.getInterfaceDetail(info, 1);
  EXPECT_EQ("interface1", *info.interfaceName_ref());
  EXPECT_EQ(1, *info.interfaceId_ref());
  EXPECT_EQ(1, *info.vlanId_ref());
  EXPECT_EQ(0, *info.routerId_ref());
  EXPECT_EQ("00:02:00:00:00:01", *info.mac_ref());
  std::vector<IpPrefix> expectedAddrs = {
      ipPrefix("10.0.0.1", 24),
      ipPrefix("192.168.0.1", 24),
      ipPrefix("2401:db00:2110:3001::0001", 64),
  };
  EXPECT_THAT(*info.address_ref(), UnorderedElementsAreArray(expectedAddrs));

  handler.getInterfaceDetail(info, 55);
  EXPECT_EQ("interface55", *info.interfaceName_ref());
  EXPECT_EQ(55, *info.interfaceId_ref());
  EXPECT_EQ(55, *info.vlanId_ref());
  EXPECT_EQ(0, *info.routerId_ref());
  EXPECT_EQ("00:02:00:00:00:55", *info.mac_ref());
  expectedAddrs = {
      ipPrefix("10.0.55.1", 24),
      ipPrefix("192.168.55.1", 24),
      ipPrefix("2401:db00:2110:3055::0001", 64),
  };
  EXPECT_THAT(*info.address_ref(), UnorderedElementsAreArray(expectedAddrs));

  // Calling getInterfaceDetail() on an unknown
  // interface should throw an FbossError.
  EXPECT_THROW(handler.getInterfaceDetail(info, 123), FbossError);
}

TEST(ThriftTest, assertPortSpeeds) {
  // We rely on the exact value of the port speeds for some
  // logic, so we want to ensure that these values don't change.
  for (const auto key : TEnumTraits<cfg::PortSpeed>::values) {
    switch (key) {
      case PortSpeed::DEFAULT:
        continue;
      case PortSpeed::GIGE:
        EXPECT_EQ(static_cast<int>(key), 1000);
        break;
      case PortSpeed::XG:
        EXPECT_EQ(static_cast<int>(key), 10000);
        break;
      case PortSpeed::TWENTYG:
        EXPECT_EQ(static_cast<int>(key), 20000);
        break;
      case PortSpeed::TWENTYFIVEG:
        EXPECT_EQ(static_cast<int>(key), 25000);
        break;
      case PortSpeed::FORTYG:
        EXPECT_EQ(static_cast<int>(key), 40000);
        break;
      case PortSpeed::FIFTYG:
        EXPECT_EQ(static_cast<int>(key), 50000);
        break;
      case PortSpeed::HUNDREDG:
        EXPECT_EQ(static_cast<int>(key), 100000);
        break;
      case PortSpeed::TWOHUNDREDG:
        EXPECT_EQ(static_cast<int>(key), 200000);
        break;
      case PortSpeed::FOURHUNDREDG:
        EXPECT_EQ(static_cast<int>(key), 400000);
        break;
    }
  }
}

TEST(ThriftTest, LinkLocalRoutes) {
  auto platform = createMockPlatform();
  auto stateV0 = testStateB();
  // Remove all linklocalroutes from stateV0 in order to clear all
  // linklocalroutes
  RouteUpdater updater(stateV0->getRouteTables());
  updater.delLinkLocalRoutes(RouterID(0));
  auto newRt = updater.updateDone();
  stateV0->resetRouteTables(newRt);
  cfg::SwitchConfig config;
  config.vlans_ref()->resize(1);
  *config.vlans_ref()[0].id_ref() = 1;
  config.interfaces_ref()->resize(1);
  *config.interfaces_ref()[0].intfID_ref() = 1;
  *config.interfaces_ref()[0].vlanID_ref() = 1;
  *config.interfaces_ref()[0].routerID_ref() = 0;
  config.interfaces_ref()[0].mac_ref() = "00:02:00:00:00:01";
  config.interfaces_ref()[0].ipAddresses_ref()->resize(3);
  config.interfaces_ref()[0].ipAddresses_ref()[0] = "10.0.0.1/24";
  config.interfaces_ref()[0].ipAddresses_ref()[1] = "192.168.0.1/24";
  config.interfaces_ref()[0].ipAddresses_ref()[2] =
      "2401:db00:2110:3001::0001/64";
  config.ports_ref()->resize(10);
  for (int i = 0; i < 10; i++) {
    auto port = i + 1;
    config.ports_ref()[i].logicalID_ref() = port;
    config.ports_ref()[i].name_ref() = folly::format("port{}", port).str();
    config.ports_ref()[i].state_ref() = cfg::PortState::DISABLED;
  }
  // Call applyThriftConfig
  auto stateV1 = publishAndApplyConfig(stateV0, &config, platform.get());
  stateV1->publish();
  // Verify that stateV1 contains the link local route
  shared_ptr<RouteTable> rt =
      stateV1->getRouteTables()->getRouteTableIf(RouterID(0));
  ASSERT_NE(nullptr, rt);
  // Link local addr.
  auto ip = IPAddressV6("fe80::");
  // Find longest match to link local addr.
  auto longestMatchRoute = rt->getRibV6()->longestMatch(ip);
  // Verify that a route is found
  ASSERT_NE(nullptr, longestMatchRoute);
  // Verify that the route is to link local addr.
  ASSERT_EQ(longestMatchRoute->prefix().network, ip);
}

std::unique_ptr<UnicastRoute> makeUnicastRoute(
    std::string prefixStr,
    std::string nxtHop,
    AdminDistance distance = AdminDistance::MAX_ADMIN_DISTANCE) {
  std::vector<std::string> vec;
  folly::split("/", prefixStr, vec);
  EXPECT_EQ(2, vec.size());
  auto nr = std::make_unique<UnicastRoute>();
  nr->dest.ip = toBinaryAddress(IPAddress(vec.at(0)));
  nr->dest.prefixLength = folly::to<uint8_t>(vec.at(1));
  nr->nextHopAddrs_ref()->push_back(toBinaryAddress(IPAddress(nxtHop)));
  nr->adminDistance_ref() = distance;
  return nr;
}

// Test for the ThriftHandler::syncFib method
TEST(ThriftTest, syncFib) {
  RouterID rid = RouterID(0);

  // Create a config
  cfg::SwitchConfig config;
  config.vlans_ref()->resize(1);
  *config.vlans_ref()[0].id_ref() = 1;
  config.interfaces_ref()->resize(1);
  *config.interfaces_ref()[0].intfID_ref() = 1;
  *config.interfaces_ref()[0].vlanID_ref() = 1;
  *config.interfaces_ref()[0].routerID_ref() = 0;
  config.interfaces_ref()[0].mac_ref() = "00:02:00:00:00:01";
  config.interfaces_ref()[0].ipAddresses_ref()->resize(3);
  config.interfaces_ref()[0].ipAddresses_ref()[0] = "10.0.0.1/24";
  config.interfaces_ref()[0].ipAddresses_ref()[1] = "192.168.0.19/24";
  config.interfaces_ref()[0].ipAddresses_ref()[2] =
      "2401:db00:2110:3001::0001/64";

  // Create a mock SwSwitch using the config, and wrap it in a ThriftHandler
  auto handle = createTestHandle(&config);
  auto sw = handle->getSw();
  sw->initialConfigApplied(std::chrono::steady_clock::now());
  sw->fibSynced();
  ThriftHandler handler(sw);

  //
  // Add a few BGP routes
  //

  auto cli1_nhop4 = "11.11.11.11";
  auto cli1_nhop6 = "11:11::0";
  auto cli2_nhop4 = "22.22.22.22";
  auto cli2_nhop6 = "22:22::0";
  auto cli3_nhop6 = "33:33::0";
  auto cli1_nhop6b = "44:44::0";

  // These routes will include nexthops from client 10 only
  auto prefixA4 = "7.1.0.0/16";
  auto prefixA6 = "aaaa:1::0/64";
  handler.addUnicastRoute(10, makeUnicastRoute(prefixA4, cli1_nhop4));
  handler.addUnicastRoute(10, makeUnicastRoute(prefixA6, cli1_nhop6));

  // This route will include nexthops from clients 10 and 20
  auto prefixB4 = "7.2.0.0/16";
  handler.addUnicastRoute(10, makeUnicastRoute(prefixB4, cli1_nhop4));
  handler.addUnicastRoute(20, makeUnicastRoute(prefixB4, cli2_nhop4));

  // This route will include nexthops from clients 10 and 20 and 30
  auto prefixC6 = "aaaa:3::0/64";
  handler.addUnicastRoute(10, makeUnicastRoute(prefixC6, cli1_nhop6));
  handler.addUnicastRoute(20, makeUnicastRoute(prefixC6, cli2_nhop6));
  handler.addUnicastRoute(30, makeUnicastRoute(prefixC6, cli3_nhop6));

  // These routes will not be used until fibSync happens.
  auto prefixD4 = "7.4.0.0/16";
  auto prefixD6 = "aaaa:4::0/64";

  //
  // Test the state of things before calling syncFib
  //

  // Make sure all the static and link-local routes are there
  auto tables2 = sw->getState()->getRouteTables();
  GET_ROUTE_V4(tables2, rid, "10.0.0.0/24");
  GET_ROUTE_V4(tables2, rid, "192.168.0.0/24");
  GET_ROUTE_V6(tables2, rid, "2401:db00:2110:3001::/64");
  GET_ROUTE_V6(tables2, rid, "fe80::/64");
  // Make sure the client 10&20&30 routes are there.
  GET_ROUTE_V4(tables2, rid, prefixA4);
  GET_ROUTE_V6(tables2, rid, prefixA6);
  GET_ROUTE_V4(tables2, rid, prefixB4);
  GET_ROUTE_V6(tables2, rid, prefixC6);
  // Make sure there are no more routes than the ones we just tested
  // plus the 1 default route that gets added automatically
  EXPECT_EQ(4 + 1, tables2->getRouteTable(rid)->getRibV4()->size());
  EXPECT_EQ(4 + 1, tables2->getRouteTable(rid)->getRibV6()->size());
  EXPECT_NO_ROUTE(tables2, rid, prefixD4);
  EXPECT_NO_ROUTE(tables2, rid, prefixD6);

  //
  // Now use syncFib to remove all the routes for client 10 and add
  // some new ones
  // Statics, link-locals, and clients 20 and 30 should remain unchanged.
  //

  auto newRoutes = std::make_unique<std::vector<UnicastRoute>>();
  UnicastRoute nr1 = *makeUnicastRoute(prefixC6, cli1_nhop6b).get();
  UnicastRoute nr2 = *makeUnicastRoute(prefixD6, cli1_nhop6b).get();
  UnicastRoute nr3 = *makeUnicastRoute(prefixD4, cli1_nhop4).get();
  newRoutes->push_back(nr1);
  newRoutes->push_back(nr2);
  newRoutes->push_back(nr3);
  handler.syncFib(10, std::move(newRoutes));

  //
  // Test the state of things after syncFib
  //

  // Make sure all the static and link-local routes are still there
  auto tables3 = sw->getState()->getRouteTables();
  GET_ROUTE_V4(tables3, rid, "10.0.0.0/24");
  GET_ROUTE_V4(tables3, rid, "192.168.0.0/24");
  GET_ROUTE_V6(tables3, rid, "2401:db00:2110:3001::/64");
  GET_ROUTE_V6(tables3, rid, "fe80::/64");

  // The prefixA* routes should have disappeared
  EXPECT_NO_ROUTE(tables3, rid, prefixA4);
  EXPECT_NO_ROUTE(tables3, rid, prefixA6);

  // The prefixB4 route should have client 20 only
  auto rt1 = GET_ROUTE_V4(tables3, rid, prefixB4);
  ASSERT_TRUE(rt1->getFields()->nexthopsmulti.isSame(
      ClientID(20),
      RouteNextHopEntry(
          makeNextHops({cli2_nhop4}), AdminDistance::MAX_ADMIN_DISTANCE)));
  auto bestNextHops = rt1->getBestEntry().second->getNextHopSet();
  EXPECT_EQ(IPAddress(cli2_nhop4), bestNextHops.begin()->addr());

  // The prefixC6 route should have clients 20 & 30, and a new value for
  // client 10
  auto rt2 = GET_ROUTE_V6(tables3, rid, prefixC6);
  ASSERT_TRUE(rt2->getFields()->nexthopsmulti.isSame(
      ClientID(20),
      RouteNextHopEntry(
          makeNextHops({cli2_nhop6}), AdminDistance::MAX_ADMIN_DISTANCE)));
  ASSERT_TRUE(rt2->getFields()->nexthopsmulti.isSame(
      ClientID(30),
      RouteNextHopEntry(
          makeNextHops({cli3_nhop6}), AdminDistance::MAX_ADMIN_DISTANCE)));
  ASSERT_TRUE(rt2->getFields()->nexthopsmulti.isSame(
      ClientID(10),
      RouteNextHopEntry(
          makeNextHops({cli1_nhop6b}), AdminDistance::MAX_ADMIN_DISTANCE)));

  // The prefixD4 and prefixD6 routes should have been created
  GET_ROUTE_V4(tables3, rid, prefixD4);
  GET_ROUTE_V6(tables3, rid, prefixD6);

  // Make sure there are no more routes (ie. the old ones were deleted)
  // + the one that gets added by default
  EXPECT_EQ(4 + 1, tables3->getRouteTable(rid)->getRibV4()->size());
  EXPECT_EQ(4 + 1, tables3->getRouteTable(rid)->getRibV6()->size());
}
