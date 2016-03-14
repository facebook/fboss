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
#include "fboss/agent/SwSwitch.h"
#include "fboss/agent/ThriftHandler.h"
#include "fboss/agent/test/TestUtils.h"
#include "fboss/agent/state/RouteUpdater.h"
#include "fboss/agent/hw/mock/MockPlatform.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/ApplyThriftConfig.h"
#include "fboss/agent/state/Route.h"

#include <folly/IPAddress.h>
#include <gtest/gtest.h>

using namespace facebook::fboss;
using folly::IPAddress;
using folly::IPAddressV6;
using folly::StringPiece;
using std::unique_ptr;
using std::shared_ptr;
using testing::UnorderedElementsAreArray;
using facebook::network::toBinaryAddress;
using cfg::PortSpeed;

namespace {

unique_ptr<SwSwitch> setupSwitch() {
  auto state = testStateA();
  auto sw = createMockSw(state);
  sw->initialConfigApplied(std::chrono::steady_clock::now());
  return sw;
}

IpPrefix ipPrefix(StringPiece ip, int length) {
  IpPrefix result;
  result.ip = toBinaryAddress(IPAddress(ip));
  result.prefixLength = length;
  return result;
}

} // unnamed namespace

TEST(ThriftTest, getInterfaceDetail) {
  auto sw = setupSwitch();
  ThriftHandler handler(sw.get());

  // Query the two interfaces configured by testStateA()
  InterfaceDetail info;
  handler.getInterfaceDetail(info, 1);
  EXPECT_EQ("interface1", info.interfaceName);
  EXPECT_EQ(1, info.interfaceId);
  EXPECT_EQ(1, info.vlanId);
  EXPECT_EQ(0, info.routerId);
  EXPECT_EQ("00:02:00:00:00:01", info.mac);
  std::vector<IpPrefix> expectedAddrs = {
    ipPrefix("10.0.0.1", 24),
    ipPrefix("192.168.0.1", 24),
    ipPrefix("2401:db00:2110:3001::0001", 64),
  };
  EXPECT_THAT(info.address, UnorderedElementsAreArray(expectedAddrs));

  handler.getInterfaceDetail(info, 55);
  EXPECT_EQ("interface55", info.interfaceName);
  EXPECT_EQ(55, info.interfaceId);
  EXPECT_EQ(55, info.vlanId);
  EXPECT_EQ(0, info.routerId);
  EXPECT_EQ("00:02:00:00:00:55", info.mac);
  expectedAddrs = {
    ipPrefix("10.0.55.1", 24),
    ipPrefix("192.168.55.1", 24),
    ipPrefix("2401:db00:2110:3055::0001", 64),
  };
  EXPECT_THAT(info.address, UnorderedElementsAreArray(expectedAddrs));

  // Calling getInterfaceDetail() on an unknown
  // interface should throw an FbossError.
  EXPECT_THROW(handler.getInterfaceDetail(info, 123), FbossError);
}


TEST(ThriftTest, assertPortSpeeds) {
  // We rely on the exact value of the port speeds for some
  // logic, so we want to ensure that these values don't change.
  EXPECT_EQ(static_cast<int>(PortSpeed::GIGE), 1000);
  EXPECT_EQ(static_cast<int>(PortSpeed::XG), 10000);
  EXPECT_EQ(static_cast<int>(PortSpeed::TWENTYG), 20000);
  EXPECT_EQ(static_cast<int>(PortSpeed::TWENTYFIVEG), 25000);
  EXPECT_EQ(static_cast<int>(PortSpeed::FORTYG), 40000);
  EXPECT_EQ(static_cast<int>(PortSpeed::FIFTYG), 50000);
  EXPECT_EQ(static_cast<int>(PortSpeed::HUNDREDG), 100000);
}

TEST(ThriftTest, LinkLocalRoutes) {
  MockPlatform platform;
  auto stateV0 = testStateB();
  // Remove all linklocalroutes from stateV0 in order to clear all
  // linklocalroutes
  RouteUpdater updater(stateV0->getRouteTables());
  updater.delLinkLocalRoutes(RouterID(0));
  auto newRt = updater.updateDone();
  stateV0->resetRouteTables(newRt);
  cfg::SwitchConfig config;
  config.vlans.resize(1);
  config.vlans[0].id = 1;
  config.interfaces.resize(1);
  config.interfaces[0].intfID = 1;
  config.interfaces[0].vlanID = 1;
  config.interfaces[0].routerID = 0;
  config.interfaces[0].__isset.mac = true;
  config.interfaces[0].mac = "00:02:00:00:00:01";
  config.interfaces[0].ipAddresses.resize(3);
  config.interfaces[0].ipAddresses[0] = "10.0.0.1/24";
  config.interfaces[0].ipAddresses[1] = "192.168.0.1/24";
  config.interfaces[0].ipAddresses[2] = "2401:db00:2110:3001::0001/64";
  // Call applyThriftConfig
  auto stateV1 = publishAndApplyConfig(stateV0, &config, &platform);
  stateV1->publish();
  // Verify that stateV1 contains the link local route
  shared_ptr<RouteTable> rt = stateV1->getRouteTables()->
                              getRouteTableIf(RouterID(0));
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
