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
#include "common/stats/MonotonicCounter.h"
#include "fboss/agent/AddressUtil.h"
#include "fboss/agent/ApplyThriftConfig.h"
#include "fboss/agent/FbossHwUpdateError.h"
#include "fboss/agent/SwSwitch.h"
#include "fboss/agent/ThriftHandler.h"
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/hw/mock/MockPlatform.h"
#include "fboss/agent/state/Port.h"
#include "fboss/agent/state/Route.h"

#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/test/CounterCache.h"
#include "fboss/agent/test/HwTestHandle.h"
#include "fboss/agent/test/RouteScaleGenerators.h"
#include "fboss/agent/test/TestUtils.h"

#include <folly/IPAddress.h>
#include <gtest/gtest.h>
#include <thrift/lib/cpp/util/EnumUtils.h>

using namespace facebook::fboss;
using namespace facebook::stats;
using apache::thrift::TEnumTraits;
using cfg::PortSpeed;
using facebook::network::toBinaryAddress;
using facebook::network::thrift::BinaryAddress;
using folly::IPAddress;
using folly::IPAddressV4;
using folly::IPAddressV6;
using folly::StringPiece;
using std::shared_ptr;
using std::unique_ptr;
using std::chrono::duration_cast;
using ::testing::_;
using ::testing::Return;
using testing::UnorderedElementsAreArray;

namespace {

IpPrefix ipPrefix(StringPiece ip, int length) {
  IpPrefix result;
  result.ip_ref() = toBinaryAddress(IPAddress(ip));
  result.prefixLength_ref() = length;
  return result;
}

IpPrefix ipPrefix(const folly::CIDRNetwork& nw) {
  IpPrefix result;
  result.ip_ref() = toBinaryAddress(nw.first);
  result.prefixLength_ref() = nw.second;
  return result;
}
} // unnamed namespace

class ThriftTest : public ::testing::Test {
 public:
  void SetUp() override {
    auto config = testConfigA();
    handle_ = createTestHandle(&config);
    sw_ = handle_->getSw();
    sw_->initialConfigApplied(std::chrono::steady_clock::now());
  }
  SwSwitch* sw_;
  std::unique_ptr<HwTestHandle> handle_;
};

TEST_F(ThriftTest, getInterfaceDetail) {
  ThriftHandler handler(this->sw_);

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
      ipPrefix("fe80::202:ff:fe00:1", 64),
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
      ipPrefix("fe80::202:ff:fe00:55", 64),
  };
  EXPECT_THAT(*info.address_ref(), UnorderedElementsAreArray(expectedAddrs));

  // Calling getInterfaceDetail() on an unknown
  // interface should throw an FbossError.
  EXPECT_THROW(handler.getInterfaceDetail(info, 123), FbossError);
}

TEST_F(ThriftTest, listHwObjects) {
  ThriftHandler handler(this->sw_);
  std::string out;
  std::vector<HwObjectType> in{HwObjectType::PORT};
  EXPECT_HW_CALL(this->sw_, listObjects(in, testing::_)).Times(1);
  handler.listHwObjects(
      out, std::make_unique<std::vector<HwObjectType>>(in), false);
}

TEST_F(ThriftTest, getHwDebugDump) {
  ThriftHandler handler(this->sw_);
  std::string out;
  EXPECT_HW_CALL(this->sw_, dumpDebugState(testing::_)).Times(1);
  // Mock getHwDebugDump doesn't write any thing so expect FbossError
  EXPECT_THROW(handler.getHwDebugDump(out), FbossError);
}

TEST(ThriftEnum, assertPortSpeeds) {
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

TEST_F(ThriftTest, LinkLocalRoutes) {
  // Link local addr.
  auto ip = IPAddressV6("fe80::");
  // Find longest match to link local addr.
  auto longestMatchRoute = findLongestMatchRoute(
      this->sw_->getRib(), RouterID(0), ip, this->sw_->getState());
  // Verify that a route is found. Link local route should always
  // be present
  ASSERT_NE(nullptr, longestMatchRoute);
  // Verify that the route is to link local addr.
  ASSERT_EQ(longestMatchRoute->prefix().network, ip);
}

TEST_F(ThriftTest, flushNonExistentNeighbor) {
  ThriftHandler handler(this->sw_);
  EXPECT_EQ(
      handler.flushNeighborEntry(
          std::make_unique<BinaryAddress>(
              toBinaryAddress(IPAddress("100.100.100.1"))),
          1),
      0);
  EXPECT_EQ(
      handler.flushNeighborEntry(
          std::make_unique<BinaryAddress>(
              toBinaryAddress(IPAddress("100::100"))),
          1),
      0);
}

TEST_F(ThriftTest, setPortState) {
  const PortID port1{1};
  ThriftHandler handler(this->sw_);
  handler.setPortState(port1, true);
  this->sw_->linkStateChanged(port1, true);
  waitForStateUpdates(this->sw_);

  auto port = this->sw_->getState()->getPorts()->getPortIf(port1);
  EXPECT_TRUE(port->isUp());
  EXPECT_TRUE(port->isEnabled());

  this->sw_->linkStateChanged(port1, false);
  handler.setPortState(port1, false);
  waitForStateUpdates(this->sw_);

  port = this->sw_->getState()->getPorts()->getPortIf(port1);
  EXPECT_FALSE(port->isUp());
  EXPECT_FALSE(port->isEnabled());
}

std::unique_ptr<UnicastRoute> makeUnicastRoute(
    std::string prefixStr,
    std::string nxtHop,
    AdminDistance distance = AdminDistance::MAX_ADMIN_DISTANCE,
    std::optional<RouteCounterID> counterID = std::nullopt) {
  std::vector<std::string> vec;
  folly::split("/", prefixStr, vec);
  EXPECT_EQ(2, vec.size());
  auto nr = std::make_unique<UnicastRoute>();
  nr->dest.ip = toBinaryAddress(IPAddress(vec.at(0)));
  nr->dest.prefixLength = folly::to<uint8_t>(vec.at(1));
  nr->nextHopAddrs_ref()->push_back(toBinaryAddress(IPAddress(nxtHop)));
  nr->adminDistance_ref() = distance;
  if (counterID.has_value()) {
    nr->counterID_ref() = *counterID;
  }
  return nr;
}

// Test for the ThriftHandler::syncFib method
TEST_F(ThriftTest, multipleClientSyncFib) {
  RouterID rid = RouterID(0);

  // Create a mock SwSwitch using the config, and wrap it in a ThriftHandler
  ThriftHandler handler(this->sw_);

  auto kIntf1 = InterfaceID(1);

  // Two clients - BGP and OPENR
  auto bgpClient = static_cast<int16_t>(ClientID::BGPD);
  auto openrClient = static_cast<int16_t>(ClientID::OPENR);
  auto bgpClientAdmin = this->sw_->clientIdToAdminDistance(bgpClient);
  auto openrClientAdmin = this->sw_->clientIdToAdminDistance(openrClient);

  // nhops to use
  auto nhop4 = "10.0.0.2";
  auto nhop6 = "2401:db00:2110:3001::0002";

  // resolve the nexthops
  auto nh1 = makeResolvedNextHops({{kIntf1, nhop4}});
  auto nh2 = makeResolvedNextHops({{kIntf1, nhop6}});

  // prefixes to add
  auto prefixA4 = "7.1.0.0/16";
  auto prefixA6 = "aaaa:1::0/64";
  auto prefixB4 = "7.2.0.0/16";
  auto prefixB6 = "aaaa:2::0/64";
  auto prefixC4 = "7.3.0.0/16";
  auto prefixC6 = "aaaa:3::0/64";
  auto prefixD4 = "7.4.0.0/16";
  auto prefixD6 = "aaaa:4::0/64";

  auto addRoutesForClient = [&](const auto& prefix4,
                                const auto& prefix6,
                                const auto& client,
                                const auto& clientAdmin) {
    handler.addUnicastRoute(
        client, makeUnicastRoute(prefix4, nhop4, clientAdmin));
    handler.addUnicastRoute(
        client, makeUnicastRoute(prefix6, nhop6, clientAdmin));
  };

  addRoutesForClient(prefixA4, prefixA6, bgpClient, bgpClientAdmin);
  addRoutesForClient(prefixB4, prefixB6, openrClient, openrClientAdmin);

  auto verifyPrefixesPresent = [&](const auto& prefix4, const auto& prefix6) {
    auto state = this->sw_->getState();
    auto rtA4 = findRoute<folly::IPAddressV4>(
        rid, IPAddress::createNetwork(prefix4), state);
    EXPECT_NE(nullptr, rtA4);
    EXPECT_EQ(
        rtA4->getForwardInfo(),
        RouteNextHopEntry(
            makeResolvedNextHops({{kIntf1, nhop4}}),
            AdminDistance::MAX_ADMIN_DISTANCE));

    auto rtA6 = findRoute<folly::IPAddressV6>(
        rid, IPAddress::createNetwork(prefix6), state);
    EXPECT_NE(nullptr, rtA6);
    EXPECT_EQ(
        rtA6->getForwardInfo(),
        RouteNextHopEntry(
            makeResolvedNextHops({{kIntf1, nhop6}}),
            AdminDistance::MAX_ADMIN_DISTANCE));
  };
  verifyPrefixesPresent(prefixA4, prefixA6);
  verifyPrefixesPresent(prefixB4, prefixB6);

  auto verifyPrefixesRemoved = [&](const auto& prefix4, const auto& prefix6) {
    auto state = this->sw_->getState();
    auto rtA4 = findRoute<folly::IPAddressV4>(
        rid, IPAddress::createNetwork(prefix4), state);
    EXPECT_EQ(nullptr, rtA4);
    auto rtA6 = findRoute<folly::IPAddressV6>(
        rid, IPAddress::createNetwork(prefix6), state);
    EXPECT_EQ(nullptr, rtA6);
  };

  // Call syncFib for BGP. Remove all BGP routes and add some new routes
  auto newBgpRoutes = std::make_unique<std::vector<UnicastRoute>>();
  newBgpRoutes->push_back(
      *makeUnicastRoute(prefixC6, nhop6, bgpClientAdmin).get());
  newBgpRoutes->push_back(
      *makeUnicastRoute(prefixC4, nhop4, bgpClientAdmin).get());
  handler.syncFib(bgpClient, std::move(newBgpRoutes));

  // verify that old BGP prefixes are removed
  verifyPrefixesRemoved(prefixA4, prefixA6);
  // verify that OPENR prefixes exist
  verifyPrefixesPresent(prefixB4, prefixB6);
  // verify new BGP prefixes are added
  verifyPrefixesPresent(prefixC4, prefixC6);

  // Call syncFib for OPENR. Remove all OPENR routes and add some new routes
  auto newOpenrRoutes = std::make_unique<std::vector<UnicastRoute>>();
  newOpenrRoutes->push_back(
      *makeUnicastRoute(prefixD4, nhop4, openrClientAdmin).get());
  newOpenrRoutes->push_back(
      *makeUnicastRoute(prefixD6, nhop6, openrClientAdmin).get());
  handler.syncFib(openrClient, std::move(newOpenrRoutes));

  // verify that old OPENR prefixes are removed
  verifyPrefixesRemoved(prefixB4, prefixB6);
  // verify that new OPENR prefixes are added
  verifyPrefixesPresent(prefixD4, prefixD6);

  // Add back BGP and OPENR routes
  addRoutesForClient(prefixA4, prefixA6, bgpClient, bgpClientAdmin);
  addRoutesForClient(prefixB4, prefixB6, openrClient, openrClientAdmin);

  // verify routes added
  verifyPrefixesPresent(prefixA4, prefixA6);
  verifyPrefixesPresent(prefixB4, prefixB6);
}

// Test for the ThriftHandler::syncFib method
TEST_F(ThriftTest, syncFib) {
  RouterID rid = RouterID(0);

  // Create a mock SwSwitch using the config, and wrap it in a ThriftHandler
  ThriftHandler handler(this->sw_);

  auto randomClient = 500;
  auto bgpClient = static_cast<int16_t>(ClientID::BGPD);
  auto staticClient = static_cast<int16_t>(ClientID::STATIC_ROUTE);
  auto randomClientAdmin = this->sw_->clientIdToAdminDistance(randomClient);
  auto bgpAdmin = this->sw_->clientIdToAdminDistance(bgpClient);
  auto staticAdmin = this->sw_->clientIdToAdminDistance(staticClient);
  // STATIC_ROUTE > BGPD > RANDOM_CLIENT
  //
  // Add a few BGP routes
  //

  auto cli1_nhop4 = "10.0.0.11";
  auto cli1_nhop6 = "2401:db00:2110:3001::0011";
  auto cli2_nhop4 = "10.0.0.22";
  auto cli2_nhop6 = "2401:db00:2110:3001::0022";
  auto cli3_nhop6 = "2401:db00:2110:3001::0033";

  // These routes will include nexthops from client 10 only
  auto prefixA4 = "7.1.0.0/16";
  auto prefixA6 = "aaaa:1::0/64";
  handler.addUnicastRoute(
      randomClient, makeUnicastRoute(prefixA4, cli1_nhop4, randomClientAdmin));
  handler.addUnicastRoute(
      randomClient, makeUnicastRoute(prefixA6, cli1_nhop6, randomClientAdmin));

  // This route will include nexthops from clients randomClient and bgpClient
  auto prefixB4 = "7.2.0.0/16";
  handler.addUnicastRoute(
      randomClient, makeUnicastRoute(prefixB4, cli1_nhop4, randomClientAdmin));
  handler.addUnicastRoute(
      bgpClient, makeUnicastRoute(prefixB4, cli2_nhop4, bgpAdmin));

  // This route will include nexthops from clients randomClient and bgpClient
  // and staticClient
  auto prefixC6 = "aaaa:3::0/64";
  handler.addUnicastRoute(
      randomClient, makeUnicastRoute(prefixC6, cli1_nhop6, randomClientAdmin));
  handler.addUnicastRoute(
      bgpClient, makeUnicastRoute(prefixC6, cli2_nhop6, bgpAdmin));
  handler.addUnicastRoute(
      staticClient, makeUnicastRoute(prefixC6, cli3_nhop6, staticAdmin));

  // These routes will not be used until fibSync happens.
  auto prefixD4 = "7.4.0.0/16";
  auto prefixD6 = "aaaa:4::0/64";

  //
  // Test the state of things before calling syncFib
  //

  // Make sure all the static and link-local routes are there
  auto ensureConfigRoutes = [this, rid]() {
    auto state = this->sw_->getState();
    EXPECT_NE(
        nullptr,
        findRoute<folly::IPAddressV4>(
            rid, IPAddress::createNetwork("10.0.0.0/24"), state));
    EXPECT_NE(
        nullptr,
        findRoute<folly::IPAddressV4>(
            rid, IPAddress::createNetwork("192.168.0.0/24"), state));
    EXPECT_NE(
        nullptr,
        findRoute<folly::IPAddressV6>(
            rid, IPAddress::createNetwork("2401:db00:2110:3001::/64"), state));
    EXPECT_NE(
        nullptr,
        findRoute<folly::IPAddressV6>(
            rid, IPAddress::createNetwork("fe80::/64"), state));
  };
  auto kIntf1 = InterfaceID(1);
  ensureConfigRoutes();
  // Make sure the lowest adming distance route is installed in FIB routes are
  // there.

  {
    auto state = this->sw_->getState();
    // Only random client routes
    auto rtA4 = findRoute<folly::IPAddressV4>(
        rid, IPAddress::createNetwork(prefixA4), state);
    EXPECT_NE(nullptr, rtA4);
    EXPECT_EQ(
        rtA4->getForwardInfo(),
        RouteNextHopEntry(
            makeResolvedNextHops({{kIntf1, cli1_nhop4}}),
            AdminDistance::MAX_ADMIN_DISTANCE));

    auto rtA6 = findRoute<folly::IPAddressV6>(
        rid, IPAddress::createNetwork(prefixA6), state);
    EXPECT_NE(nullptr, rtA6);
    EXPECT_EQ(
        rtA6->getForwardInfo(),
        RouteNextHopEntry(
            makeResolvedNextHops({{kIntf1, cli1_nhop6}}),
            AdminDistance::MAX_ADMIN_DISTANCE));
    // Random client and BGP routes - bgp should win
    auto rtB4 = findRoute<folly::IPAddressV4>(
        rid, IPAddress::createNetwork(prefixB4), state);
    EXPECT_NE(nullptr, rtB4);
    EXPECT_EQ(
        rtB4->getForwardInfo(),
        RouteNextHopEntry(
            makeResolvedNextHops({{kIntf1, cli2_nhop4}}),
            AdminDistance::MAX_ADMIN_DISTANCE));
    // Random client, bgp, static routes. Static shouold win
    auto rtC6 = findRoute<folly::IPAddressV6>(
        rid, IPAddress::createNetwork(prefixC6), state);
    EXPECT_NE(nullptr, rtC6);
    EXPECT_EQ(
        rtC6->getForwardInfo(),
        RouteNextHopEntry(
            makeResolvedNextHops({{kIntf1, cli3_nhop6}}),
            AdminDistance::MAX_ADMIN_DISTANCE));
    auto [v4Routes, v6Routes] = getRouteCount(state);
    EXPECT_EQ(
        7, v4Routes); // 4 intf routes + 2 routes from above + 1 default routes
    EXPECT_EQ(
        6, v6Routes); // 2 intf routes + 2 routes from above + 1 link local
                      // + 1 default route
    // Unistalled routes. These should not be found
    EXPECT_EQ(
        nullptr,
        findRoute<folly::IPAddressV4>(
            rid, IPAddress::createNetwork(prefixD4), state));
    EXPECT_EQ(
        nullptr,
        findRoute<folly::IPAddressV6>(
            rid, IPAddress::createNetwork(prefixD6), state));
  }
  //
  // Now use syncFib to remove all the routes for randomClient and add
  // some new ones
  // Statics, link-locals, and clients bgp and static should remain unchanged.
  //

  auto newRoutes = std::make_unique<std::vector<UnicastRoute>>();
  UnicastRoute nr1 =
      *makeUnicastRoute(prefixC6, cli1_nhop6, randomClientAdmin).get();
  UnicastRoute nr2 =
      *makeUnicastRoute(prefixD6, cli1_nhop6, randomClientAdmin).get();
  UnicastRoute nr3 =
      *makeUnicastRoute(prefixD4, cli1_nhop4, randomClientAdmin).get();
  newRoutes->push_back(nr1);
  newRoutes->push_back(nr2);
  newRoutes->push_back(nr3);
  handler.syncFib(randomClient, std::move(newRoutes));

  //
  // Test the state of things after syncFib
  //
  {
    // Make sure all the static and link-local routes are still there
    auto state = this->sw_->getState();
    ensureConfigRoutes();
    // Only random client routes from before are gone, since we did not syncFib
    // with them
    auto rtA4 = findRoute<folly::IPAddressV4>(
        rid, IPAddress::createNetwork(prefixA4), state);
    EXPECT_EQ(nullptr, rtA4);

    auto rtA6 = findRoute<folly::IPAddressV6>(
        rid, IPAddress::createNetwork(prefixA6), state);
    EXPECT_EQ(nullptr, rtA6);
    // random client and bgp routes. Bgp should continue to win
    auto rtB4 = findRoute<folly::IPAddressV4>(
        rid, IPAddress::createNetwork(prefixB4), state);
    EXPECT_TRUE(rtB4->getForwardInfo().isSame(RouteNextHopEntry(
        makeResolvedNextHops({{InterfaceID(1), cli2_nhop4}}),
        AdminDistance::MAX_ADMIN_DISTANCE)));

    // Random client, bgp, static routes. Static should win
    auto rtC6 = findRoute<folly::IPAddressV6>(
        rid, IPAddress::createNetwork(prefixC6), state);
    EXPECT_NE(nullptr, rtC6);
    EXPECT_EQ(
        rtC6->getForwardInfo(),
        RouteNextHopEntry(
            makeResolvedNextHops({{kIntf1, cli3_nhop6}}),
            AdminDistance::MAX_ADMIN_DISTANCE));
    // D6 and D4 should now be found and resolved by random client nhops
    auto rtD4 = findRoute<folly::IPAddressV4>(
        rid, IPAddress::createNetwork(prefixD4), state);
    EXPECT_NE(nullptr, rtD4);
    EXPECT_EQ(
        rtD4->getForwardInfo(),
        RouteNextHopEntry(
            makeResolvedNextHops({{kIntf1, cli1_nhop4}}),
            AdminDistance::MAX_ADMIN_DISTANCE));
    auto rtD6 = findRoute<folly::IPAddressV6>(
        rid, IPAddress::createNetwork(prefixD6), state);
    EXPECT_NE(nullptr, rtD6);
    EXPECT_EQ(
        rtD6->getForwardInfo(),
        RouteNextHopEntry(
            makeResolvedNextHops({{kIntf1, cli1_nhop6}}),
            AdminDistance::MAX_ADMIN_DISTANCE));
    // A4, A6 removed, D4, D6 added. Count should remain same
    auto [v4Routes, v6Routes] = getRouteCount(state);
    EXPECT_EQ(7, v4Routes);
    EXPECT_EQ(6, v6Routes);
  }
}

// Test for the ThriftHandler::add/del Unicast routes methods
// This test is a replica of syncFib test from above, except that
// when adding, deleting routes for a client it uses add, del
// UnicastRoute APIs instead of syncFib
TEST_F(ThriftTest, addDelUnicastRoutes) {
  RouterID rid = RouterID(0);

  // Create a mock SwSwitch using the config, and wrap it in a ThriftHandler
  ThriftHandler handler(this->sw_);

  auto randomClient = 500;
  auto bgpClient = static_cast<int16_t>(ClientID::BGPD);
  auto staticClient = static_cast<int16_t>(ClientID::STATIC_ROUTE);
  auto randomClientAdmin = this->sw_->clientIdToAdminDistance(randomClient);
  auto bgpAdmin = this->sw_->clientIdToAdminDistance(bgpClient);
  auto staticAdmin = this->sw_->clientIdToAdminDistance(staticClient);
  // STATIC_ROUTE > BGPD > RANDOM_CLIENT
  //
  // Add a few BGP routes
  //

  auto cli1_nhop4 = "10.0.0.11";
  auto cli1_nhop6 = "2401:db00:2110:3001::0011";
  auto cli2_nhop4 = "10.0.0.22";
  auto cli2_nhop6 = "2401:db00:2110:3001::0022";
  auto cli3_nhop6 = "2401:db00:2110:3001::0033";

  // These routes will include nexthops from client 10 only
  auto prefixA4 = "7.1.0.0/16";
  auto prefixA6 = "aaaa:1::0/64";
  handler.addUnicastRoute(
      randomClient, makeUnicastRoute(prefixA4, cli1_nhop4, randomClientAdmin));
  handler.addUnicastRoute(
      randomClient, makeUnicastRoute(prefixA6, cli1_nhop6, randomClientAdmin));

  // This route will include nexthops from clients randomClient and bgpClient
  auto prefixB4 = "7.2.0.0/16";
  handler.addUnicastRoute(
      randomClient, makeUnicastRoute(prefixB4, cli1_nhop4, randomClientAdmin));
  handler.addUnicastRoute(
      bgpClient, makeUnicastRoute(prefixB4, cli2_nhop4, bgpAdmin));

  // This route will include nexthops from clients randomClient and bgpClient
  // and staticClient
  auto prefixC6 = "aaaa:3::0/64";
  handler.addUnicastRoute(
      randomClient, makeUnicastRoute(prefixC6, cli1_nhop6, randomClientAdmin));
  handler.addUnicastRoute(
      bgpClient, makeUnicastRoute(prefixC6, cli2_nhop6, bgpAdmin));
  handler.addUnicastRoute(
      staticClient, makeUnicastRoute(prefixC6, cli3_nhop6, staticAdmin));

  // These routes will not be used until fibSync happens.
  auto prefixD4 = "7.4.0.0/16";
  auto prefixD6 = "aaaa:4::0/64";

  //
  // Test the state of things before calling syncFib
  //

  // Make sure all the static and link-local routes are there
  auto ensureConfigRoutes = [this, rid]() {
    auto state = this->sw_->getState();
    EXPECT_NE(
        nullptr,
        findRoute<folly::IPAddressV4>(

            rid, IPAddress::createNetwork("10.0.0.0/24"), state));
    EXPECT_NE(
        nullptr,
        findRoute<folly::IPAddressV4>(

            rid, IPAddress::createNetwork("192.168.0.0/24"), state));
    EXPECT_NE(
        nullptr,
        findRoute<folly::IPAddressV6>(

            rid, IPAddress::createNetwork("2401:db00:2110:3001::/64"), state));
    EXPECT_NE(
        nullptr,
        findRoute<folly::IPAddressV6>(

            rid, IPAddress::createNetwork("fe80::/64"), state));
  };
  auto kIntf1 = InterfaceID(1);
  ensureConfigRoutes();
  // Make sure the lowest admin distance route is installed in FIB routes are
  // there.

  {
    auto state = this->sw_->getState();
    // Only random client routes
    auto rtA4 = findRoute<folly::IPAddressV4>(
        rid, IPAddress::createNetwork(prefixA4), state);
    EXPECT_NE(nullptr, rtA4);
    EXPECT_EQ(
        rtA4->getForwardInfo(),
        RouteNextHopEntry(
            makeResolvedNextHops({{kIntf1, cli1_nhop4}}),
            AdminDistance::MAX_ADMIN_DISTANCE));

    auto rtA6 = findRoute<folly::IPAddressV6>(
        rid, IPAddress::createNetwork(prefixA6), state);
    EXPECT_NE(nullptr, rtA6);
    EXPECT_EQ(
        rtA6->getForwardInfo(),
        RouteNextHopEntry(
            makeResolvedNextHops({{kIntf1, cli1_nhop6}}),
            AdminDistance::MAX_ADMIN_DISTANCE));
    // Random client and BGP routes - bgp should win
    auto rtB4 = findRoute<folly::IPAddressV4>(
        rid, IPAddress::createNetwork(prefixB4), state);
    EXPECT_NE(nullptr, rtB4);
    EXPECT_EQ(
        rtB4->getForwardInfo(),
        RouteNextHopEntry(
            makeResolvedNextHops({{kIntf1, cli2_nhop4}}),
            AdminDistance::MAX_ADMIN_DISTANCE));
    // Random client, bgp, static routes. Static should win
    auto rtC6 = findRoute<folly::IPAddressV6>(
        rid, IPAddress::createNetwork(prefixC6), state);
    EXPECT_NE(nullptr, rtC6);
    EXPECT_EQ(
        rtC6->getForwardInfo(),
        RouteNextHopEntry(
            makeResolvedNextHops({{kIntf1, cli3_nhop6}}),
            AdminDistance::MAX_ADMIN_DISTANCE));
    auto [v4Routes, v6Routes] = getRouteCount(state);
    EXPECT_EQ(
        7, v4Routes); // 4 intf routes + 2 routes from above + 1 default routes
    EXPECT_EQ(
        6, v6Routes); // 2 intf routes + 2 routes from above + 1 link local
                      // + 1 default route
    // Unistalled routes. These should not be found
    EXPECT_EQ(
        nullptr,
        findRoute<folly::IPAddressV4>(
            rid, IPAddress::createNetwork(prefixD4), state));
    EXPECT_EQ(
        nullptr,
        findRoute<folly::IPAddressV6>(
            rid, IPAddress::createNetwork(prefixD6), state));
  }
  //
  // Now use deleteUnicastRoute to remove all the routes for randomClient and
  // add some new ones Statics, link-locals, and clients bgp and static should
  // remain unchanged.
  //
  std::vector<IpPrefix> delRoutes = {
      ipPrefix(IPAddress::createNetwork(prefixA4)),
      ipPrefix(IPAddress::createNetwork(prefixA6)),
      ipPrefix(IPAddress::createNetwork(prefixB4)),
  };
  handler.deleteUnicastRoutes(
      randomClient, std::make_unique<std::vector<IpPrefix>>(delRoutes));
  auto newRoutes = std::make_unique<std::vector<UnicastRoute>>();
  UnicastRoute nr1 =
      *makeUnicastRoute(prefixC6, cli1_nhop6, randomClientAdmin).get();
  UnicastRoute nr2 =
      *makeUnicastRoute(prefixD6, cli1_nhop6, randomClientAdmin).get();
  UnicastRoute nr3 =
      *makeUnicastRoute(prefixD4, cli1_nhop4, randomClientAdmin).get();
  newRoutes->push_back(nr1);
  newRoutes->push_back(nr2);
  newRoutes->push_back(nr3);
  handler.addUnicastRoutes(randomClient, std::move(newRoutes));

  //
  // Test the state of things after syncFib
  //
  {
    // Make sure all the static and link-local routes are still there
    auto state = this->sw_->getState();
    ensureConfigRoutes();
    // Only random client routes from before are gone, since we did not syncFib
    // with them
    auto rtA4 = findRoute<folly::IPAddressV4>(
        rid, IPAddress::createNetwork(prefixA4), state);
    EXPECT_EQ(nullptr, rtA4);

    auto rtA6 = findRoute<folly::IPAddressV6>(
        rid, IPAddress::createNetwork(prefixA6), state);
    EXPECT_EQ(nullptr, rtA6);
    // random client and bgp routes. Bgp should continue to win
    auto rtB4 = findRoute<folly::IPAddressV4>(
        rid, IPAddress::createNetwork(prefixB4), state);
    EXPECT_TRUE(rtB4->getForwardInfo().isSame(RouteNextHopEntry(
        makeResolvedNextHops({{InterfaceID(1), cli2_nhop4}}),
        AdminDistance::MAX_ADMIN_DISTANCE)));

    // Random client, bgp, static routes. Static should win
    auto rtC6 = findRoute<folly::IPAddressV6>(
        rid, IPAddress::createNetwork(prefixC6), state);
    EXPECT_NE(nullptr, rtC6);
    EXPECT_EQ(
        rtC6->getForwardInfo(),
        RouteNextHopEntry(
            makeResolvedNextHops({{kIntf1, cli3_nhop6}}),
            AdminDistance::MAX_ADMIN_DISTANCE));
    // D6 and D4 should now be found and resolved by random client nhops
    auto rtD4 = findRoute<folly::IPAddressV4>(
        rid, IPAddress::createNetwork(prefixD4), state);
    EXPECT_NE(nullptr, rtD4);
    EXPECT_EQ(
        rtD4->getForwardInfo(),
        RouteNextHopEntry(
            makeResolvedNextHops({{kIntf1, cli1_nhop4}}),
            AdminDistance::MAX_ADMIN_DISTANCE));
    auto rtD6 = findRoute<folly::IPAddressV6>(
        rid, IPAddress::createNetwork(prefixD6), state);
    EXPECT_NE(nullptr, rtD6);
    EXPECT_EQ(
        rtD6->getForwardInfo(),
        RouteNextHopEntry(
            makeResolvedNextHops({{kIntf1, cli1_nhop6}}),
            AdminDistance::MAX_ADMIN_DISTANCE));
    // A4, A6 removed, D4, D6 added. Count should remain same
    auto [v4Routes, v6Routes] = getRouteCount(state);
    EXPECT_EQ(7, v4Routes);
    EXPECT_EQ(6, v6Routes);
  }
}

TEST_F(ThriftTest, delUnicastRoutes) {
  RouterID rid = RouterID(0);

  // Create a mock SwSwitch using the config, and wrap it in a ThriftHandler
  ThriftHandler handler(this->sw_);

  auto randomClient = 500;
  auto bgpClient = static_cast<int16_t>(ClientID::BGPD);
  auto staticClient = static_cast<int16_t>(ClientID::STATIC_ROUTE);
  auto randomClientAdmin = this->sw_->clientIdToAdminDistance(randomClient);
  auto bgpAdmin = this->sw_->clientIdToAdminDistance(bgpClient);
  auto staticAdmin = this->sw_->clientIdToAdminDistance(staticClient);
  // STATIC_ROUTE > BGPD > RANDOM_CLIENT
  //
  // Add a few BGP routes
  //

  auto cli1_nhop4 = "10.0.0.11";
  auto cli1_nhop6 = "2401:db00:2110:3001::0011";
  auto cli2_nhop4 = "10.0.0.22";
  auto cli2_nhop6 = "2401:db00:2110:3001::0022";
  auto cli3_nhop6 = "2401:db00:2110:3001::0033";

  // These routes will include nexthops from client 10 only
  auto prefixA4 = "7.1.0.0/16";
  auto prefixA6 = "aaaa:1::0/64";
  handler.addUnicastRoute(
      randomClient, makeUnicastRoute(prefixA4, cli1_nhop4, randomClientAdmin));
  handler.addUnicastRoute(
      randomClient, makeUnicastRoute(prefixA6, cli1_nhop6, randomClientAdmin));

  // This route will include nexthops from clients randomClient and bgpClient
  auto prefixB4 = "7.2.0.0/16";
  handler.addUnicastRoute(
      randomClient, makeUnicastRoute(prefixB4, cli1_nhop4, randomClientAdmin));
  handler.addUnicastRoute(
      bgpClient, makeUnicastRoute(prefixB4, cli2_nhop4, bgpAdmin));

  // This route will include nexthops from clients randomClient and bgpClient
  // and staticClient
  auto prefixC6 = "aaaa:3::0/64";
  handler.addUnicastRoute(
      randomClient, makeUnicastRoute(prefixC6, cli1_nhop6, randomClientAdmin));
  handler.addUnicastRoute(
      bgpClient, makeUnicastRoute(prefixC6, cli2_nhop6, bgpAdmin));
  handler.addUnicastRoute(
      staticClient, makeUnicastRoute(prefixC6, cli3_nhop6, staticAdmin));

  auto assertRoute = [rid, prefixC6, this](
                         bool expectPresent, const std::string& nhop) {
    auto kIntf1 = InterfaceID(1);
    auto rtC6 = findRoute<folly::IPAddressV6>(
        rid, IPAddress::createNetwork(prefixC6), this->sw_->getState());
    if (!expectPresent) {
      EXPECT_EQ(nullptr, rtC6);
      return;
    }
    ASSERT_NE(nullptr, rtC6);
    EXPECT_EQ(
        rtC6->getForwardInfo(),
        RouteNextHopEntry(
            makeResolvedNextHops({{kIntf1, nhop}}),
            AdminDistance::MAX_ADMIN_DISTANCE));
  };
  // Random client, bgp, static routes. Static should win
  assertRoute(true, cli3_nhop6);
  std::vector<IpPrefix> delRoutes = {
      ipPrefix(IPAddress::createNetwork(prefixC6)),
  };
  // Now delete prefixC6 for Static client. BGP should win
  handler.deleteUnicastRoutes(
      staticClient, std::make_unique<std::vector<IpPrefix>>(delRoutes));
  // Random client, bgp, routes. BGP should win
  assertRoute(true, cli2_nhop6);
  // Now delete prefixC6 for BGP client. random client should win
  // For good measure - use the single route delete API here
  handler.deleteUnicastRoute(
      bgpClient,
      std::make_unique<IpPrefix>(ipPrefix(IPAddress::createNetwork(prefixC6))));
  // Random client routes only
  assertRoute(true, cli1_nhop6);
  // Now delete prefixC6 for random client. Route should be dropped now
  handler.deleteUnicastRoutes(
      randomClient, std::make_unique<std::vector<IpPrefix>>(delRoutes));
  // Random client, bgp, routes. BGP should win
  assertRoute(false, "none");
  // Add routes back and see that lowest admin distance route comes in
  // again
  handler.addUnicastRoute(
      randomClient, makeUnicastRoute(prefixC6, cli1_nhop6, randomClientAdmin));
  // Random client routes only
  assertRoute(true, cli1_nhop6);
  handler.addUnicastRoute(
      bgpClient, makeUnicastRoute(prefixC6, cli2_nhop6, bgpAdmin));
  // Random client, bgp, routes. BGP should win
  assertRoute(true, cli2_nhop6);
  handler.addUnicastRoute(
      staticClient, makeUnicastRoute(prefixC6, cli3_nhop6, staticAdmin));
  // Random client, bgp, static routes. Static should win
  assertRoute(true, cli3_nhop6);
}

TEST_F(ThriftTest, syncFibIsHwProtected) {
  // Create a mock SwSwitch using the config, and wrap it in a ThriftHandler
  ThriftHandler handler(this->sw_);
  auto addRoutes = std::make_unique<std::vector<UnicastRoute>>();
  UnicastRoute nr1 =
      *makeUnicastRoute("aaaa::/64", "2401:db00:2110:3001::1").get();
  addRoutes->push_back(nr1);
  EXPECT_HW_CALL(this->sw_, stateChanged(_));
  handler.addUnicastRoutes(10, std::move(addRoutes));
  auto newRoutes = std::make_unique<std::vector<UnicastRoute>>();
  UnicastRoute nr2 = *makeUnicastRoute("bbbb::/64", "42::42").get();
  newRoutes->push_back(nr2);
  // Fail HW update by returning current state
  EXPECT_HW_CALL(this->sw_, stateChanged(_))
      .Times(::testing::AtLeast(1))
      .WillOnce(Return(this->sw_->getState()));
  EXPECT_THROW(
      {
        try {
          handler.syncFib(10, std::move(newRoutes));
        } catch (const FbossFibUpdateError& fibError) {
          EXPECT_EQ(fibError.vrf2failedAddUpdatePrefixes_ref()->size(), 1);
          auto itr = fibError.vrf2failedAddUpdatePrefixes_ref()->find(0);
          EXPECT_EQ(itr->second.size(), 1);
          itr = fibError.vrf2failedDeletePrefixes_ref()->find(0);
          EXPECT_EQ(itr->second.size(), 1);
          throw;
        }
      },
      FbossFibUpdateError);
}

TEST_F(ThriftTest, addUnicastRoutesIsHwProtected) {
  ThriftHandler handler(this->sw_);
  auto newRoutes = std::make_unique<std::vector<UnicastRoute>>();
  UnicastRoute nr1 = *makeUnicastRoute("aaaa::/64", "42::42").get();
  newRoutes->push_back(nr1);
  // Fail HW update by returning current state
  EXPECT_HW_CALL(this->sw_, stateChanged(_))
      .WillOnce(Return(this->sw_->getState()));
  EXPECT_THROW(
      {
        try {
          handler.addUnicastRoutes(10, std::move(newRoutes));

        } catch (const FbossFibUpdateError& fibError) {
          EXPECT_EQ(fibError.vrf2failedAddUpdatePrefixes_ref()->size(), 1);
          auto itr = fibError.vrf2failedAddUpdatePrefixes_ref()->find(0);
          EXPECT_EQ(itr->second.size(), 1);
          throw;
        }
      },
      FbossFibUpdateError);
}

TEST_F(ThriftTest, getRouteTable) {
  ThriftHandler handler(this->sw_);
  auto [v4Routes, v6Routes] = getRouteCount(this->sw_->getState());
  std::vector<UnicastRoute> routeTable;
  handler.getRouteTable(routeTable);
  // 6 intf routes + 2 default routes + 1 link local route
  EXPECT_EQ(9, v4Routes + v6Routes);
  EXPECT_EQ(9, routeTable.size());
}

TEST_F(ThriftTest, getRouteDetails) {
  ThriftHandler handler(this->sw_);
  auto [v4Routes, v6Routes] = getRouteCount(this->sw_->getState());
  std::vector<RouteDetails> routeDetails;
  handler.getRouteTableDetails(routeDetails);
  // 6 intf routes + 2 default routes + 1 link local route
  EXPECT_EQ(9, v4Routes + v6Routes);
  EXPECT_EQ(9, routeDetails.size());
}

TEST_F(ThriftTest, getRouteTableByClient) {
  ThriftHandler handler(this->sw_);
  std::vector<UnicastRoute> routeTable;
  handler.getRouteTableByClient(
      routeTable, static_cast<int16_t>(ClientID::INTERFACE_ROUTE));
  // 6 intf routes + 2 default routes + 1 link local route
  EXPECT_EQ(6, routeTable.size());
}
std::unique_ptr<MplsRoute> makeMplsRoute(
    int32_t mplsLabel,
    std::string nxtHop,
    AdminDistance distance = AdminDistance::MAX_ADMIN_DISTANCE) {
  auto nr = std::make_unique<MplsRoute>();
  nr->topLabel_ref() = mplsLabel;
  NextHopThrift nh;
  MplsAction mplsAction;
  mplsAction.action_ref() = MplsActionCode::POP_AND_LOOKUP;
  nh.address_ref() = toBinaryAddress(IPAddress(nxtHop));
  nh.mplsAction_ref() = mplsAction;
  nr->nextHops_ref()->push_back(nh);
  nr->adminDistance_ref() = distance;
  return nr;
}

TEST_F(ThriftTest, syncMplsFibIsHwProtected) {
  // Create a mock SwSwitch using the config, and wrap it in a ThriftHandler
  ThriftHandler handler(this->sw_);
  auto newRoutes = std::make_unique<std::vector<MplsRoute>>();
  MplsRoute nr1 = *makeMplsRoute(101, "10.0.0.2").get();
  newRoutes->push_back(nr1);
  // Fail HW update by returning current state
  EXPECT_HW_CALL(this->sw_, stateChanged(_))
      .WillRepeatedly(Return(this->sw_->getState()));
  EXPECT_THROW(
      {
        try {
          handler.syncMplsFib(10, std::move(newRoutes));
        } catch (const FbossFibUpdateError& fibError) {
          EXPECT_EQ(fibError.failedAddUpdateMplsLabels_ref()->size(), 1);
          EXPECT_EQ(*fibError.failedAddUpdateMplsLabels_ref()->begin(), 101);
          throw;
        }
      },
      FbossFibUpdateError);
}

TEST_F(ThriftTest, addMplsRoutesIsHwProtected) {
  // Create a mock SwSwitch using the config, and wrap it in a ThriftHandler
  ThriftHandler handler(this->sw_);
  auto newRoutes = std::make_unique<std::vector<MplsRoute>>();
  MplsRoute nr1 = *makeMplsRoute(101, "10.0.0.2").get();
  newRoutes->push_back(nr1);
  // Fail HW update by returning current state
  EXPECT_HW_CALL(this->sw_, stateChanged(_))
      .WillRepeatedly(Return(this->sw_->getState()));
  EXPECT_THROW(
      {
        try {
          handler.addMplsRoutes(10, std::move(newRoutes));
        } catch (const FbossFibUpdateError& fibError) {
          EXPECT_EQ(fibError.failedAddUpdateMplsLabels_ref()->size(), 1);
          EXPECT_EQ(*fibError.failedAddUpdateMplsLabels_ref()->begin(), 101);
          throw;
        }
      },
      FbossFibUpdateError);
}

TEST_F(ThriftTest, hwUpdateErrorAfterPartialUpdate) {
  ThriftHandler handler(this->sw_);
  std::vector<UnicastRoute>();
  UnicastRoute nr1 =
      *makeUnicastRoute("aaaa::/64", "2401:db00:2110:3001::1").get();
  std::vector<UnicastRoute> routes;
  routes.push_back(nr1);
  EXPECT_HW_CALL(this->sw_, stateChanged(_)).Times(2);
  handler.addUnicastRoutes(
      10, std::make_unique<std::vector<UnicastRoute>>(routes));
  auto oneRouteAddedState = this->sw_->getState();
  std::vector<IpPrefix> delRoutes = {
      ipPrefix(IPAddress::createNetwork("aaaa::/64")),
  };
  // Delete added route so we revert back to starting state
  handler.deleteUnicastRoutes(
      10, std::make_unique<std::vector<IpPrefix>>(delRoutes));
  // Now try to add 2 routes, have the HwSwitch fail after adding one route
  UnicastRoute nr2 =
      *makeUnicastRoute("bbbb::/64", "2401:db00:2110:3001::1").get();
  routes.push_back(nr2);
  // Fail HW update by returning one route added state.
  EXPECT_HW_CALL(this->sw_, stateChanged(_))
      .Times(::testing::AtLeast(1))
      .WillOnce(Return(oneRouteAddedState));
  EXPECT_THROW(
      {
        try {
          handler.addUnicastRoutes(
              10, std::make_unique<std::vector<UnicastRoute>>(routes));
        } catch (const FbossFibUpdateError& fibError) {
          EXPECT_EQ(fibError.vrf2failedAddUpdatePrefixes_ref()->size(), 1);
          auto itr = fibError.vrf2failedAddUpdatePrefixes_ref()->find(0);
          EXPECT_EQ(itr->second.size(), 1);
          itr = fibError.vrf2failedDeletePrefixes_ref()->find(0);
          EXPECT_EQ(itr->second.size(), 0);
          throw;
        }
      },
      FbossFibUpdateError);
}

TEST_F(ThriftTest, routeUpdatesWithConcurrentReads) {
  auto thriftHgridRoutes =
      utility::HgridDuRouteScaleGenerator(this->sw_->getState(), 100000)
          .getThriftRoutes()[0];
  auto thriftRswRoutes =
      utility::RSWRouteScaleGenerator(this->sw_->getState(), 10000)
          .getThriftRoutes()[0];
  std::atomic<bool> done{false};

  ThriftHandler handler(this->sw_);
  std::thread routeReads([&handler, &done]() {
    while (!done) {
      std::vector<RouteDetails> details;
      handler.getRouteTableDetails(details);
      UnicastRoute route;
      handler.getIpRoute(
          route,
          std::make_unique<facebook::network::thrift::Address>(
              facebook::network::toAddress(folly::IPAddress("2001::"))),
          RouterID(0));
    }
  });
  handler.addUnicastRoutes(
      10, std::make_unique<std::vector<UnicastRoute>>(thriftHgridRoutes));
  handler.addUnicastRoutes(
      11, std::make_unique<std::vector<UnicastRoute>>(thriftRswRoutes));
  done = true;
  routeReads.join();
}

TEST_F(ThriftTest, UnicastRoutesWithCounterID) {
  RouterID rid = RouterID(0);

  // Create a mock SwSwitch using the config, and wrap it in a ThriftHandler
  ThriftHandler handler(this->sw_);

  auto randomClient = 500;
  auto randomClientAdmin = this->sw_->clientIdToAdminDistance(randomClient);
  auto bgpClient = static_cast<int16_t>(ClientID::BGPD);
  auto bgpClientAdmin = this->sw_->clientIdToAdminDistance(bgpClient);

  auto cli1_nhop4 = "10.0.0.11";
  auto cli1_nhop6 = "2401:db00:2110:3001::0011";

  // These routes will include nexthops from client 10 only
  auto prefixA4 = "7.1.0.0/16";
  auto prefixA6 = "aaaa:1::0/64";

  std::optional<RouteCounterID> counterID1("route.counter.0");
  std::optional<RouteCounterID> counterID2("route.counter.1");

  // Add BGP routes with counter ID
  handler.addUnicastRoute(
      bgpClient,
      makeUnicastRoute(prefixA4, cli1_nhop4, bgpClientAdmin, counterID1));
  handler.addUnicastRoute(
      bgpClient,
      makeUnicastRoute(prefixA6, cli1_nhop6, bgpClientAdmin, counterID2));

  // Add random client routes with no counter ID
  handler.addUnicastRoute(
      randomClient, makeUnicastRoute(prefixA4, cli1_nhop4, randomClientAdmin));
  handler.addUnicastRoute(
      randomClient, makeUnicastRoute(prefixA6, cli1_nhop6, randomClientAdmin));

  // BGP route should get selected and counter ID set
  auto state = this->sw_->getState();
  auto rtA4 = findRoute<folly::IPAddressV4>(
      rid, IPAddress::createNetwork(prefixA4), state);
  EXPECT_NE(nullptr, rtA4);
  EXPECT_EQ(rtA4->getForwardInfo().getCounterID(), counterID1);
  EXPECT_EQ(
      rtA4->getEntryForClient(ClientID::BGPD)->getCounterID(), counterID1);
  EXPECT_EQ(
      rtA4->getEntryForClient(static_cast<ClientID>(randomClient))
          ->getCounterID(),
      std::nullopt);
  auto rtA6 = findRoute<folly::IPAddressV6>(
      rid, IPAddress::createNetwork(prefixA6), state);
  EXPECT_NE(nullptr, rtA6);
  EXPECT_EQ(rtA6->getForwardInfo().getCounterID(), counterID2);
  EXPECT_EQ(
      rtA6->getEntryForClient(ClientID::BGPD)->getCounterID(), counterID2);
  EXPECT_EQ(
      rtA6->getEntryForClient(static_cast<ClientID>(randomClient))
          ->getCounterID(),
      std::nullopt);

  // delete BGP routes
  std::vector<IpPrefix> delRoutes = {
      ipPrefix(IPAddress::createNetwork(prefixA4)),
      ipPrefix(IPAddress::createNetwork(prefixA6)),
  };
  XLOG(DBG2) << "Deleting routes";
  handler.deleteUnicastRoutes(
      bgpClient, std::make_unique<std::vector<IpPrefix>>(delRoutes));

  // counter IDs should not be active
  state = this->sw_->getState();
  rtA4 = findRoute<folly::IPAddressV4>(
      rid, IPAddress::createNetwork(prefixA4), state);
  EXPECT_NE(nullptr, rtA4);
  EXPECT_EQ(rtA4->getForwardInfo().getCounterID(), std::nullopt);
  rtA6 = findRoute<folly::IPAddressV6>(
      rid, IPAddress::createNetwork(prefixA6), state);
  EXPECT_NE(nullptr, rtA6);
  EXPECT_EQ(rtA6->getForwardInfo().getCounterID(), std::nullopt);

  // Add back BGP routes with counter ID
  handler.addUnicastRoute(
      bgpClient,
      makeUnicastRoute(prefixA4, cli1_nhop4, bgpClientAdmin, counterID1));
  handler.addUnicastRoute(
      bgpClient,
      makeUnicastRoute(prefixA6, cli1_nhop6, bgpClientAdmin, counterID2));
  state = this->sw_->getState();
  rtA4 = findRoute<folly::IPAddressV4>(
      rid, IPAddress::createNetwork(prefixA4), state);

  // counter id should get set again
  EXPECT_NE(nullptr, rtA4);
  EXPECT_EQ(rtA4->getForwardInfo().getCounterID(), counterID1);
  EXPECT_EQ(
      rtA4->getEntryForClient(ClientID::BGPD)->getCounterID(), counterID1);
  EXPECT_EQ(
      rtA4->getEntryForClient(static_cast<ClientID>(randomClient))
          ->getCounterID(),
      std::nullopt);
  rtA6 = findRoute<folly::IPAddressV6>(
      rid, IPAddress::createNetwork(prefixA6), state);
  EXPECT_NE(nullptr, rtA6);
  EXPECT_EQ(rtA6->getForwardInfo().getCounterID(), counterID2);
  EXPECT_EQ(
      rtA6->getEntryForClient(ClientID::BGPD)->getCounterID(), counterID2);
  EXPECT_EQ(
      rtA6->getEntryForClient(static_cast<ClientID>(randomClient))
          ->getCounterID(),
      std::nullopt);
}

TEST_F(ThriftTest, CounterIDThriftReadTest) {
  RouterID rid = RouterID(0);

  // Create a mock SwSwitch using the config, and wrap it in a ThriftHandler
  ThriftHandler handler(this->sw_);

  auto bgpClient = static_cast<int16_t>(ClientID::BGPD);
  auto bgpClientAdmin = this->sw_->clientIdToAdminDistance(bgpClient);

  auto cli1_nhop4 = "10.0.0.11";
  auto cli1_nhop6 = "2401:db00:2110:3001::0011";

  // These routes will include nexthops from client 10 only
  auto prefixA4 = "7.1.0.0/16";
  auto prefixA6 = "aaaa:1::0/64";
  auto prefixB6 = "aaaa:2::0/64";

  std::optional<RouteCounterID> counterID1("route.counter.0");
  std::optional<RouteCounterID> counterID2("route.counter.1");

  // Add BGP routes with counter ID
  handler.addUnicastRoute(
      bgpClient,
      makeUnicastRoute(prefixA4, cli1_nhop4, bgpClientAdmin, counterID1));
  handler.addUnicastRoute(
      bgpClient,
      makeUnicastRoute(prefixA6, cli1_nhop6, bgpClientAdmin, counterID2));
  // This route shares counterID
  handler.addUnicastRoute(
      bgpClient,
      makeUnicastRoute(prefixB6, cli1_nhop6, bgpClientAdmin, counterID2));

  auto state = this->sw_->getState();
  auto rtA4 = findRoute<folly::IPAddressV4>(
      rid, IPAddress::createNetwork(prefixA4), state);
  EXPECT_NE(nullptr, rtA4);
  auto rtA6 = findRoute<folly::IPAddressV6>(
      rid, IPAddress::createNetwork(prefixA6), state);
  EXPECT_NE(nullptr, rtA6);
  auto rtB6 = findRoute<folly::IPAddressV6>(
      rid, IPAddress::createNetwork(prefixB6), state);
  EXPECT_NE(nullptr, rtB6);

  auto updateCounter = [](auto counterID) {
    auto counter = std::make_unique<MonotonicCounter>(
        *counterID, facebook::fb303::SUM, facebook::fb303::RATE);
    auto now = duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch());
    counter->updateValue(now, 0);
    now = duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch());
    counter->updateValue(now, 64);
  };
  // create and update route counters
  updateCounter(counterID1);
  updateCounter(counterID2);
  CounterCache counters(sw_);
  counters.update();

  std::map<std::string, std::int64_t> routeCounters;
  auto counterIDs = std::make_unique<std::vector<std::string>>();
  auto verifyCounters = [&routeCounters]() {
    EXPECT_EQ(routeCounters.size(), 2);
    for (const auto& counterAndBytes : routeCounters) {
      EXPECT_EQ(counterAndBytes.second, 64);
    }
  };
  handler.getAllRouteCounterBytes(routeCounters);
  verifyCounters();

  routeCounters.clear();
  counterIDs->emplace_back(*counterID1);
  counterIDs->emplace_back(*counterID2);
  handler.getRouteCounterBytes(routeCounters, std::move(counterIDs));
  verifyCounters();

  routeCounters.clear();
  counterIDs = std::make_unique<std::vector<std::string>>();
  // invalid counter should return 0
  counterIDs->emplace_back("invalid");
  handler.getRouteCounterBytes(routeCounters, std::move(counterIDs));
  EXPECT_EQ(routeCounters.size(), 1);
  auto counterBytes = routeCounters.find("invalid");
  EXPECT_NE(counterBytes, routeCounters.end());
  EXPECT_EQ(counterBytes->second, 0);
}
