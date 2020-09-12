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
#include "fboss/agent/AddressUtil.h"
#include "fboss/agent/hw/mock/MockPlatform.h"
#include "fboss/agent/if/gen-cpp2/ctrl_types.h"
#include "fboss/agent/rib/ForwardingInformationBaseUpdater.h"
#include "fboss/agent/rib/NetworkToRouteMap.h"
#include "fboss/agent/rib/Route.h"
#include "fboss/agent/rib/RouteNextHop.h"
#include "fboss/agent/rib/RouteNextHopEntry.h"
#include "fboss/agent/rib/RouteTypes.h"
#include "fboss/agent/state/ForwardingInformationBase.h"
#include "fboss/agent/state/ForwardingInformationBaseContainer.h"
#include "fboss/agent/state/ForwardingInformationBaseMap.h"
#include "fboss/agent/state/Route.h"
#include "fboss/agent/state/RouteNextHop.h"
#include "fboss/agent/state/RouteNextHopEntry.h"
#include "fboss/agent/state/RouteTypes.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/test/HwTestHandle.h"
#include "fboss/agent/test/TestUtils.h"
#include "fboss/agent/types.h"

#include <folly/IPAddress.h>
#include <folly/functional/Partial.h>
#include <gtest/gtest.h>
#include <optional>

using facebook::fboss::AdminDistance;
using facebook::fboss::InterfaceID;

namespace {

const AdminDistance kDefaultAdminDistance = AdminDistance::EBGP;

void dynamicFibUpdate(
    facebook::fboss::RouterID vrf,
    const facebook::fboss::rib::IPv4NetworkToRouteMap& v4NetworkToRoute,
    const facebook::fboss::rib::IPv6NetworkToRouteMap& v6NetworkToRoute,
    void* cookie) {
  facebook::fboss::rib::ForwardingInformationBaseUpdater fibUpdater(
      vrf, v4NetworkToRoute, v6NetworkToRoute);

  auto sw = static_cast<facebook::fboss::SwSwitch*>(cookie);
  sw->updateStateBlocking("", std::move(fibUpdater));
}
} // namespace

TEST(RouteNextHopEntry, ConvertRibDropToFibDrop) {
  facebook::fboss::rib::RouteNextHopEntry ribDrop(
      facebook::fboss::rib::RouteNextHopEntry::Action::DROP,
      kDefaultAdminDistance);

  facebook::fboss::RouteNextHopEntry fibDrop =
      facebook::fboss::rib::ForwardingInformationBaseUpdater::toFibNextHop(
          ribDrop);

  ASSERT_TRUE(fibDrop.isDrop());
  ASSERT_EQ(fibDrop.getAdminDistance(), kDefaultAdminDistance);
}

TEST(RouteNextHopEntry, ConvertRibCpuToFibCpu) {
  facebook::fboss::rib::RouteNextHopEntry ribToCpu(
      facebook::fboss::rib::RouteNextHopEntry::Action::TO_CPU,
      kDefaultAdminDistance);

  facebook::fboss::RouteNextHopEntry fibToCpu =
      facebook::fboss::rib::ForwardingInformationBaseUpdater::toFibNextHop(
          ribToCpu);

  ASSERT_TRUE(fibToCpu.isToCPU());
  ASSERT_EQ(fibToCpu.getAdminDistance(), kDefaultAdminDistance);
}

TEST(RouteNextHopEntry, ConvertRibResolvedNextHopToFibResolvedNextHop) {
  auto address = folly::IPAddress("2401:db00:e112:9103:1028::1b");
  auto interfaceID = InterfaceID(1);

  facebook::fboss::rib::RouteNextHopEntry ribResolvedNextHop(
      facebook::fboss::rib::ResolvedNextHop(
          address, interfaceID, facebook::fboss::rib::ECMP_WEIGHT),
      kDefaultAdminDistance);

  facebook::fboss::RouteNextHopEntry fibResolvedNextHop =
      facebook::fboss::rib::ForwardingInformationBaseUpdater::toFibNextHop(
          ribResolvedNextHop);

  ASSERT_EQ(
      fibResolvedNextHop.getAction(),
      facebook::fboss::RouteNextHopEntry::Action::NEXTHOPS);
  ASSERT_EQ(fibResolvedNextHop.getNextHopSet().size(), 1);
  ASSERT_EQ(
      *(fibResolvedNextHop.getNextHopSet().nth(0)),
      facebook::fboss::ResolvedNextHop(
          address, interfaceID, facebook::fboss::ECMP_WEIGHT));
  ASSERT_EQ(fibResolvedNextHop.getAdminDistance(), kDefaultAdminDistance);
}

TEST(RouteNextHopEntry, AttemptToConvertRibUnresolvedNextHopToFibNextHop) {
  facebook::fboss::rib::RouteNextHopEntry ribUnresolvedNextHop(
      facebook::fboss::rib::UnresolvedNextHop(
          folly::IPAddress("2401:db00:e112:9103:1028::1b"),
          facebook::fboss::rib::ECMP_WEIGHT),
      kDefaultAdminDistance);

  EXPECT_THROW(
      facebook::fboss::rib::ForwardingInformationBaseUpdater::toFibNextHop(
          ribUnresolvedNextHop),
      std::bad_optional_access);
}

TEST(Route, RibRouteToFibRoute) {
  folly::IPAddressV6 prefixAddress("::0");
  uint8_t prefixMask = 0;

  facebook::fboss::rib::PrefixV6 prefix;
  prefix.network = prefixAddress;
  prefix.mask = prefixMask;

  folly::IPAddress nextHopAddress("2401:db00:e112:9103:1028::1b");
  InterfaceID nextHopInterfaceID(1);

  facebook::fboss::rib::RouteNextHopEntry ribResolvedNextHop(
      facebook::fboss::rib::ResolvedNextHop(
          nextHopAddress,
          nextHopInterfaceID,
          facebook::fboss::rib::ECMP_WEIGHT),
      kDefaultAdminDistance);

  facebook::fboss::rib::RouteV6 ribRoute(
      prefix, facebook::fboss::ClientID(1), ribResolvedNextHop);
  ribRoute.setResolved(ribResolvedNextHop);

  auto fibRoute =
      facebook::fboss::rib::ForwardingInformationBaseUpdater::toFibRoute(
          ribRoute);

  ASSERT_EQ(fibRoute->prefix().network, prefixAddress);
  ASSERT_EQ(fibRoute->prefix().mask, prefixMask);
}

TEST(Route, DirectlyConnectedRibRouteToFibRoute) {
  folly::IPAddressV6 prefixAddress("1::");
  uint8_t prefixMask = 127;

  facebook::fboss::rib::PrefixV6 prefix;
  prefix.network = prefixAddress;
  prefix.mask = prefixMask;

  folly::IPAddress nextHopAddress("2401:db00:e112:9103:1028::1b");
  InterfaceID nextHopInterfaceID(1);

  facebook::fboss::rib::RouteNextHopEntry ribResolvedNextHop(
      facebook::fboss::rib::ResolvedNextHop(
          nextHopAddress,
          nextHopInterfaceID,
          facebook::fboss::rib::ECMP_WEIGHT),
      kDefaultAdminDistance);

  facebook::fboss::rib::RouteV6 ribRoute(
      prefix, facebook::fboss::ClientID(1), ribResolvedNextHop);
  ribRoute.setResolved(ribResolvedNextHop);
  ribRoute.setConnected();

  auto fibRoute =
      facebook::fboss::rib::ForwardingInformationBaseUpdater::toFibRoute(
          ribRoute);

  ASSERT_EQ(fibRoute->prefix().network, prefixAddress);
  ASSERT_EQ(fibRoute->prefix().mask, prefixMask);
  ASSERT_TRUE(fibRoute->isConnected());
}

TEST(ForwardingInformationBaseUpdater, ModifyUnpublishedSwitchState) {
  using namespace facebook::fboss;

  auto vrfOne = RouterID(1);

  // First, we put together a Forwarding Information Base tree containing
  // no routes and hang it off an unpublished SwitchState.
  auto v4Fib = std::make_shared<facebook::fboss::ForwardingInformationBaseV4>();
  auto v6Fib = std::make_shared<facebook::fboss::ForwardingInformationBaseV6>();

  auto fibContainer =
      std::make_shared<facebook::fboss::ForwardingInformationBaseContainer>(
          vrfOne);
  fibContainer->writableFields()->fibV4 = v4Fib;
  fibContainer->writableFields()->fibV6 = v6Fib;

  auto fibMap =
      std::make_shared<facebook::fboss::ForwardingInformationBaseMap>();
  fibMap->addNode(fibContainer);

  auto initialState = std::make_shared<facebook::fboss::SwitchState>();
  initialState->resetForwardingInformationBases(fibMap);

  // Second, we pass the unpublished SwitchState through an update, which
  // transitively invokes  modify() on the nodes in the Forwarding Information
  // Base subtree
  facebook::fboss::rib::IPv4NetworkToRouteMap v4NetworkToRouteMap;
  facebook::fboss::rib::IPv6NetworkToRouteMap v6NetworkToRouteMap;
  facebook::fboss::rib::ForwardingInformationBaseUpdater updater(
      vrfOne, v4NetworkToRouteMap, v6NetworkToRouteMap);
  auto updatedState = updater(initialState);

  // Lastly, we check that the invocations of modify() operated on the
  // unpublished SwitchState in-place
  ASSERT_EQ(updatedState, initialState);
  ASSERT_FALSE(updatedState->isPublished());

  ASSERT_EQ(updatedState->getFibs(), initialState->getFibs());
  ASSERT_FALSE(updatedState->getFibs()->isPublished());

  ASSERT_EQ(
      updatedState->getFibs()->getFibContainerIf(vrfOne),
      initialState->getFibs()->getFibContainerIf(vrfOne));
  ASSERT_FALSE(
      updatedState->getFibs()->getFibContainerIf(vrfOne)->isPublished());

  ASSERT_EQ(
      updatedState->getFibs()->getFibContainerIf(vrfOne)->getFibV4(),
      initialState->getFibs()->getFibContainerIf(vrfOne)->getFibV4());
  ASSERT_FALSE(updatedState->getFibs()
                   ->getFibContainerIf(vrfOne)
                   ->getFibV4()
                   ->isPublished());

  ASSERT_EQ(
      updatedState->getFibs()->getFibContainerIf(vrfOne)->getFibV6(),
      initialState->getFibs()->getFibContainerIf(vrfOne)->getFibV6());
  ASSERT_FALSE(updatedState->getFibs()
                   ->getFibContainerIf(vrfOne)
                   ->getFibV6()
                   ->isPublished());
}

namespace {
template <typename AddressT>
std::shared_ptr<facebook::fboss::Route<AddressT>> getRoute(
    const std::shared_ptr<facebook::fboss::SwitchState>& state,
    facebook::fboss::RouterID vrf,
    AddressT address,
    uint8_t mask) {
  const auto& fibs = state->getFibs();
  const auto& fibContainer = fibs->getFibContainer(vrf);

  const std::shared_ptr<facebook::fboss::ForwardingInformationBase<AddressT>>&
      fib = fibContainer->template getFib<AddressT>();

  facebook::fboss::RoutePrefix<AddressT> prefix{address, mask};
  return fib->exactMatch(prefix);
}

template <typename AddressT>
void EXPECT_ROUTE(
    const std::shared_ptr<facebook::fboss::SwitchState>& state,
    facebook::fboss::RouterID vrf,
    AddressT address,
    uint8_t mask) {
  auto route = getRoute(state, vrf, address, mask);
  EXPECT_NE(nullptr, route);
}

template <typename AddressT>
void EXPECT_NO_ROUTE(
    const std::shared_ptr<facebook::fboss::SwitchState>& state,
    facebook::fboss::RouterID vrf,
    AddressT address,
    uint8_t mask) {
  auto route = getRoute(state, vrf, address, mask);
  EXPECT_EQ(nullptr, route);
}

facebook::fboss::UnicastRoute createUnicastRoute(
    folly::IPAddress address,
    uint8_t mask,
    folly::IPAddress nexthop) {
  facebook::fboss::UnicastRoute unicastRoute;

  facebook::fboss::IpPrefix network;
  network.ip_ref() = facebook::network::toBinaryAddress(address);
  network.prefixLength_ref() = mask;
  unicastRoute.dest_ref() = network;

  std::vector<facebook::fboss::NextHopThrift> nexthops(1);
  nexthops.back().address_ref() = facebook::network::toBinaryAddress(nexthop);
  nexthops.back().weight_ref() = facebook::fboss::ECMP_WEIGHT;
  unicastRoute.nextHops_ref() = std::move(nexthops);

  return unicastRoute;
}

void EXPECT_FIB_SIZE(
    const std::shared_ptr<facebook::fboss::SwitchState>& state,
    facebook::fboss::RouterID vrf,
    std::size_t v4FibSize,
    std::size_t v6FibSize) {
  const auto& fibs = state->getFibs();
  const auto& fibContainer = fibs->getFibContainer(vrf);

  EXPECT_EQ(fibContainer->getFibV4()->size(), v4FibSize);
  EXPECT_EQ(fibContainer->getFibV6()->size(), v6FibSize);
}

} // namespace

TEST(Rib, Update) {
  using namespace facebook::fboss;

  const RouterID vrfZero{0};
  auto v4Default = folly::CIDRNetworkV4(folly::IPAddressV4("0.0.0.0"), 0);
  auto v6Default = folly::CIDRNetworkV6(folly::IPAddressV6("::"), 0);

  cfg::SwitchConfig config;
  config.vlans_ref()->resize(1);
  *config.vlans_ref()[0].id_ref() = 1;
  config.interfaces_ref()->resize(1);
  *config.interfaces_ref()[0].intfID_ref() = 1;
  *config.interfaces_ref()[0].vlanID_ref() = 1;
  *config.interfaces_ref()[0].routerID_ref() = 0;
  config.interfaces_ref()[0].mac_ref() = "00:02:00:00:00:01";
  config.interfaces_ref()[0].ipAddresses_ref()->resize(3);
  config.interfaces_ref()[0].ipAddresses_ref()[0] = "0.0.0.0/0";
  config.interfaces_ref()[0].ipAddresses_ref()[1] = "192.168.0.19/24";
  config.interfaces_ref()[0].ipAddresses_ref()[2] = "::/0";

  auto testHandle =
      createTestHandle(&config, SwitchFlags::ENABLE_STANDALONE_RIB);
  auto sw = testHandle->getSw();

  EXPECT_FIB_SIZE(sw->getState(), vrfZero, 2, 2);

  auto client10Nexthop4 = folly::IPAddressV4("11.11.11.11");
  auto client10Nexthop6 = folly::IPAddressV6("11:11::0");
  auto client20Nexthop4 = folly::IPAddressV4("22.22.22.22");
  auto client20Nexthop6 = folly::IPAddressV6("22:22::0");
  auto client30Nexthop6 = folly::IPAddressV6("33:33::0");
  auto client10Nexthop6b = folly::IPAddressV6("44:44::0");

  // Prefix A will include next-hops from client 10 only
  auto prefixA4 = folly::CIDRNetworkV4(folly::IPAddressV4("7.1.0.0"), 16);
  auto prefixA6 = folly::CIDRNetworkV6(folly::IPAddressV6("aaaa:1::0"), 64);

  std::vector<UnicastRoute> routesToPrefixA;
  routesToPrefixA.push_back(
      createUnicastRoute(prefixA4.first, prefixA4.second, client10Nexthop4));
  routesToPrefixA.push_back(
      createUnicastRoute(prefixA6.first, prefixA6.second, client10Nexthop6));

  // Get number of routes in rib
  const size_t numStaticRoutes =
      sw->getRib()->getRouteTableDetails(vrfZero).size();

  sw->getRib()->update(
      vrfZero,
      ClientID(10),
      AdminDistance::EBGP,
      routesToPrefixA,
      {},
      false /* sync */,
      "rib update unit test",
      &dynamicFibUpdate,
      static_cast<void*>(sw));

  EXPECT_FIB_SIZE(sw->getState(), vrfZero, 3, 3);

  // Verify that new routes are reflected in route table details
  EXPECT_EQ(
      numStaticRoutes + 2, sw->getRib()->getRouteTableDetails(vrfZero).size());

  // Prefix B will include next-hops from clients 10 and 20
  auto prefixB4 = folly::CIDRNetworkV4(folly::IPAddressV4("7.2.0.0"), 16);

  std::vector<UnicastRoute> routeToPrefixBFromClient10;
  routeToPrefixBFromClient10.push_back(
      createUnicastRoute(prefixB4.first, prefixB4.second, client10Nexthop4));

  sw->getRib()->update(
      vrfZero,
      ClientID(10),
      AdminDistance::EBGP,
      routeToPrefixBFromClient10,
      {},
      false /* sync */,
      "rib update unit test",
      &dynamicFibUpdate,
      static_cast<void*>(sw));

  std::vector<UnicastRoute> routeToPrefixBFromClient20;
  routeToPrefixBFromClient20.push_back(
      createUnicastRoute(prefixB4.first, prefixB4.second, client20Nexthop4));
  sw->getRib()->update(
      vrfZero,
      ClientID(20),
      AdminDistance::EBGP,
      routeToPrefixBFromClient20,
      {},
      false /* sync */,
      "rib update unit test",
      &dynamicFibUpdate,
      static_cast<void*>(sw));

  EXPECT_FIB_SIZE(sw->getState(), vrfZero, 4, 3);

  // Prefix C will include nexthops from clients 10, 20, and 30
  auto prefixC6 = folly::CIDRNetworkV6(folly::IPAddressV6("aaaa:3::0"), 64);

  std::vector<UnicastRoute> routeToPrefixCFromClient10;
  routeToPrefixCFromClient10.push_back(
      createUnicastRoute(prefixC6.first, prefixC6.second, client10Nexthop6));
  sw->getRib()->update(
      vrfZero,
      ClientID(10),
      AdminDistance::EBGP,
      routeToPrefixCFromClient10,
      {},
      false /* sync */,
      "rib update unit test",
      &dynamicFibUpdate,
      static_cast<void*>(sw));

  std::vector<UnicastRoute> routeToPrefixCFromClient20;
  routeToPrefixCFromClient20.push_back(
      createUnicastRoute(prefixC6.first, prefixC6.second, client20Nexthop6));
  sw->getRib()->update(
      vrfZero,
      ClientID(20),
      AdminDistance::EBGP,
      routeToPrefixCFromClient20,
      {},
      false /* sync */,
      "rib update unit test",
      &dynamicFibUpdate,
      static_cast<void*>(sw));

  std::vector<UnicastRoute> routeToPrefixCFromClient30;
  routeToPrefixCFromClient30.push_back(
      createUnicastRoute(prefixC6.first, prefixC6.second, client30Nexthop6));
  sw->getRib()->update(
      vrfZero,
      ClientID(30),
      AdminDistance::EBGP,
      routeToPrefixCFromClient30,
      {},
      false /* sync */,
      "rib update unit test",
      &dynamicFibUpdate,
      static_cast<void*>(sw));

  // Make sure all the static and link-local routes are there
  auto state = sw->getState();
  EXPECT_ROUTE(state, vrfZero, v4Default.first, v4Default.second);
  EXPECT_ROUTE(state, vrfZero, folly::IPAddressV4("192.168.0.0"), 24);
  EXPECT_ROUTE(state, vrfZero, v6Default.first, v6Default.second);
  EXPECT_ROUTE(state, vrfZero, folly::IPAddressV6("fe80::"), 64);
  // Make sure the client 10&20&30 routes are there.
  EXPECT_ROUTE(state, vrfZero, prefixA4.first, prefixA4.second);
  EXPECT_ROUTE(state, vrfZero, prefixA6.first, prefixA6.second);
  EXPECT_ROUTE(state, vrfZero, prefixB4.first, prefixB4.second);
  EXPECT_ROUTE(state, vrfZero, prefixC6.first, prefixC6.second);

  EXPECT_FIB_SIZE(sw->getState(), vrfZero, 4, 4);

  //
  // Now use syncFib to remove all the routes for client 10 and add
  // some new ones
  // Statics, link-locals, and clients 20 and 30 should remain unchanged.
  //
  auto prefixD4 = folly::CIDRNetworkV4(folly::IPAddressV4("7.4.0.0"), 16);
  auto prefixD6 = folly::CIDRNetworkV6(folly::IPAddressV6("aaaa:4::0"), 64);

  std::vector<UnicastRoute> syncFibRoutes;
  syncFibRoutes.push_back(
      createUnicastRoute(prefixC6.first, prefixC6.second, client10Nexthop6b));
  syncFibRoutes.push_back(
      createUnicastRoute(prefixD6.first, prefixD6.second, client10Nexthop6b));
  syncFibRoutes.push_back(
      createUnicastRoute(prefixD4.first, prefixD4.second, client10Nexthop4));
  sw->getRib()->update(
      vrfZero,
      ClientID(10),
      AdminDistance::EBGP,
      syncFibRoutes,
      {},
      true /* sync */,
      "rib update unit test",
      &dynamicFibUpdate,
      static_cast<void*>(sw));

  // Make sure all the static and link-local routes are still there
  state = sw->getState();
  EXPECT_ROUTE(state, vrfZero, v4Default.first, v4Default.second);
  EXPECT_ROUTE(state, vrfZero, folly::IPAddressV4("192.168.0.0"), 24);
  EXPECT_ROUTE(state, vrfZero, v6Default.first, v6Default.second);
  EXPECT_ROUTE(state, vrfZero, folly::IPAddressV6("fe80::"), 64);

  // The prefixA* routes should have disappeared
  EXPECT_NO_ROUTE(state, vrfZero, prefixA4.first, prefixA4.second);
  EXPECT_NO_ROUTE(state, vrfZero, prefixA6.first, prefixA6.second);

  EXPECT_ROUTE(state, vrfZero, prefixB4.first, prefixB4.second);

  // The prefixD4 and prefixD6 routes should have been created
  EXPECT_ROUTE(state, vrfZero, prefixD4.first, prefixD4.second);
  EXPECT_ROUTE(state, vrfZero, prefixD6.first, prefixD6.second);

  // Make sure there are no more routes (ie. the old ones were deleted) + the
  // one that gets added by default
  EXPECT_FIB_SIZE(state, vrfZero, 4, 4);
}

// There are 3 cases that should be exercised:
// 1) a route has been added whose prefix _doesn't_ exist in the RIB
// 2) a route has been added whose prefix exists in the RIB BUT whose
//    forwarding information differs from that in the FIB
// 3) a route has been added whose prefix exists in the RIB AND whose
//    forwarding information matches that in the FIB
TEST(ForwardingInformationBaseUpdater, Deduplication) {
  using namespace facebook::fboss;

  const RouterID vrfZero{0};
  RoutePrefixV6 prefix{folly::IPAddressV6("2a03:2880:ff:1e::"), 64};

  cfg::SwitchConfig config;
  config.vlans_ref()->resize(1);
  *config.vlans_ref()[0].id_ref() = 1;
  config.interfaces_ref()->resize(1);
  *config.interfaces_ref()[0].intfID_ref() = 1;
  *config.interfaces_ref()[0].vlanID_ref() = 1;
  *config.interfaces_ref()[0].routerID_ref() = vrfZero;
  config.interfaces_ref()[0].mac_ref() = "00:00:00:00:00:11";
  config.interfaces_ref()[0].ipAddresses_ref()->resize(2);
  config.interfaces_ref()[0].ipAddresses_ref()[0] = "10.120.70.44/31";
  config.interfaces_ref()[0].ipAddresses_ref()[1] =
      "2401:db00:e003:9100:1006::2c/127";

  auto testHandle =
      createTestHandle(&config, SwitchFlags::ENABLE_STANDALONE_RIB);
  auto sw = testHandle->getSw();

  std::vector<UnicastRoute> routesToAdd;
  routesToAdd.push_back(createUnicastRoute(
      prefix.network,
      prefix.mask,
      folly::IPAddress("2401:db00:e003:9100:1006::2c")));
  std::vector<IpPrefix> routesToDelete;

  // 1)
  sw->getRib()->update(
      vrfZero,
      ClientID(0),
      AdminDistance::EBGP,
      routesToAdd,
      routesToDelete,
      false /* sync */,
      "deduplication unit test",
      &dynamicFibUpdate,
      static_cast<void*>(sw));

  auto route = sw->getState()
                   ->getFibs()
                   ->getFibContainer(vrfZero)
                   ->getFibV6()
                   ->exactMatch(prefix);
  ASSERT_TRUE(route);

  // 3)
  sw->getRib()->update(
      vrfZero,
      ClientID(0),
      AdminDistance::EBGP,
      routesToAdd,
      routesToDelete,
      false /* sync */,
      "deduplication unit test",
      &dynamicFibUpdate,
      static_cast<void*>(sw));

  auto route2 = sw->getState()
                    ->getFibs()
                    ->getFibContainer(vrfZero)
                    ->getFibV6()
                    ->exactMatch(prefix);
  ASSERT_TRUE(route2);
  EXPECT_EQ(route, route2);

  routesToAdd.push_back(createUnicastRoute(
      prefix.network,
      prefix.mask,
      folly::IPAddress("2401:db00:e003:9100:1006::2b")));

  // 2)
  sw->getRib()->update(
      vrfZero,
      ClientID(0),
      AdminDistance::EBGP,
      routesToAdd,
      routesToDelete,
      false /* sync */,
      "deduplication unit test",
      &dynamicFibUpdate,
      static_cast<void*>(sw));

  auto route3 = sw->getState()
                    ->getFibs()
                    ->getFibContainer(vrfZero)
                    ->getFibV6()
                    ->exactMatch(prefix);
  ASSERT_TRUE(route3);
  EXPECT_NE(route, route3);
}
