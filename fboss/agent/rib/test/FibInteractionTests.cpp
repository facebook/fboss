/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/rib/RouteNextHop.h"
#include "fboss/agent/rib/RouteNextHopEntry.h"
#include "fboss/agent/rib/RouteTypes.h"

#include "fboss/agent/rib/ForwardingInformationBaseUpdater.h"
#include "fboss/agent/rib/NetworkToRouteMap.h"
#include "fboss/agent/rib/Route.h"
#include "fboss/agent/state/ForwardingInformationBase.h"
#include "fboss/agent/state/ForwardingInformationBaseContainer.h"
#include "fboss/agent/state/ForwardingInformationBaseMap.h"
#include "fboss/agent/state/Route.h"
#include "fboss/agent/state/RouteNextHop.h"
#include "fboss/agent/state/RouteNextHopEntry.h"
#include "fboss/agent/state/RouteTypes.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/types.h"

#include "fboss/agent/if/gen-cpp2/ctrl_types.h"

#include <folly/IPAddress.h>
#include <folly/Optional.h>
#include <gtest/gtest.h>

using facebook::fboss::AdminDistance;
using facebook::fboss::InterfaceID;

namespace {

const AdminDistance kDefaultAdminDistance = AdminDistance::EBGP;

}

TEST(RouteNextHopEntry, ConvertRibDropToFibDrop) {
  facebook::fboss::rib::RouteNextHopEntry ribDrop(
      facebook::fboss::rib::RouteNextHopEntry::Action::DROP,
      kDefaultAdminDistance);

  facebook::fboss::RouteNextHopEntry fibDrop = ribDrop.toFibNextHop();

  ASSERT_TRUE(fibDrop.isDrop());
  ASSERT_EQ(fibDrop.getAdminDistance(), kDefaultAdminDistance);
}

TEST(RouteNextHopEntry, ConvertRibCpuToFibCpu) {
  facebook::fboss::rib::RouteNextHopEntry ribToCpu(
      facebook::fboss::rib::RouteNextHopEntry::Action::TO_CPU,
      kDefaultAdminDistance);

  facebook::fboss::RouteNextHopEntry fibToCpu = ribToCpu.toFibNextHop();

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
      ribResolvedNextHop.toFibNextHop();

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
      ribUnresolvedNextHop.toFibNextHop(), folly::OptionalEmptyException);
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

  auto fibRoute = ribRoute.toFibRoute();

  ASSERT_EQ(fibRoute->prefix().network, prefixAddress);
  ASSERT_EQ(fibRoute->prefix().mask, prefixMask);
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
