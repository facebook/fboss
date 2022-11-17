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
#include "fboss/agent/state/Transceiver.h"
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
static TeCounterID kCounterID("counter0");
static std::string kNhopAddrA("2401:db00:2110:3001::0002");
static std::string kNhopAddrB("2401:db00:2110:3055::0002");
static folly::MacAddress kMacAddress("01:02:03:04:05:06");
static VlanID kVlanA(1);
static VlanID kVlanB(55);
static PortID kPortIDA(1);
static PortID kPortIDB(11);

IpPrefix ipPrefix(StringPiece ip, int length) {
  IpPrefix result;
  result.ip() = toBinaryAddress(IPAddress(ip));
  result.prefixLength() = length;
  return result;
}

IpPrefix ipPrefix(const folly::CIDRNetwork& nw) {
  IpPrefix result;
  result.ip() = toBinaryAddress(nw.first);
  result.prefixLength() = nw.second;
  return result;
}

FlowEntry makeFlow(
    std::string dstIp,
    std::string nhip = kNhopAddrA,
    std::string counter = "counter0",
    std::string nhopAddr = "fboss1") {
  FlowEntry flowEntry;
  flowEntry.flow()->srcPort() = 100;
  flowEntry.flow()->dstPrefix() = ipPrefix(dstIp, 64);
  flowEntry.counterID() = counter;
  flowEntry.nextHops()->resize(1);
  flowEntry.nextHops()[0].address() = toBinaryAddress(IPAddress(nhip));
  flowEntry.nextHops()[0].address()->ifName() = nhopAddr;
  return flowEntry;
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
  ThriftHandler handler(sw_);

  // Query the two interfaces configured by testStateA()
  InterfaceDetail info;
  handler.getInterfaceDetail(info, 1);
  EXPECT_EQ("fboss1", *info.interfaceName());
  EXPECT_EQ(1, *info.interfaceId());
  EXPECT_EQ(1, *info.vlanId());
  EXPECT_EQ(0, *info.routerId());
  EXPECT_EQ("00:02:00:00:00:01", *info.mac());
  std::vector<IpPrefix> expectedAddrs = {
      ipPrefix("10.0.0.1", 24),
      ipPrefix("192.168.0.1", 24),
      ipPrefix("2401:db00:2110:3001::0001", 64),
      ipPrefix("fe80::202:ff:fe00:1", 64),
      ipPrefix("fe80::", 64),
  };
  EXPECT_THAT(*info.address(), UnorderedElementsAreArray(expectedAddrs));

  handler.getInterfaceDetail(info, 55);
  EXPECT_EQ("fboss55", *info.interfaceName());
  EXPECT_EQ(55, *info.interfaceId());
  EXPECT_EQ(55, *info.vlanId());
  EXPECT_EQ(0, *info.routerId());
  EXPECT_EQ("00:02:00:00:00:55", *info.mac());
  expectedAddrs = {
      ipPrefix("10.0.55.1", 24),
      ipPrefix("192.168.55.1", 24),
      ipPrefix("2401:db00:2110:3055::0001", 64),
      ipPrefix("fe80::202:ff:fe00:55", 64),
      ipPrefix("169.254.0.0", 16),
  };
  EXPECT_THAT(*info.address(), UnorderedElementsAreArray(expectedAddrs));

  // Calling getInterfaceDetail() on an unknown
  // interface should throw an FbossError.
  EXPECT_THROW(handler.getInterfaceDetail(info, 123), FbossError);
}

TEST_F(ThriftTest, listHwObjects) {
  ThriftHandler handler(sw_);
  std::string out;
  std::vector<HwObjectType> in{HwObjectType::PORT};
  EXPECT_HW_CALL(sw_, listObjects(in, testing::_)).Times(1);
  handler.listHwObjects(
      out, std::make_unique<std::vector<HwObjectType>>(in), false);
}

TEST_F(ThriftTest, getHwDebugDump) {
  ThriftHandler handler(sw_);
  std::string out;
  EXPECT_HW_CALL(sw_, dumpDebugState(testing::_)).Times(1);
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
      case PortSpeed::FIFTYTHREEPOINTONETWOFIVEG:
        EXPECT_EQ(static_cast<int>(key), 53125);
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
      sw_->getRib(), RouterID(0), ip, this->sw_->getState());
  // Verify that a route is found. Link local route should always
  // be present
  ASSERT_NE(nullptr, longestMatchRoute);
  // Verify that the route is to link local addr.
  ASSERT_EQ(longestMatchRoute->prefix().network(), ip);
}

TEST_F(ThriftTest, flushNonExistentNeighbor) {
  ThriftHandler handler(sw_);
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
  ThriftHandler handler(sw_);
  handler.setPortState(port1, true);
  sw_->linkStateChanged(port1, true);
  waitForStateUpdates(sw_);

  auto port = sw_->getState()->getPorts()->getPortIf(port1);
  EXPECT_TRUE(port->isUp());
  EXPECT_TRUE(port->isEnabled());

  sw_->linkStateChanged(port1, false);
  handler.setPortState(port1, false);
  waitForStateUpdates(sw_);

  port = sw_->getState()->getPorts()->getPortIf(port1);
  EXPECT_FALSE(port->isUp());
  EXPECT_FALSE(port->isEnabled());
}

TEST_F(ThriftTest, getAndSetNeighborsToBlock) {
  ThriftHandler handler(sw_);

  auto blockListVerify = [&handler](
                             std::vector<std::pair<VlanID, folly::IPAddress>>
                                 neighborsToBlock) {
    auto cfgNeighborsToBlock = std::make_unique<std::vector<cfg::Neighbor>>();

    for (const auto& [vlanID, ipAddress] : neighborsToBlock) {
      cfg::Neighbor neighbor;
      neighbor.vlanID() = vlanID;
      neighbor.ipAddress() = ipAddress.str();
      cfgNeighborsToBlock->emplace_back(neighbor);
    }
    auto expectedCfgNeighborsToBlock = *cfgNeighborsToBlock;
    handler.setNeighborsToBlock(std::move(cfgNeighborsToBlock));
    waitForStateUpdates(handler.getSw());

    auto gotBlockedNeighbors =
        handler.getSw()->getState()->getSwitchSettings()->getBlockNeighbors();
    EXPECT_EQ(neighborsToBlock, gotBlockedNeighbors);

    std::vector<cfg::Neighbor> gotBlockedNeighborsViaThrift;
    handler.getBlockedNeighbors(gotBlockedNeighborsViaThrift);
    EXPECT_EQ(gotBlockedNeighborsViaThrift, expectedCfgNeighborsToBlock);
  };

  // set blockneighbor1
  blockListVerify(
      {{VlanID(2000), folly::IPAddress("2401:db00:2110:3001::0003")}});

  // set blockneighbor1, blockNeighbor2
  blockListVerify(
      {{VlanID(2000), folly::IPAddress("2401:db00:2110:3001::0003")},
       {VlanID(2000), folly::IPAddress("2401:db00:2110:3001::0004")}});

  // set blockNeighbor2
  blockListVerify(
      {{VlanID(2000), folly::IPAddress("2401:db00:2110:3001::0004")}});

  // set null list (clears block list)
  std::vector<cfg::Neighbor> blockedNeighbors;
  handler.setNeighborsToBlock({});
  waitForStateUpdates(sw_);
  EXPECT_EQ(
      0, sw_->getState()->getSwitchSettings()->getBlockNeighbors().size());
  handler.getBlockedNeighbors(blockedNeighbors);
  EXPECT_TRUE(blockedNeighbors.empty());

  // set empty list (clears block list)
  auto neighborsToBlock = std::make_unique<std::vector<cfg::Neighbor>>();
  handler.setNeighborsToBlock(std::move(neighborsToBlock));
  waitForStateUpdates(sw_);
  EXPECT_EQ(
      0, sw_->getState()->getSwitchSettings()->getBlockNeighbors().size());
  handler.getBlockedNeighbors(blockedNeighbors);
  EXPECT_TRUE(blockedNeighbors.empty());

  auto invalidNeighborToBlock =
      "twshared12345.06.abc7"; // only IPs are supported
  auto invalidNeighborsToBlock = std::make_unique<std::vector<cfg::Neighbor>>();
  cfg::Neighbor neighbor;
  neighbor.vlanID() = 2000;
  neighbor.ipAddress() = invalidNeighborToBlock;
  invalidNeighborsToBlock->emplace_back(neighbor);
  EXPECT_THROW(
      handler.setNeighborsToBlock(std::move(invalidNeighborsToBlock)),
      FbossError);
  handler.getBlockedNeighbors(blockedNeighbors);
  EXPECT_TRUE(blockedNeighbors.empty());
}

TEST_F(ThriftTest, getAndSetMacAddrsToBlock) {
  ThriftHandler handler(sw_);

  auto blockListVerify = [&handler](
                             std::vector<std::pair<VlanID, folly::MacAddress>>
                                 macAddrsToBlock) {
    auto cfgMacAddrsToBlock = std::make_unique<std::vector<cfg::MacAndVlan>>();

    for (const auto& [vlanID, macAddress] : macAddrsToBlock) {
      cfg::MacAndVlan macAndVlan;
      macAndVlan.vlanID() = vlanID;
      macAndVlan.macAddress() = macAddress.toString();
      cfgMacAddrsToBlock->emplace_back(macAndVlan);
    }
    auto expectedCfgMacAddrsToBlock = *cfgMacAddrsToBlock;
    handler.setMacAddrsToBlock(std::move(cfgMacAddrsToBlock));
    waitForStateUpdates(handler.getSw());

    auto gotMacAddrsToBlock =
        handler.getSw()->getState()->getSwitchSettings()->getMacAddrsToBlock();
    EXPECT_EQ(macAddrsToBlock, gotMacAddrsToBlock);

    std::vector<cfg::MacAndVlan> gotMacAddrsToBlockViaThrift;
    handler.getMacAddrsToBlock(gotMacAddrsToBlockViaThrift);
    EXPECT_EQ(gotMacAddrsToBlockViaThrift, expectedCfgMacAddrsToBlock);
  };

  // set blockneighbor1
  blockListVerify({{VlanID(2000), folly::MacAddress("00:11:22:33:44:55")}});

  // set blockneighbor1, blockNeighbor2
  blockListVerify(
      {{VlanID(2000), folly::MacAddress("00:11:22:33:44:55")},
       {VlanID(2000), folly::MacAddress("00:11:22:33:44:66")}});

  // set blockNeighbor2
  blockListVerify({{VlanID(2000), folly::MacAddress("00:11:22:33:44:66")}});

  // set null list (clears block list)
  std::vector<cfg::MacAndVlan> macAddrsToBlock;
  handler.setMacAddrsToBlock({});
  waitForStateUpdates(sw_);
  EXPECT_EQ(
      0, sw_->getState()->getSwitchSettings()->getMacAddrsToBlock().size());
  handler.getMacAddrsToBlock(macAddrsToBlock);
  EXPECT_TRUE(macAddrsToBlock.empty());

  // set empty list (clears block list)
  auto macAddrsToBlock2 = std::make_unique<std::vector<cfg::MacAndVlan>>();
  handler.setMacAddrsToBlock(std::move(macAddrsToBlock2));
  waitForStateUpdates(sw_);
  EXPECT_EQ(
      0, sw_->getState()->getSwitchSettings()->getMacAddrsToBlock().size());
  handler.getMacAddrsToBlock(macAddrsToBlock);
  EXPECT_TRUE(macAddrsToBlock.empty());

  auto invalidMacAddrToBlock =
      "twshared12345.06.abc7"; // only MACs are supported
  auto invalidMacAddrsToBlock =
      std::make_unique<std::vector<cfg::MacAndVlan>>();
  cfg::MacAndVlan macAndVlan;
  macAndVlan.vlanID() = 2000;
  macAndVlan.macAddress() = invalidMacAddrToBlock;
  invalidMacAddrsToBlock->emplace_back(macAndVlan);
  EXPECT_THROW(
      handler.setMacAddrsToBlock(std::move(invalidMacAddrsToBlock)),
      FbossError);
  handler.getMacAddrsToBlock(macAddrsToBlock);
  EXPECT_TRUE(macAddrsToBlock.empty());
}

TEST_F(ThriftTest, setNeighborsToBlockAndMacAddrsToBlock) {
  ThriftHandler handler(sw_);

  auto getNeighborsToBlock = []() {
    auto neighborsToBlock = std::make_unique<std::vector<cfg::Neighbor>>();
    cfg::Neighbor neighbor;
    neighbor.vlanID() = 2000;
    neighbor.ipAddress() = "2401:db00:2110:3001::0003";
    neighborsToBlock->emplace_back(neighbor);
    return neighborsToBlock;
  };

  auto getMacAddrsToBlock = []() {
    auto macAddrsToBlock = std::make_unique<std::vector<cfg::MacAndVlan>>();
    cfg::MacAndVlan macAndVlan;
    macAndVlan.vlanID() = 2000;
    macAndVlan.macAddress() = "00:11:22:33:44:55";
    macAddrsToBlock->emplace_back(macAndVlan);
    return macAddrsToBlock;
  };

  // set neighborsToBlock and then set macAddrsToBlock: expect FAIL
  handler.setNeighborsToBlock(getNeighborsToBlock());
  waitForStateUpdates(handler.getSw());
  EXPECT_THROW(handler.setMacAddrsToBlock(getMacAddrsToBlock()), FbossError);
  waitForStateUpdates(handler.getSw());

  // clear neighborsToBlock and then set macAddrsToBlock: expect PASS
  handler.setNeighborsToBlock({});
  waitForStateUpdates(handler.getSw());
  handler.setMacAddrsToBlock(getMacAddrsToBlock());
  waitForStateUpdates(handler.getSw());

  // macAddrsToBlock already set, now set neighborsToBlock: expect FAIL
  EXPECT_THROW(handler.setNeighborsToBlock(getNeighborsToBlock()), FbossError);
  waitForStateUpdates(handler.getSw());
}

std::unique_ptr<UnicastRoute> makeUnicastRoute(
    std::string prefixStr,
    std::string nxtHop,
    AdminDistance distance = AdminDistance::MAX_ADMIN_DISTANCE,
    std::optional<RouteCounterID> counterID = std::nullopt,
    std::optional<cfg::AclLookupClass> classID = std::nullopt) {
  std::vector<std::string> vec;
  folly::split("/", prefixStr, vec);
  EXPECT_EQ(2, vec.size());
  auto nr = std::make_unique<UnicastRoute>();
  *nr->dest()->ip() = toBinaryAddress(IPAddress(vec.at(0)));
  *nr->dest()->prefixLength() = folly::to<uint8_t>(vec.at(1));
  nr->nextHopAddrs()->push_back(toBinaryAddress(IPAddress(nxtHop)));
  nr->adminDistance() = distance;
  if (counterID.has_value()) {
    nr->counterID() = *counterID;
  }
  if (classID.has_value()) {
    nr->classID() = *classID;
  }
  return nr;
}

// Test for the ThriftHandler::syncFib method
TEST_F(ThriftTest, multipleClientSyncFib) {
  RouterID rid = RouterID(0);

  // Create a mock SwSwitch using the config, and wrap it in a ThriftHandler
  ThriftHandler handler(sw_);

  auto kIntf1 = InterfaceID(1);

  // Two clients - BGP and OPENR
  auto bgpClient = static_cast<int16_t>(ClientID::BGPD);
  auto openrClient = static_cast<int16_t>(ClientID::OPENR);
  auto bgpClientAdmin = sw_->clientIdToAdminDistance(bgpClient);
  auto openrClientAdmin = sw_->clientIdToAdminDistance(openrClient);

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

  auto verifyPrefixesPresent = [&](const auto& prefix4,
                                   const auto& prefix6,
                                   AdminDistance distance) {
    auto state = sw_->getState();
    auto rtA4 = findRoute<folly::IPAddressV4>(
        rid, IPAddress::createNetwork(prefix4), state);
    EXPECT_NE(nullptr, rtA4);
    EXPECT_EQ(
        rtA4->getForwardInfo(),
        RouteNextHopEntry(makeResolvedNextHops({{kIntf1, nhop4}}), distance));

    auto rtA6 = findRoute<folly::IPAddressV6>(
        rid, IPAddress::createNetwork(prefix6), state);
    EXPECT_NE(nullptr, rtA6);
    EXPECT_EQ(
        rtA6->getForwardInfo(),
        RouteNextHopEntry(makeResolvedNextHops({{kIntf1, nhop6}}), distance));
  };
  verifyPrefixesPresent(prefixA4, prefixA6, AdminDistance::EBGP);
  verifyPrefixesPresent(prefixB4, prefixB6, AdminDistance::OPENR);

  auto verifyPrefixesRemoved = [&](const auto& prefix4, const auto& prefix6) {
    auto state = sw_->getState();
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
  verifyPrefixesPresent(prefixB4, prefixB6, AdminDistance::OPENR);
  // verify new BGP prefixes are added
  verifyPrefixesPresent(prefixC4, prefixC6, AdminDistance::EBGP);

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
  verifyPrefixesPresent(prefixD4, prefixD6, AdminDistance::OPENR);

  // Add back BGP and OPENR routes
  addRoutesForClient(prefixA4, prefixA6, bgpClient, bgpClientAdmin);
  addRoutesForClient(prefixB4, prefixB6, openrClient, openrClientAdmin);

  // verify routes added
  verifyPrefixesPresent(prefixA4, prefixA6, AdminDistance::EBGP);
  verifyPrefixesPresent(prefixB4, prefixB6, AdminDistance::OPENR);
}

// Test for the ThriftHandler::syncFib method
TEST_F(ThriftTest, syncFib) {
  RouterID rid = RouterID(0);

  // Create a mock SwSwitch using the config, and wrap it in a ThriftHandler
  ThriftHandler handler(sw_);

  auto randomClient = 500;
  auto bgpClient = static_cast<int16_t>(ClientID::BGPD);
  auto staticClient = static_cast<int16_t>(ClientID::STATIC_ROUTE);
  auto randomClientAdmin = sw_->clientIdToAdminDistance(randomClient);
  auto bgpAdmin = sw_->clientIdToAdminDistance(bgpClient);
  auto staticAdmin = sw_->clientIdToAdminDistance(staticClient);
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
    auto state = sw_->getState();
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
    auto state = sw_->getState();
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
            makeResolvedNextHops({{kIntf1, cli2_nhop4}}), AdminDistance::EBGP));
    // Random client, bgp, static routes. Static shouold win
    auto rtC6 = findRoute<folly::IPAddressV6>(
        rid, IPAddress::createNetwork(prefixC6), state);
    EXPECT_NE(nullptr, rtC6);
    EXPECT_EQ(
        rtC6->getForwardInfo(),
        RouteNextHopEntry(
            makeResolvedNextHops({{kIntf1, cli3_nhop6}}),
            AdminDistance::STATIC_ROUTE));
    auto [v4Routes, v6Routes] = getRouteCount(state);
    EXPECT_EQ(
        8, v4Routes); // 5 intf routes + 2 routes from above + 1 default routes
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
    auto state = sw_->getState();
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
        AdminDistance::EBGP)));

    // Random client, bgp, static routes. Static should win
    auto rtC6 = findRoute<folly::IPAddressV6>(
        rid, IPAddress::createNetwork(prefixC6), state);
    EXPECT_NE(nullptr, rtC6);
    EXPECT_EQ(
        rtC6->getForwardInfo(),
        RouteNextHopEntry(
            makeResolvedNextHops({{kIntf1, cli3_nhop6}}),
            AdminDistance::STATIC_ROUTE));
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
    EXPECT_EQ(8, v4Routes);
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
  ThriftHandler handler(sw_);

  auto randomClient = 500;
  auto bgpClient = static_cast<int16_t>(ClientID::BGPD);
  auto staticClient = static_cast<int16_t>(ClientID::STATIC_ROUTE);
  auto randomClientAdmin = sw_->clientIdToAdminDistance(randomClient);
  auto bgpAdmin = sw_->clientIdToAdminDistance(bgpClient);
  auto staticAdmin = sw_->clientIdToAdminDistance(staticClient);
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
    auto state = sw_->getState();
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
    auto state = sw_->getState();
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
            makeResolvedNextHops({{kIntf1, cli2_nhop4}}), AdminDistance::EBGP));
    // Random client, bgp, static routes. Static should win
    auto rtC6 = findRoute<folly::IPAddressV6>(
        rid, IPAddress::createNetwork(prefixC6), state);
    EXPECT_NE(nullptr, rtC6);
    EXPECT_EQ(
        rtC6->getForwardInfo(),
        RouteNextHopEntry(
            makeResolvedNextHops({{kIntf1, cli3_nhop6}}),
            AdminDistance::STATIC_ROUTE));
    auto [v4Routes, v6Routes] = getRouteCount(state);
    EXPECT_EQ(
        8, v4Routes); // 5 intf routes + 2 routes from above + 1 default routes
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
    auto state = sw_->getState();
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
        AdminDistance::EBGP)));

    // Random client, bgp, static routes. Static should win
    auto rtC6 = findRoute<folly::IPAddressV6>(
        rid, IPAddress::createNetwork(prefixC6), state);
    EXPECT_NE(nullptr, rtC6);
    EXPECT_EQ(
        rtC6->getForwardInfo(),
        RouteNextHopEntry(
            makeResolvedNextHops({{kIntf1, cli3_nhop6}}),
            AdminDistance::STATIC_ROUTE));
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
    EXPECT_EQ(8, v4Routes);
    EXPECT_EQ(6, v6Routes);
  }
}

TEST_F(ThriftTest, delUnicastRoutes) {
  RouterID rid = RouterID(0);

  // Create a mock SwSwitch using the config, and wrap it in a ThriftHandler
  ThriftHandler handler(sw_);

  auto randomClient = 500;
  auto bgpClient = static_cast<int16_t>(ClientID::BGPD);
  auto staticClient = static_cast<int16_t>(ClientID::STATIC_ROUTE);
  auto randomClientAdmin = sw_->clientIdToAdminDistance(randomClient);
  auto bgpAdmin = sw_->clientIdToAdminDistance(bgpClient);
  auto staticAdmin = sw_->clientIdToAdminDistance(staticClient);
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
                         bool expectPresent,
                         const std::string& nhop,
                         AdminDistance distance) {
    auto kIntf1 = InterfaceID(1);
    auto rtC6 = findRoute<folly::IPAddressV6>(
        rid, IPAddress::createNetwork(prefixC6), sw_->getState());
    if (!expectPresent) {
      EXPECT_EQ(nullptr, rtC6);
      return;
    }
    ASSERT_NE(nullptr, rtC6);
    EXPECT_EQ(
        rtC6->getForwardInfo(),
        RouteNextHopEntry(makeResolvedNextHops({{kIntf1, nhop}}), distance));
  };
  // Random client, bgp, static routes. Static should win
  assertRoute(true, cli3_nhop6, AdminDistance::STATIC_ROUTE);
  std::vector<IpPrefix> delRoutes = {
      ipPrefix(IPAddress::createNetwork(prefixC6)),
  };
  // Now delete prefixC6 for Static client. BGP should win
  handler.deleteUnicastRoutes(
      staticClient, std::make_unique<std::vector<IpPrefix>>(delRoutes));
  // Random client, bgp, routes. BGP should win
  assertRoute(true, cli2_nhop6, AdminDistance::EBGP);
  // Now delete prefixC6 for BGP client. random client should win
  // For good measure - use the single route delete API here
  handler.deleteUnicastRoute(
      bgpClient,
      std::make_unique<IpPrefix>(ipPrefix(IPAddress::createNetwork(prefixC6))));
  // Random client routes only
  assertRoute(true, cli1_nhop6, AdminDistance::MAX_ADMIN_DISTANCE);
  // Now delete prefixC6 for random client. Route should be dropped now
  handler.deleteUnicastRoutes(
      randomClient, std::make_unique<std::vector<IpPrefix>>(delRoutes));
  // Random client, bgp, routes. BGP should win
  assertRoute(false, "none", AdminDistance::EBGP);
  // Add routes back and see that lowest admin distance route comes in
  // again
  handler.addUnicastRoute(
      randomClient, makeUnicastRoute(prefixC6, cli1_nhop6, randomClientAdmin));
  // Random client routes only
  assertRoute(true, cli1_nhop6, AdminDistance::MAX_ADMIN_DISTANCE);
  handler.addUnicastRoute(
      bgpClient, makeUnicastRoute(prefixC6, cli2_nhop6, bgpAdmin));
  // Random client, bgp, routes. BGP should win
  assertRoute(true, cli2_nhop6, AdminDistance::EBGP);
  handler.addUnicastRoute(
      staticClient, makeUnicastRoute(prefixC6, cli3_nhop6, staticAdmin));
  // Random client, bgp, static routes. Static should win
  assertRoute(true, cli3_nhop6, AdminDistance::STATIC_ROUTE);
}

TEST_F(ThriftTest, syncFibIsHwProtected) {
  // Create a mock SwSwitch using the config, and wrap it in a ThriftHandler
  ThriftHandler handler(sw_);
  auto addRoutes = std::make_unique<std::vector<UnicastRoute>>();
  UnicastRoute nr1 =
      *makeUnicastRoute("aaaa::/64", "2401:db00:2110:3001::1").get();
  addRoutes->push_back(nr1);
  EXPECT_HW_CALL(sw_, stateChanged(_));
  handler.addUnicastRoutes(10, std::move(addRoutes));
  auto newRoutes = std::make_unique<std::vector<UnicastRoute>>();
  UnicastRoute nr2 = *makeUnicastRoute("bbbb::/64", "42::42").get();
  newRoutes->push_back(nr2);
  // Fail HW update by returning current state
  EXPECT_HW_CALL(sw_, stateChanged(_))
      .Times(::testing::AtLeast(1))
      .WillOnce(Return(sw_->getState()));
  EXPECT_THROW(
      {
        try {
          handler.syncFib(10, std::move(newRoutes));
        } catch (const FbossFibUpdateError& fibError) {
          EXPECT_EQ(fibError.vrf2failedAddUpdatePrefixes()->size(), 1);
          auto itr = fibError.vrf2failedAddUpdatePrefixes()->find(0);
          EXPECT_EQ(itr->second.size(), 1);
          itr = fibError.vrf2failedDeletePrefixes()->find(0);
          EXPECT_EQ(itr->second.size(), 1);
          throw;
        }
      },
      FbossFibUpdateError);
}

TEST_F(ThriftTest, addUnicastRoutesIsHwProtected) {
  ThriftHandler handler(sw_);
  auto newRoutes = std::make_unique<std::vector<UnicastRoute>>();
  UnicastRoute nr1 = *makeUnicastRoute("aaaa::/64", "42::42").get();
  newRoutes->push_back(nr1);
  // Fail HW update by returning current state
  EXPECT_HW_CALL(sw_, stateChanged(_)).WillOnce(Return(sw_->getState()));
  EXPECT_THROW(
      {
        try {
          handler.addUnicastRoutes(10, std::move(newRoutes));

        } catch (const FbossFibUpdateError& fibError) {
          EXPECT_EQ(fibError.vrf2failedAddUpdatePrefixes()->size(), 1);
          auto itr = fibError.vrf2failedAddUpdatePrefixes()->find(0);
          EXPECT_EQ(itr->second.size(), 1);
          throw;
        }
      },
      FbossFibUpdateError);
}

TEST_F(ThriftTest, getRouteTable) {
  ThriftHandler handler(sw_);
  auto [v4Routes, v6Routes] = getRouteCount(sw_->getState());
  std::vector<UnicastRoute> routeTable;
  handler.getRouteTable(routeTable);
  // 7 intf routes + 2 default routes + 1 link local route
  EXPECT_EQ(10, v4Routes + v6Routes);
  EXPECT_EQ(10, routeTable.size());
}

TEST_F(ThriftTest, getRouteDetails) {
  ThriftHandler handler(sw_);
  auto [v4Routes, v6Routes] = getRouteCount(sw_->getState());
  std::vector<RouteDetails> routeDetails;
  handler.getRouteTableDetails(routeDetails);
  // 7 intf routes + 2 default routes + 1 link local route
  EXPECT_EQ(10, v4Routes + v6Routes);
  EXPECT_EQ(10, routeDetails.size());
}

TEST_F(ThriftTest, getRouteTableByClient) {
  ThriftHandler handler(sw_);
  std::vector<UnicastRoute> routeTable;
  handler.getRouteTableByClient(
      routeTable, static_cast<int16_t>(ClientID::INTERFACE_ROUTE));
  // 6 intf routes + 2 default routes + 1 link local route
  EXPECT_EQ(7, routeTable.size());
}
std::unique_ptr<MplsRoute> makeMplsRoute(
    int32_t mplsLabel,
    std::string nxtHop,
    AdminDistance distance = AdminDistance::MAX_ADMIN_DISTANCE) {
  auto nr = std::make_unique<MplsRoute>();
  nr->topLabel() = mplsLabel;
  NextHopThrift nh;
  MplsAction mplsAction;
  mplsAction.action() = MplsActionCode::POP_AND_LOOKUP;
  nh.address() = toBinaryAddress(IPAddress(nxtHop));
  nh.mplsAction() = mplsAction;
  nr->nextHops()->push_back(nh);
  nr->adminDistance() = distance;
  return nr;
}

TEST_F(ThriftTest, syncMplsFibIsHwProtected) {
  // Create a mock SwSwitch using the config, and wrap it in a ThriftHandler
  ThriftHandler handler(sw_);
  auto newRoutes = std::make_unique<std::vector<MplsRoute>>();
  MplsRoute nr1 = *makeMplsRoute(101, "10.0.0.2").get();
  newRoutes->push_back(nr1);
  // Fail HW update by returning current state
  EXPECT_HW_CALL(sw_, stateChanged(_)).WillRepeatedly(Return(sw_->getState()));
  EXPECT_THROW(
      {
        try {
          handler.syncMplsFib(10, std::move(newRoutes));
        } catch (const FbossFibUpdateError& fibError) {
          EXPECT_EQ(fibError.failedAddUpdateMplsLabels()->size(), 1);
          EXPECT_EQ(*fibError.failedAddUpdateMplsLabels()->begin(), 101);
          throw;
        }
      },
      FbossFibUpdateError);
}

TEST_F(ThriftTest, addMplsRoutesIsHwProtected) {
  // Create a mock SwSwitch using the config, and wrap it in a ThriftHandler
  ThriftHandler handler(sw_);
  auto newRoutes = std::make_unique<std::vector<MplsRoute>>();
  MplsRoute nr1 = *makeMplsRoute(101, "10.0.0.2").get();
  newRoutes->push_back(nr1);
  // Fail HW update by returning current state
  EXPECT_HW_CALL(sw_, stateChanged(_)).WillRepeatedly(Return(sw_->getState()));
  EXPECT_THROW(
      {
        try {
          handler.addMplsRoutes(10, std::move(newRoutes));
        } catch (const FbossFibUpdateError& fibError) {
          EXPECT_EQ(fibError.failedAddUpdateMplsLabels()->size(), 1);
          EXPECT_EQ(*fibError.failedAddUpdateMplsLabels()->begin(), 101);
          throw;
        }
      },
      FbossFibUpdateError);
}

TEST_F(ThriftTest, hwUpdateErrorAfterPartialUpdate) {
  ThriftHandler handler(sw_);
  std::vector<UnicastRoute>();
  UnicastRoute nr1 =
      *makeUnicastRoute("aaaa::/64", "2401:db00:2110:3001::1").get();
  std::vector<UnicastRoute> routes;
  routes.push_back(nr1);
  EXPECT_HW_CALL(sw_, stateChanged(_)).Times(2);
  handler.addUnicastRoutes(
      10, std::make_unique<std::vector<UnicastRoute>>(routes));
  auto oneRouteAddedState = sw_->getState();
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
  EXPECT_HW_CALL(sw_, stateChanged(_))
      .Times(::testing::AtLeast(1))
      .WillOnce(Return(oneRouteAddedState));
  EXPECT_THROW(
      {
        try {
          handler.addUnicastRoutes(
              10, std::make_unique<std::vector<UnicastRoute>>(routes));
        } catch (const FbossFibUpdateError& fibError) {
          EXPECT_EQ(fibError.vrf2failedAddUpdatePrefixes()->size(), 1);
          auto itr = fibError.vrf2failedAddUpdatePrefixes()->find(0);
          EXPECT_EQ(itr->second.size(), 1);
          itr = fibError.vrf2failedDeletePrefixes()->find(0);
          EXPECT_EQ(itr->second.size(), 0);
          throw;
        }
      },
      FbossFibUpdateError);
}

TEST_F(ThriftTest, routeUpdatesWithConcurrentReads) {
  auto thriftHgridRoutes =
      utility::HgridDuRouteScaleGenerator(sw_->getState(), 100000)
          .getThriftRoutes()[0];
  auto thriftRswRoutes = utility::RSWRouteScaleGenerator(sw_->getState(), 10000)
                             .getThriftRoutes()[0];
  std::atomic<bool> done{false};

  ThriftHandler handler(sw_);
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

TEST_F(ThriftTest, UnicastRoutesWithClassID) {
  RouterID rid = RouterID(0);

  // Create a mock SwSwitch using the config, and wrap it in a ThriftHandler
  ThriftHandler handler(sw_);

  auto randomClient = 500;
  auto randomClientAdmin = sw_->clientIdToAdminDistance(randomClient);
  auto bgpClient = static_cast<int16_t>(ClientID::BGPD);
  auto bgpClientAdmin = sw_->clientIdToAdminDistance(bgpClient);

  auto cli1_nhop4 = "10.0.0.11";
  auto cli1_nhop6 = "2401:db00:2110:3001::0011";

  // These routes will include nexthops from client 10 only
  auto prefixA4 = "7.1.0.0/16";
  auto prefixA6 = "aaaa:1::0/64";

  std::optional<cfg::AclLookupClass> classID1(
      cfg::AclLookupClass::DST_CLASS_L3_DPR);
  std::optional<cfg::AclLookupClass> classID2(
      cfg::AclLookupClass::DST_CLASS_L3_LOCAL_IP6);

  // Add BGP routes with class ID
  handler.addUnicastRoute(
      bgpClient,
      makeUnicastRoute(
          prefixA4, cli1_nhop4, bgpClientAdmin, std::nullopt, classID1));
  handler.addUnicastRoute(
      bgpClient,
      makeUnicastRoute(
          prefixA6, cli1_nhop6, bgpClientAdmin, std::nullopt, classID2));

  // Add random client routes with no class ID
  handler.addUnicastRoute(
      randomClient, makeUnicastRoute(prefixA4, cli1_nhop4, randomClientAdmin));
  handler.addUnicastRoute(
      randomClient, makeUnicastRoute(prefixA6, cli1_nhop6, randomClientAdmin));

  // BGP route should get selected and class ID set
  auto state = sw_->getState();
  auto rtA4 = findRoute<folly::IPAddressV4>(
      rid, IPAddress::createNetwork(prefixA4), state);
  EXPECT_NE(nullptr, rtA4);
  EXPECT_EQ(rtA4->getClassID(), classID1);
  EXPECT_EQ(rtA4->getForwardInfo().getClassID(), classID1);
  if (auto classID = rtA4->getEntryForClient(ClientID::BGPD)->classID()) {
    EXPECT_EQ(*classID, classID1);
  }
  EXPECT_TRUE(!(
      rtA4->getEntryForClient(static_cast<ClientID>(randomClient))->classID()));
  auto rtA6 = findRoute<folly::IPAddressV6>(
      rid, IPAddress::createNetwork(prefixA6), state);
  EXPECT_NE(nullptr, rtA6);
  EXPECT_EQ(rtA6->getClassID(), classID2);
  EXPECT_EQ(rtA6->getForwardInfo().getClassID(), classID2);
  EXPECT_EQ(*(rtA6->getEntryForClient(ClientID::BGPD)->classID()), classID2);
  EXPECT_FALSE(
      rtA6->getEntryForClient(static_cast<ClientID>(randomClient))->classID());

  // delete BGP routes
  std::vector<IpPrefix> delRoutes = {
      ipPrefix(IPAddress::createNetwork(prefixA4)),
      ipPrefix(IPAddress::createNetwork(prefixA6)),
  };
  XLOG(DBG2) << "Deleting routes";
  handler.deleteUnicastRoutes(
      bgpClient, std::make_unique<std::vector<IpPrefix>>(delRoutes));

  // class IDs should not be active
  state = sw_->getState();
  rtA4 = findRoute<folly::IPAddressV4>(
      rid, IPAddress::createNetwork(prefixA4), state);
  EXPECT_NE(nullptr, rtA4);
  EXPECT_EQ(rtA4->getClassID(), std::nullopt);
  EXPECT_EQ(rtA4->getForwardInfo().getClassID(), std::nullopt);
  rtA6 = findRoute<folly::IPAddressV6>(
      rid, IPAddress::createNetwork(prefixA6), state);
  EXPECT_NE(nullptr, rtA6);
  EXPECT_EQ(rtA6->getClassID(), std::nullopt);
  EXPECT_EQ(rtA6->getForwardInfo().getClassID(), std::nullopt);

  // Add back BGP routes with class ID
  handler.addUnicastRoute(
      bgpClient,
      makeUnicastRoute(
          prefixA4, cli1_nhop4, bgpClientAdmin, std::nullopt, classID1));
  handler.addUnicastRoute(
      bgpClient,
      makeUnicastRoute(
          prefixA6, cli1_nhop6, bgpClientAdmin, std::nullopt, classID2));
  state = sw_->getState();
  rtA4 = findRoute<folly::IPAddressV4>(
      rid, IPAddress::createNetwork(prefixA4), state);

  // class id should get set again
  EXPECT_NE(nullptr, rtA4);
  EXPECT_EQ(rtA4->getClassID(), classID1);
  EXPECT_EQ(rtA4->getForwardInfo().getClassID(), classID1);
  EXPECT_EQ(*(rtA4->getEntryForClient(ClientID::BGPD)->classID()), classID1);
  EXPECT_FALSE(
      rtA4->getEntryForClient(static_cast<ClientID>(randomClient))->classID());
  rtA6 = findRoute<folly::IPAddressV6>(
      rid, IPAddress::createNetwork(prefixA6), state);
  EXPECT_NE(nullptr, rtA6);
  EXPECT_EQ(rtA6->getClassID(), classID2);
  EXPECT_EQ(rtA6->getForwardInfo().getClassID(), classID2);
  EXPECT_EQ(*(rtA6->getEntryForClient(ClientID::BGPD)->classID()), classID2);
  EXPECT_FALSE(
      rtA6->getEntryForClient(static_cast<ClientID>(randomClient))->classID());
}

TEST_F(ThriftTest, UnicastRoutesWithCounterID) {
  RouterID rid = RouterID(0);

  // Create a mock SwSwitch using the config, and wrap it in a ThriftHandler
  ThriftHandler handler(sw_);

  auto randomClient = 500;
  auto randomClientAdmin = sw_->clientIdToAdminDistance(randomClient);
  auto bgpClient = static_cast<int16_t>(ClientID::BGPD);
  auto bgpClientAdmin = sw_->clientIdToAdminDistance(bgpClient);

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
  auto state = sw_->getState();
  auto rtA4 = findRoute<folly::IPAddressV4>(
      rid, IPAddress::createNetwork(prefixA4), state);
  EXPECT_NE(nullptr, rtA4);
  EXPECT_EQ(rtA4->getForwardInfo().getCounterID(), counterID1);
  EXPECT_EQ(
      *(rtA4->getEntryForClient(ClientID::BGPD)->counterID()), counterID1);
  EXPECT_FALSE(rtA4->getEntryForClient(static_cast<ClientID>(randomClient))
                   ->counterID());
  auto rtA6 = findRoute<folly::IPAddressV6>(
      rid, IPAddress::createNetwork(prefixA6), state);
  EXPECT_NE(nullptr, rtA6);
  EXPECT_EQ(rtA6->getForwardInfo().getCounterID(), counterID2);
  EXPECT_EQ(
      *(rtA6->getEntryForClient(ClientID::BGPD)->counterID()), counterID2);
  EXPECT_FALSE(rtA6->getEntryForClient(static_cast<ClientID>(randomClient))
                   ->counterID());

  // delete BGP routes
  std::vector<IpPrefix> delRoutes = {
      ipPrefix(IPAddress::createNetwork(prefixA4)),
      ipPrefix(IPAddress::createNetwork(prefixA6)),
  };
  XLOG(DBG2) << "Deleting routes";
  handler.deleteUnicastRoutes(
      bgpClient, std::make_unique<std::vector<IpPrefix>>(delRoutes));

  // counter IDs should not be active
  state = sw_->getState();
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
  state = sw_->getState();
  rtA4 = findRoute<folly::IPAddressV4>(
      rid, IPAddress::createNetwork(prefixA4), state);

  // counter id should get set again
  EXPECT_NE(nullptr, rtA4);
  EXPECT_EQ(rtA4->getForwardInfo().getCounterID(), counterID1);
  EXPECT_EQ(
      *(rtA4->getEntryForClient(ClientID::BGPD)->counterID()), counterID1);
  EXPECT_FALSE(rtA4->getEntryForClient(static_cast<ClientID>(randomClient))
                   ->counterID());
  rtA6 = findRoute<folly::IPAddressV6>(
      rid, IPAddress::createNetwork(prefixA6), state);
  EXPECT_NE(nullptr, rtA6);
  EXPECT_EQ(rtA6->getForwardInfo().getCounterID(), counterID2);
  EXPECT_EQ(
      *(rtA6->getEntryForClient(ClientID::BGPD)->counterID()), counterID2);
  EXPECT_FALSE(rtA6->getEntryForClient(static_cast<ClientID>(randomClient))
                   ->counterID());
}

TEST_F(ThriftTest, CounterIDThriftReadTest) {
  RouterID rid = RouterID(0);

  // Create a mock SwSwitch using the config, and wrap it in a ThriftHandler
  ThriftHandler handler(sw_);

  auto bgpClient = static_cast<int16_t>(ClientID::BGPD);
  auto bgpClientAdmin = sw_->clientIdToAdminDistance(bgpClient);

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

  auto state = sw_->getState();
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

TEST_F(ThriftTest, getRouteTableVerifyCounterID) {
  ThriftHandler handler(sw_);
  auto bgpClient = static_cast<int16_t>(ClientID::BGPD);
  auto bgpClientAdmin = sw_->clientIdToAdminDistance(bgpClient);

  auto cli1_nhop6 = "2401:db00:2110:3001::0011";
  auto prefixA6 = "aaaa:1::0/64";
  auto addrA6 = folly::IPAddress("aaaa:1::0");
  std::optional<RouteCounterID> counterID1("route.counter.0");

  // Add BGP routes with counter ID
  handler.addUnicastRoute(
      bgpClient,
      makeUnicastRoute(prefixA6, cli1_nhop6, bgpClientAdmin, counterID1));

  std::vector<UnicastRoute> routeTable;
  bool found = false;
  handler.getRouteTable(routeTable);
  for (const auto& rt : routeTable) {
    if (rt.dest()->ip() == toBinaryAddress(addrA6)) {
      EXPECT_EQ(*rt.counterID(), *counterID1);
      found = true;
      break;
    }
  }
  EXPECT_TRUE(found);

  routeTable.resize(0);
  found = false;
  handler.getRouteTableByClient(routeTable, bgpClient);
  for (const auto& rt : routeTable) {
    if (rt.dest()->ip() == toBinaryAddress(addrA6)) {
      EXPECT_EQ(*rt.counterID(), *counterID1);
      found = true;
      break;
    }
  }
  EXPECT_TRUE(found);

  std::vector<RouteDetails> routeDetails;
  found = false;
  handler.getRouteTableDetails(routeDetails);
  for (const auto& route : routeDetails) {
    if (route.dest()->ip() == toBinaryAddress(addrA6)) {
      EXPECT_EQ(*route.counterID(), *counterID1);
      found = true;
      break;
    }
  }
  EXPECT_TRUE(found);

  UnicastRoute route;
  auto addr = std::make_unique<facebook::network::thrift::Address>(
      facebook::network::toAddress(IPAddress("aaaa:1::")));
  handler.getIpRoute(route, std::move(addr), RouterID(0));
  EXPECT_EQ(*route.counterID(), *counterID1);
}

TEST_F(ThriftTest, getRouteTableVerifyClassID) {
  ThriftHandler handler(sw_);
  auto bgpClient = static_cast<int16_t>(ClientID::BGPD);
  auto bgpClientAdmin = sw_->clientIdToAdminDistance(bgpClient);

  auto cli1_nhop6 = "2401:db00:2110:3001::0011";
  auto prefixA6 = "aaaa:1::0/64";
  auto addrA6 = folly::IPAddress("aaaa:1::0");
  std::optional<cfg::AclLookupClass> classID1(
      cfg::AclLookupClass::DST_CLASS_L3_DPR);

  // Add BGP routes with class ID
  handler.addUnicastRoute(
      bgpClient,
      makeUnicastRoute(
          prefixA6, cli1_nhop6, bgpClientAdmin, std::nullopt, classID1));

  std::vector<UnicastRoute> routeTable;
  bool found = false;
  handler.getRouteTable(routeTable);
  for (const auto& rt : routeTable) {
    if (rt.dest()->ip() == toBinaryAddress(addrA6)) {
      EXPECT_EQ(*rt.classID(), *classID1);
      found = true;
      break;
    }
  }
  EXPECT_TRUE(found);

  routeTable.resize(0);
  found = false;
  handler.getRouteTableByClient(routeTable, bgpClient);
  for (const auto& rt : routeTable) {
    if (rt.dest()->ip() == toBinaryAddress(addrA6)) {
      EXPECT_EQ(*rt.classID(), *classID1);
      found = true;
      break;
    }
  }
  EXPECT_TRUE(found);

  std::vector<RouteDetails> routeDetails;
  found = false;
  handler.getRouteTableDetails(routeDetails);
  for (const auto& route : routeDetails) {
    if (route.dest()->ip() == toBinaryAddress(addrA6)) {
      EXPECT_EQ(*route.classID(), *classID1);
      found = true;
      break;
    }
  }
  EXPECT_TRUE(found);

  UnicastRoute route;
  auto addr = std::make_unique<facebook::network::thrift::Address>(
      facebook::network::toAddress(IPAddress("aaaa:1::")));
  handler.getIpRoute(route, std::move(addr), RouterID(0));
  EXPECT_EQ(*route.classID(), *classID1);
}

TEST_F(ThriftTest, getLoopbackMode) {
  ThriftHandler handler(sw_);
  std::map<int32_t, PortLoopbackMode> port2LoopbackMode;
  handler.getAllPortLoopbackMode(port2LoopbackMode);
  EXPECT_EQ(port2LoopbackMode.size(), sw_->getState()->getPorts()->size());
  std::for_each(
      port2LoopbackMode.begin(),
      port2LoopbackMode.end(),
      [](auto& portAndLbMode) {
        EXPECT_EQ(portAndLbMode.second, PortLoopbackMode::NONE);
      });
}

TEST_F(ThriftTest, setLoopbackMode) {
  ThriftHandler handler(sw_);
  std::map<int32_t, PortLoopbackMode> port2LoopbackMode;
  auto firstPort = (*sw_->getState()->getPorts()->begin())->getID();
  auto otherPortsUnchanged = [firstPort, this]() {
    for (auto& port : *sw_->getState()->getPorts()) {
      if (port->getID() != firstPort) {
        EXPECT_EQ(port->getLoopbackMode(), cfg::PortLoopbackMode::NONE);
      }
    }
  };

  for (auto lbMode :
       {PortLoopbackMode::MAC, PortLoopbackMode::PHY, PortLoopbackMode::NONE}) {
    // MAC
    handler.setPortLoopbackMode(firstPort, lbMode);
    handler.getAllPortLoopbackMode(port2LoopbackMode);
    EXPECT_EQ(port2LoopbackMode.size(), sw_->getState()->getPorts()->size());
    EXPECT_EQ(port2LoopbackMode.find(firstPort)->second, lbMode);
    otherPortsUnchanged();
  }
}

TEST_F(ThriftTest, programInternalPhyPorts) {
  ThriftHandler handler(sw_);
  // Using Wedge100PlatformMapping in MockPlatform. Transceiver 4 is used by
  // eth1/5/[1-4], which is software port 1/2/3/4
  TransceiverID id = TransceiverID(4);
  constexpr auto kEnabledPort = 1;
  constexpr auto kCableLength = 3.5;
  constexpr auto kMediaInterface = MediaInterfaceCode::CWDM4_100G;
  constexpr auto kComplianceCode = ExtendedSpecComplianceCode::CWDM4_100G;
  constexpr auto kManagementInterface = TransceiverManagementInterface::CMIS;
  auto preparedTcvrInfo = [id,
                           kMediaInterface,
                           kComplianceCode,
                           kManagementInterface](double cableLength) {
    auto tcvrInfo = std::make_unique<TransceiverInfo>();
    tcvrInfo->port() = id;
    tcvrInfo->present() = true;
    Cable cable;
    cable.length() = cableLength;
    tcvrInfo->cable() = cable;
    tcvrInfo->transceiverManagementInterface() = kManagementInterface;
    TransceiverSettings tcvrSettings;
    std::vector<MediaInterfaceId> mediaInterfaces;
    for (int i = 0; i < 4; i++) {
      MediaInterfaceId intf;
      intf.lane() = i;
      intf.code() = kMediaInterface;
      mediaInterfaces.push_back(intf);
    }
    tcvrSettings.mediaInterface() = std::move(mediaInterfaces);
    tcvrInfo->settings() = tcvrSettings;
    return tcvrInfo;
  };

  // Only enabled ports should be return
  auto checkProgrammedPorts =
      [&](const std::map<int32_t, cfg::PortProfileID>& programmedPorts) {
        EXPECT_EQ(programmedPorts.size(), 1);
        EXPECT_TRUE(
            programmedPorts.find(kEnabledPort) != programmedPorts.end());
        // Controlling port should be 100G
        EXPECT_EQ(
            programmedPorts.find(kEnabledPort)->second,
            cfg::PortProfileID::PROFILE_100G_4_NRZ_CL91_COPPER);

        // Make sure the enabled ports using the new profile config/pin configs
        auto platform = sw_->getPlatform();
        auto tcvr = sw_->getState()->getTransceivers()->getTransceiverIf(id);
        std::optional<cfg::PlatformPortConfigOverrideFactor> factor;
        if (tcvr != nullptr) {
          factor = tcvr->toPlatformPortConfigOverrideFactor();
        }
        platform->getPlatformMapping()
            ->customizePlatformPortConfigOverrideFactor(factor);
        // Port must exist in the SwitchState
        const auto port =
            sw_->getState()->getPorts()->getPort(PortID(kEnabledPort));
        EXPECT_TRUE(port->isEnabled());
        PlatformPortProfileConfigMatcher matcher{
            port->getProfileID(), port->getID(), factor};
        auto portProfileCfg = platform->getPortProfileConfig(matcher);
        CHECK(portProfileCfg) << "No port profile config found with matcher:"
                              << matcher.toString();
        auto expectedProfileConfig = *portProfileCfg->iphy();
        const auto& expectedPinConfigs =
            platform->getPlatformMapping()->getPortIphyPinConfigs(matcher);

        EXPECT_TRUE(expectedProfileConfig == port->getProfileConfig());
        EXPECT_TRUE(expectedPinConfigs == port->getPinConfigs());
      };

  std::map<int32_t, cfg::PortProfileID> programmedPorts;
  handler.programInternalPhyPorts(
      programmedPorts, preparedTcvrInfo(kCableLength), false);

  checkProgrammedPorts(programmedPorts);
  auto tcvr = sw_->getState()->getTransceivers()->getTransceiver(id);
  EXPECT_EQ(tcvr->getID(), id);
  EXPECT_EQ(*tcvr->getCableLength(), kCableLength);
  EXPECT_EQ(*tcvr->getMediaInterface(), kMediaInterface);
  EXPECT_EQ(*tcvr->getManagementInterface(), kManagementInterface);

  // Now change the cable length
  const auto oldPort =
      sw_->getState()->getPorts()->getPort(PortID(kEnabledPort));
  constexpr auto kCableLength2 = 1;
  std::map<int32_t, cfg::PortProfileID> programmedPorts2;
  handler.programInternalPhyPorts(
      programmedPorts2, preparedTcvrInfo(kCableLength2), false);

  checkProgrammedPorts(programmedPorts2);
  tcvr = sw_->getState()->getTransceivers()->getTransceiver(id);
  EXPECT_EQ(tcvr->getID(), id);
  EXPECT_EQ(*tcvr->getCableLength(), kCableLength2);
  EXPECT_EQ(*tcvr->getMediaInterface(), kMediaInterface);
  EXPECT_EQ(*tcvr->getManagementInterface(), kManagementInterface);
  const auto newPort =
      sw_->getState()->getPorts()->getPort(PortID(kEnabledPort));
  // Because we're using Wedge100PlatformMapping here, we should see pinConfig
  // change due to the cable length change
  EXPECT_TRUE(oldPort->getProfileConfig() == newPort->getProfileConfig());
  EXPECT_TRUE(oldPort->getPinConfigs() != newPort->getPinConfigs());

  // Using the same transceiver info to program and no new state created
  auto beforeGen = sw_->getState()->getGeneration();
  programmedPorts2.clear();
  handler.programInternalPhyPorts(
      programmedPorts2, preparedTcvrInfo(kCableLength2), false);
  checkProgrammedPorts(programmedPorts2);
  EXPECT_EQ(beforeGen, sw_->getState()->getGeneration());

  // Finally remove the transceiver
  auto unpresentTcvr = std::make_unique<TransceiverInfo>();
  unpresentTcvr->port() = id;
  unpresentTcvr->present() = false;
  std::map<int32_t, cfg::PortProfileID> programmedPorts3;
  handler.programInternalPhyPorts(
      programmedPorts3, std::move(unpresentTcvr), false);

  // Still return programmed ports even though no transceiver there
  checkProgrammedPorts(programmedPorts3);
  tcvr = sw_->getState()->getTransceivers()->getTransceiverIf(id);
  EXPECT_TRUE(tcvr == nullptr);

  // Remove the same Transceiver again, and make sure no new state created.
  beforeGen = sw_->getState()->getGeneration();
  programmedPorts3.clear();
  unpresentTcvr = std::make_unique<TransceiverInfo>();
  unpresentTcvr->port() = id;
  unpresentTcvr->present() = false;
  handler.programInternalPhyPorts(
      programmedPorts3, std::move(unpresentTcvr), false);
  // Still return programmed ports even though no transceiver there
  checkProgrammedPorts(programmedPorts3);
  tcvr = sw_->getState()->getTransceivers()->getTransceiverIf(id);
  EXPECT_TRUE(tcvr == nullptr);
  EXPECT_EQ(beforeGen, sw_->getState()->getGeneration());
}

TEST_F(ThriftTest, getConfigAppliedInfo) {
  ThriftHandler handler(sw_);
  // The SetUp() will applied an initialed config, so we should verify the
  // lastConfigAppliedInMs should be > 0 and < now.
  ConfigAppliedInfo initConfigAppliedInfo;
  handler.getConfigAppliedInfo(initConfigAppliedInfo);
  auto initConfigAppliedInMs = *initConfigAppliedInfo.lastAppliedInMs();
  EXPECT_GT(initConfigAppliedInMs, 0);
  // Thrift test should always trigger coldboot
  auto coldbootConfigAppliedTime =
      initConfigAppliedInfo.lastColdbootAppliedInMs();
  if (coldbootConfigAppliedTime) {
    EXPECT_EQ(*coldbootConfigAppliedTime, initConfigAppliedInMs);
  } else {
    throw FbossError("No coldboot config applied time");
  }

  // Adding sleep in case we immediatly check the last config applied time
  /* sleep override */
  usleep(1000);
  auto currentInMs = std::chrono::duration_cast<std::chrono::milliseconds>(
                         std::chrono::system_clock::now().time_since_epoch())
                         .count();
  EXPECT_LT(initConfigAppliedInMs, currentInMs);

  // Try to apply a new config, the lastConfigAppliedTime should changed
  // Adding sleep in case we apply a new config immediatly after the last config
  /* sleep override */
  usleep(1000);
  sw_->applyConfig(
      "New config with new speed profile", testConfigAWithLookupClasses());

  ConfigAppliedInfo newConfigAppliedInfo;
  handler.getConfigAppliedInfo(newConfigAppliedInfo);
  auto newConfigAppliedInMs = *newConfigAppliedInfo.lastAppliedInMs();
  EXPECT_GT(newConfigAppliedInMs, initConfigAppliedInMs);
  // Coldboot time should not change
  if (auto newColdbootConfigAppliedTime =
          newConfigAppliedInfo.lastColdbootAppliedInMs()) {
    EXPECT_EQ(*newColdbootConfigAppliedTime, *coldbootConfigAppliedTime);
  } else {
    throw FbossError("No coldboot config applied time");
  }
}

TEST_F(ThriftTest, applySpeedAndProfileMismatchConfig) {
  ThriftHandler handler(sw_);
  auto mismatchConfig = testConfigA();
  // Change the speed mismatch with the profile
  CHECK(!mismatchConfig.ports()->empty());
  mismatchConfig.ports()[0].state() = cfg::PortState::ENABLED;
  mismatchConfig.ports()[0].speed() = cfg::PortSpeed::TWENTYFIVEG;
  mismatchConfig.ports()[0].profileID() =
      cfg::PortProfileID::PROFILE_100G_4_NRZ_CL91_COPPER;

  EXPECT_THROW(
      sw_->applyConfig(
          "Mismatch config with wrong speed and profile", mismatchConfig),
      FbossError);
}

class ThriftTeFlowTest : public ::testing::Test {
 public:
  void SetUp() override {
    auto config = testConfigA();
    handle_ = createTestHandle(&config);
    sw_ = handle_->getSw();
    sw_->initialConfigApplied(std::chrono::steady_clock::now());
    sw_->getNeighborUpdater()->receivedNdpMine(
        kVlanA,
        folly::IPAddressV6(kNhopAddrA),
        kMacAddress,
        PortDescriptor(kPortIDA),
        ICMPv6Type::ICMPV6_TYPE_NDP_NEIGHBOR_ADVERTISEMENT,
        0);
    sw_->getNeighborUpdater()->receivedNdpMine(
        kVlanB,
        folly::IPAddressV6(kNhopAddrB),
        kMacAddress,
        PortDescriptor(kPortIDB),
        ICMPv6Type::ICMPV6_TYPE_NDP_NEIGHBOR_ADVERTISEMENT,
        0);

    sw_->getNeighborUpdater()->waitForPendingUpdates();
    waitForBackgroundThread(sw_);
    waitForStateUpdates(sw_);
  }
  SwSwitch* sw_;
  std::unique_ptr<HwTestHandle> handle_;
};

TEST_F(ThriftTeFlowTest, addRemoveTeFlow) {
  ThriftHandler handler(sw_);
  auto teFlowEntries = std::make_unique<std::vector<FlowEntry>>();
  auto flowEntry = makeFlow("100::1");
  teFlowEntries->emplace_back(flowEntry);
  handler.addTeFlows(std::move(teFlowEntries));
  auto state = sw_->getState();
  auto teFlowTable = state->getTeFlowTable();
  auto verifyEntry = [&teFlowTable](
                         const auto& flow,
                         const auto& nhop,
                         const auto& counter,
                         const auto& intf) {
    EXPECT_EQ(teFlowTable->size(), 1);
    auto tableEntry = teFlowTable->getTeFlowIf(flow);
    EXPECT_NE(tableEntry, nullptr);
    EXPECT_EQ(*tableEntry->getCounterID(), counter);
    EXPECT_EQ(tableEntry->getNextHops()->size(), 1);
    auto expectedNhop = toBinaryAddress(IPAddress(nhop));
    expectedNhop.ifName() = intf;
    auto nexthop = tableEntry->getNextHops()->cref(0)->toThrift();
    EXPECT_EQ(*nexthop.address(), expectedNhop);
    EXPECT_EQ(tableEntry->getResolvedNextHops()->size(), 1);
    auto resolveNexthop =
        tableEntry->getResolvedNextHops()->cref(0)->toThrift();
    EXPECT_EQ(*resolveNexthop.address(), expectedNhop);
  };
  verifyEntry(*flowEntry.flow(), kNhopAddrA, "counter0", "fboss1");

  // modify the entry
  auto flowEntry2 = makeFlow("100::1", kNhopAddrB, "counter1", "fboss55");
  auto newFlowEntries = std::make_unique<std::vector<FlowEntry>>();
  newFlowEntries->emplace_back(flowEntry2);
  handler.addTeFlows(std::move(newFlowEntries));
  state = sw_->getState();
  teFlowTable = state->getTeFlowTable();
  verifyEntry(*flowEntry.flow(), kNhopAddrB, "counter1", "fboss55");

  auto teFlows = std::make_unique<std::vector<TeFlow>>();
  teFlows->emplace_back(*flowEntry.flow());
  handler.deleteTeFlows(std::move(teFlows));
  state = sw_->getState();
  teFlowTable = state->getTeFlowTable();
  auto tableEntry = teFlowTable->getTeFlowIf(*flowEntry.flow());
  EXPECT_EQ(tableEntry, nullptr);

  // add flows in bulk
  auto testPrefixes = {"100::1", "101::1", "102::1", "103::1"};
  auto bulkEntries = std::make_unique<std::vector<FlowEntry>>();
  for (const auto& prefix : testPrefixes) {
    bulkEntries->emplace_back(makeFlow(prefix));
  }
  handler.addTeFlows(std::move(bulkEntries));
  state = sw_->getState();
  teFlowTable = state->getTeFlowTable();
  EXPECT_EQ(teFlowTable->size(), 4);

  // bulk delete
  auto flowsToDelete = {"100::1", "101::1"};
  auto deletionFlows = std::make_unique<std::vector<TeFlow>>();
  for (const auto& prefix : flowsToDelete) {
    TeFlow flow;
    flow.dstPrefix() = ipPrefix(prefix, 64);
    flow.srcPort() = 100;
    deletionFlows->emplace_back(flow);
  }
  handler.deleteTeFlows(std::move(deletionFlows));
  state = sw_->getState();
  teFlowTable = state->getTeFlowTable();
  EXPECT_EQ(teFlowTable->size(), 2);
  for (const auto& prefix : flowsToDelete) {
    TeFlow flow;
    flow.dstPrefix() = ipPrefix(prefix, 64);
    flow.srcPort() = 100;
    EXPECT_EQ(teFlowTable->getTeFlowIf(flow), nullptr);
  }
}

TEST_F(ThriftTeFlowTest, syncTeFlows) {
  auto initalPrefixes = {"100::1", "101::1", "102::1", "103::1"};
  ThriftHandler handler(sw_);
  auto teFlowEntries = std::make_unique<std::vector<FlowEntry>>();
  for (const auto& prefix : initalPrefixes) {
    auto flowEntry = makeFlow(prefix);
    teFlowEntries->emplace_back(flowEntry);
  }
  handler.addTeFlows(std::move(teFlowEntries));
  auto state = sw_->getState();
  auto teFlowTable = state->getTeFlowTable();
  EXPECT_EQ(teFlowTable->size(), 4);

  // Ensure that all entries are created
  for (const auto& prefix : initalPrefixes) {
    TeFlow flow;
    flow.dstPrefix() = ipPrefix(prefix, 64);
    flow.srcPort() = 100;
    auto tableEntry = teFlowTable->getTeFlowIf(flow);
    EXPECT_NE(tableEntry, nullptr);
  }

  auto syncPrefixes = {"100::1", "101::1", "104::1"};
  auto syncFlowEntries = std::make_unique<std::vector<FlowEntry>>();
  for (const auto& prefix : syncPrefixes) {
    auto flowEntry = makeFlow(prefix);
    syncFlowEntries->emplace_back(flowEntry);
  }
  handler.syncTeFlows(std::move(syncFlowEntries));
  state = sw_->getState();
  teFlowTable = state->getTeFlowTable();
  EXPECT_EQ(teFlowTable->size(), 3);
  // Ensure that newly added entries are present
  for (const auto& prefix : syncPrefixes) {
    TeFlow flow;
    flow.dstPrefix() = ipPrefix(prefix, 64);
    flow.srcPort() = 100;
    auto tableEntry = teFlowTable->getTeFlowIf(flow);
    EXPECT_NE(tableEntry, nullptr);
  }
  // Ensure that missing entries are removed
  for (const auto& prefix : {"102::1", "103::1"}) {
    TeFlow flow;
    flow.dstPrefix() = ipPrefix(prefix, 64);
    flow.srcPort() = 100;
    auto tableEntry = teFlowTable->getTeFlowIf(flow);
    EXPECT_EQ(tableEntry, nullptr);
  }
  // sync flows with no entries
  auto nullFlowEntries = std::make_unique<std::vector<FlowEntry>>();
  handler.syncTeFlows(std::move(nullFlowEntries));
  state = sw_->getState();
  teFlowTable = state->getTeFlowTable();
  EXPECT_EQ(teFlowTable->size(), 0);
}

TEST_F(ThriftTeFlowTest, getTeFlowDetails) {
  auto testPrefixes = {"100::1", "101::1", "102::1", "103::1"};
  ThriftHandler handler(sw_);
  auto teFlowEntries = std::make_unique<std::vector<FlowEntry>>();
  for (const auto& prefix : testPrefixes) {
    auto flowEntry = makeFlow(prefix);
    teFlowEntries->emplace_back(flowEntry);
  }
  handler.addTeFlows(std::move(teFlowEntries));
  auto state = sw_->getState();
  auto teFlowTable = state->getTeFlowTable();
  EXPECT_EQ(teFlowTable->size(), 4);

  std::vector<TeFlowDetails> flowDetails;
  handler.getTeFlowTableDetails(flowDetails);
  EXPECT_EQ(flowDetails.size(), teFlowTable->size());

  auto idx = 0;
  for (const auto& prefix : testPrefixes) {
    TeFlow flow;
    flow.srcPort() = 100;
    flow.dstPrefix() = ipPrefix(prefix, 64);
    auto flowDetail = flowDetails[idx++];
    auto tableEntry = state->getTeFlowTable()->getTeFlowIf(flow);
    EXPECT_EQ(flowDetail.enabled(), tableEntry->getEnabled());
    EXPECT_EQ(flowDetail.counterID(), tableEntry->getCounterID()->toThrift());
    EXPECT_EQ(flowDetail.nexthops(), tableEntry->getNextHops()->toThrift());
    EXPECT_EQ(
        flowDetail.resolvedNexthops(),
        tableEntry->getResolvedNextHops()->toThrift());
  }
}

TEST_F(ThriftTeFlowTest, teFlowUpdateHwProtection) {
  ThriftHandler handler(sw_);
  auto teFlowEntries = std::make_unique<std::vector<FlowEntry>>();
  auto flowEntry = makeFlow("100::1");
  teFlowEntries->emplace_back(flowEntry);
  // Fail HW update by returning current state
  EXPECT_HW_CALL(sw_, stateChanged(_)).WillOnce(Return(sw_->getState()));

  EXPECT_THROW(
      {
        try {
          handler.addTeFlows(std::move(teFlowEntries));

        } catch (const FbossTeUpdateError& flowError) {
          EXPECT_EQ(flowError.failedAddUpdateFlows()->size(), 1);
          EXPECT_EQ(
              flowError.failedAddUpdateFlows()[0], makeFlow("100::1").flow());
          throw;
        }
      },
      FbossTeUpdateError);
}

TEST_F(ThriftTeFlowTest, teFlowSyncUpdateHwProtection) {
  ThriftHandler handler(sw_);
  auto teFlowEntries = std::make_unique<std::vector<FlowEntry>>();
  auto flowEntry = makeFlow("100::1");
  teFlowEntries->emplace_back(flowEntry);
  // Fail HW update by returning current state
  EXPECT_HW_CALL(sw_, stateChanged(_)).WillOnce(Return(sw_->getState()));

  EXPECT_THROW(
      {
        try {
          handler.syncTeFlows(std::move(teFlowEntries));

        } catch (const FbossTeUpdateError& flowError) {
          EXPECT_EQ(flowError.failedAddUpdateFlows()->size(), 1);
          EXPECT_EQ(
              flowError.failedAddUpdateFlows()[0], makeFlow("100::1").flow());
          throw;
        }
      },
      FbossTeUpdateError);
}
