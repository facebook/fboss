/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/FbossError.h"
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/hw/mock/MockPlatform.h"
#include "fboss/agent/state/Route.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/test/TestUtils.h"

#include <folly/IPAddress.h>
#include <folly/IPAddressV4.h>
#include <folly/IPAddressV6.h>

#include <gtest/gtest.h>
#include <memory>

using namespace facebook::fboss;
using folly::IPAddress;
using folly::IPAddressV4;
using folly::IPAddressV6;
using std::make_shared;

auto kStaticClient = ClientID::STATIC_ROUTE;

TEST(StaticRoutes, configureUnconfigure) {
  auto platform = createMockPlatform();
  auto stateV0 = make_shared<SwitchState>();
  auto tablesV0 = stateV0->getRouteTables();

  cfg::SwitchConfig config;
  config.staticRoutesToNull_ref()->resize(2);
  *config.staticRoutesToNull_ref()[0].prefix_ref() = "1.1.1.1/32";
  *config.staticRoutesToNull_ref()[1].prefix_ref() = "2001::1/128";
  config.staticRoutesToCPU_ref()->resize(2);
  *config.staticRoutesToCPU_ref()[0].prefix_ref() = "2.2.2.2/32";
  *config.staticRoutesToCPU_ref()[1].prefix_ref() = "2001::2/128";
  config.staticRoutesWithNhops_ref()->resize(4);
  *config.staticRoutesWithNhops_ref()[0].prefix_ref() = "3.3.3.3/32";
  config.staticRoutesWithNhops_ref()[0].nexthops_ref()->resize(1);
  config.staticRoutesWithNhops_ref()[0].nexthops_ref()[0] = "1.1.1.1";
  *config.staticRoutesWithNhops_ref()[1].prefix_ref() = "4.4.4.4/32";
  config.staticRoutesWithNhops_ref()[1].nexthops_ref()->resize(1);
  config.staticRoutesWithNhops_ref()[1].nexthops_ref()[0] = "2.2.2.2";
  // Now add v6 recursive routes
  *config.staticRoutesWithNhops_ref()[2].prefix_ref() = "2001::3/128";
  config.staticRoutesWithNhops_ref()[2].nexthops_ref()->resize(1);
  config.staticRoutesWithNhops_ref()[2].nexthops_ref()[0] = "2001::1";
  *config.staticRoutesWithNhops_ref()[3].prefix_ref() = "2001::4/128";
  config.staticRoutesWithNhops_ref()[3].nexthops_ref()->resize(1);
  config.staticRoutesWithNhops_ref()[3].nexthops_ref()[0] = "2001::2";

  auto stateV1 = publishAndApplyConfig(stateV0, &config, platform.get());
  ASSERT_NE(nullptr, stateV1);
  RouterID rid0(0);
  auto t1 = stateV1->getRouteTables()->getRouteTableIf(rid0);
  RouteV4::Prefix prefix1v4{IPAddressV4("1.1.1.1"), 32};
  RouteV4::Prefix prefix2v4{IPAddressV4("2.2.2.2"), 32};
  RouteV4::Prefix prefix3v4{IPAddressV4("3.3.3.3"), 32};
  RouteV4::Prefix prefix4v4{IPAddressV4("4.4.4.4"), 32};

  RouteV6::Prefix prefix1v6{IPAddressV6("2001::1"), 128};
  RouteV6::Prefix prefix2v6{IPAddressV6("2001::2"), 128};
  RouteV6::Prefix prefix3v6{IPAddressV6("2001::3"), 128};
  RouteV6::Prefix prefix4v6{IPAddressV6("2001::4"), 128};

  auto rib1v4 = t1->getRibV4();
  auto r1v4 = rib1v4->exactMatch(prefix1v4);
  ASSERT_NE(nullptr, r1v4);
  EXPECT_TRUE(r1v4->isResolved());
  EXPECT_FALSE(r1v4->isUnresolvable());
  EXPECT_FALSE(r1v4->isConnected());
  EXPECT_FALSE(r1v4->needResolve());
  EXPECT_EQ(
      r1v4->getForwardInfo(),
      RouteNextHopEntry(DROP, AdminDistance::MAX_ADMIN_DISTANCE));

  auto r2v4 = rib1v4->exactMatch(prefix2v4);
  ASSERT_NE(nullptr, r2v4);
  EXPECT_TRUE(r2v4->isResolved());
  EXPECT_FALSE(r2v4->isUnresolvable());
  EXPECT_FALSE(r2v4->isConnected());
  EXPECT_FALSE(r2v4->needResolve());
  EXPECT_EQ(
      r2v4->getForwardInfo(),
      RouteNextHopEntry(TO_CPU, AdminDistance::MAX_ADMIN_DISTANCE));

  // Recursive resolution to DROP
  auto r3v4 = rib1v4->exactMatch(prefix3v4);
  ASSERT_NE(nullptr, r3v4);
  EXPECT_TRUE(r3v4->isResolved());
  EXPECT_FALSE(r3v4->isUnresolvable());
  EXPECT_FALSE(r3v4->isConnected());
  EXPECT_FALSE(r3v4->needResolve());
  EXPECT_EQ(
      r3v4->getForwardInfo(),
      RouteNextHopEntry(DROP, AdminDistance::MAX_ADMIN_DISTANCE));

  // Recursive resolution to CPU
  auto r4v4 = rib1v4->exactMatch(prefix4v4);
  ASSERT_NE(nullptr, r4v4);
  EXPECT_TRUE(r4v4->isResolved());
  EXPECT_FALSE(r4v4->isUnresolvable());
  EXPECT_FALSE(r4v4->isConnected());
  EXPECT_FALSE(r4v4->needResolve());
  EXPECT_EQ(
      r4v4->getForwardInfo(),
      RouteNextHopEntry(TO_CPU, AdminDistance::MAX_ADMIN_DISTANCE));

  auto rib1v6 = t1->getRibV6();
  auto r1v6 = rib1v6->exactMatch(prefix1v6);
  ASSERT_NE(nullptr, r1v6);
  EXPECT_TRUE(r1v6->isResolved());
  EXPECT_FALSE(r1v6->isUnresolvable());
  EXPECT_FALSE(r1v6->isConnected());
  EXPECT_FALSE(r1v6->needResolve());
  EXPECT_EQ(
      r1v6->getForwardInfo(),
      RouteNextHopEntry(DROP, AdminDistance::MAX_ADMIN_DISTANCE));

  auto r2v6 = rib1v6->exactMatch(prefix2v6);
  ASSERT_NE(nullptr, r2v6);
  EXPECT_TRUE(r2v6->isResolved());
  EXPECT_FALSE(r2v6->isUnresolvable());
  EXPECT_FALSE(r2v6->isConnected());
  EXPECT_FALSE(r2v6->needResolve());
  EXPECT_EQ(
      r2v6->getForwardInfo(),
      RouteNextHopEntry(TO_CPU, AdminDistance::MAX_ADMIN_DISTANCE));

  // Recursive resolution to DROP
  auto r3v6 = rib1v6->exactMatch(prefix3v6);
  ASSERT_NE(nullptr, r3v6);
  EXPECT_TRUE(r3v6->isResolved());
  EXPECT_FALSE(r3v6->isUnresolvable());
  EXPECT_FALSE(r3v6->isConnected());
  EXPECT_FALSE(r3v6->needResolve());
  EXPECT_EQ(
      r3v6->getForwardInfo(),
      RouteNextHopEntry(DROP, AdminDistance::MAX_ADMIN_DISTANCE));

  // Recursive resolution to CPU
  auto r4v6 = rib1v6->exactMatch(prefix4v6);
  ASSERT_NE(nullptr, r4v6);
  EXPECT_TRUE(r4v6->isResolved());
  EXPECT_FALSE(r4v6->isUnresolvable());
  EXPECT_FALSE(r4v6->isConnected());
  EXPECT_FALSE(r4v6->needResolve());
  EXPECT_EQ(
      r4v6->getForwardInfo(),
      RouteNextHopEntry(TO_CPU, AdminDistance::MAX_ADMIN_DISTANCE));

  // Now blow away the static routes from config.
  cfg::SwitchConfig emptyConfig;
  auto stateV2 =
      publishAndApplyConfig(stateV1, &emptyConfig, platform.get(), nullptr);
  ASSERT_NE(nullptr, stateV2);
  auto t2 = stateV2->getRouteTables()->getRouteTableIf(rid0);
  // No routes and hence no routing table
  ASSERT_EQ(nullptr, t2);
}
