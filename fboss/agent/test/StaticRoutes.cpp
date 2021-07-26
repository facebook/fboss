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
#include "fboss/agent/if/gen-cpp2/mpls_types.h"
#include "fboss/agent/state/Route.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/test/HwTestHandle.h"
#include "fboss/agent/test/TestUtils.h"

#include "fboss/agent/AddressUtil.h"

#include <folly/IPAddress.h>
#include <folly/IPAddressV4.h>
#include <folly/IPAddressV6.h>

#include <gtest/gtest.h>
#include <memory>

using namespace facebook::fboss;
using facebook::network::toBinaryAddress;
using folly::IPAddress;
using folly::IPAddressV4;
using folly::IPAddressV6;
using std::make_shared;

auto kStaticClient = ClientID::STATIC_ROUTE;

class StaticRouteTest : public ::testing::Test {
  cfg::SwitchConfig initialConfig() const {
    cfg::SwitchConfig config;
    config.vlans_ref()->resize(1);
    config.vlans_ref()[0].id_ref() = 1;
    config.vlans_ref()[0].name_ref() = "Vlan1";
    config.vlans_ref()[0].intfID_ref() = 1;
    config.interfaces_ref()->resize(1);
    config.interfaces_ref()[0].ipAddresses_ref()->resize(3);
    config.interfaces_ref()[0].ipAddresses_ref()[0] = "10.0.0.1/24";
    config.interfaces_ref()[0].ipAddresses_ref()[1] = "192.168.0.1/24";
    config.interfaces_ref()[0].ipAddresses_ref()[2] =
        "2401:db00:2110:3001::0001/64";
    config.interfaces_ref()[0].intfID_ref() = 1;
    config.interfaces_ref()[0].routerID_ref() = 0;
    config.interfaces_ref()[0].vlanID_ref() = 1;
    config.interfaces_ref()[0].name_ref() = "interface1";
    config.interfaces_ref()[0].mac_ref() = "00:02:00:00:00:01";
    config.interfaces_ref()[0].mtu_ref() = 9000;
    config.staticRoutesToNull_ref()->resize(2);
    config.staticRoutesToNull_ref()[0].prefix_ref() = "1.1.1.1/32";
    config.staticRoutesToNull_ref()[1].prefix_ref() = "2001::1/128";
    config.staticRoutesToCPU_ref()->resize(2);
    config.staticRoutesToCPU_ref()[0].prefix_ref() = "2.2.2.2/32";
    config.staticRoutesToCPU_ref()[1].prefix_ref() = "2001::2/128";
    config.staticRoutesWithNhops_ref()->resize(4);
    config.staticRoutesWithNhops_ref()[0].prefix_ref() = "3.3.3.3/32";
    config.staticRoutesWithNhops_ref()[0].nexthops_ref()->resize(1);
    config.staticRoutesWithNhops_ref()[0].nexthops_ref()[0] = "1.1.1.1";
    config.staticRoutesWithNhops_ref()[1].prefix_ref() = "4.4.4.4/32";
    config.staticRoutesWithNhops_ref()[1].nexthops_ref()->resize(1);
    config.staticRoutesWithNhops_ref()[1].nexthops_ref()[0] = "2.2.2.2";
    // Now add v6 recursive routes
    config.staticRoutesWithNhops_ref()[2].prefix_ref() = "2001::3/128";
    config.staticRoutesWithNhops_ref()[2].nexthops_ref()->resize(1);
    config.staticRoutesWithNhops_ref()[2].nexthops_ref()[0] = "2001::1";
    config.staticRoutesWithNhops_ref()[3].prefix_ref() = "2001::4/128";
    config.staticRoutesWithNhops_ref()[3].nexthops_ref()->resize(1);
    config.staticRoutesWithNhops_ref()[3].nexthops_ref()[0] = "2001::2";

    // Now add v6 route with stack
    config.staticIp2MplsRoutes_ref()->resize(1);
    config.staticIp2MplsRoutes_ref()[0].prefix_ref() = "2001::5/128";
    config.staticIp2MplsRoutes_ref()[0].nexthops_ref()->resize(1);
    config.staticIp2MplsRoutes_ref()[0].nexthops_ref()[0].address_ref() =
        toBinaryAddress(folly::IPAddress("2001::1"));
    MplsAction action;
    action.action_ref() = MplsActionCode::PUSH;
    action.pushLabels_ref() = {101, 102};
    config.staticIp2MplsRoutes_ref()[0].nexthops_ref()[0].mplsAction_ref() =
        action;
    return config;
  }

 public:
  void SetUp() override {
    auto config = initialConfig();
    handle_ = createTestHandle(&config);
    sw_ = handle_->getSw();
  }
  template <typename AddrT>
  std::shared_ptr<Route<AddrT>> findRoute(const RoutePrefix<AddrT>& nw) {
    return ::findRoute<AddrT>(
        RouterID(0), nw.toCidrNetwork(), this->sw_->getState());
  }

 protected:
  SwSwitch* sw_;
  std::unique_ptr<HwTestHandle> handle_;
};

TEST_F(StaticRouteTest, configureUnconfigure) {
  auto stateV1 = this->sw_->getState();
  ASSERT_NE(nullptr, stateV1);
  RouterID rid0(0);
  RouteV4::Prefix prefix1v4{IPAddressV4("1.1.1.1"), 32};
  RouteV4::Prefix prefix2v4{IPAddressV4("2.2.2.2"), 32};
  RouteV4::Prefix prefix3v4{IPAddressV4("3.3.3.3"), 32};
  RouteV4::Prefix prefix4v4{IPAddressV4("4.4.4.4"), 32};

  RouteV6::Prefix prefix1v6{IPAddressV6("2001::1"), 128};
  RouteV6::Prefix prefix2v6{IPAddressV6("2001::2"), 128};
  RouteV6::Prefix prefix3v6{IPAddressV6("2001::3"), 128};
  RouteV6::Prefix prefix4v6{IPAddressV6("2001::4"), 128};

  RouteV6::Prefix prefix1v6ToMpls{IPAddressV6("2001::5"), 128};

  auto r1v4 = this->findRoute(prefix1v4);
  ASSERT_NE(nullptr, r1v4);
  EXPECT_TRUE(r1v4->isResolved());
  EXPECT_FALSE(r1v4->isUnresolvable());
  EXPECT_FALSE(r1v4->isConnected());
  EXPECT_FALSE(r1v4->needResolve());
  EXPECT_EQ(
      r1v4->getForwardInfo(),
      RouteNextHopEntry(
          RouteForwardAction::DROP, AdminDistance::MAX_ADMIN_DISTANCE));
  auto r2v4 = this->findRoute(prefix2v4);
  ASSERT_NE(nullptr, r2v4);
  EXPECT_TRUE(r2v4->isResolved());
  EXPECT_FALSE(r2v4->isUnresolvable());
  EXPECT_FALSE(r2v4->isConnected());
  EXPECT_FALSE(r2v4->needResolve());
  EXPECT_EQ(
      r2v4->getForwardInfo(),
      RouteNextHopEntry(
          RouteForwardAction::TO_CPU, AdminDistance::MAX_ADMIN_DISTANCE));
  // Recursive resolution to RouteForwardAction::DROP
  auto r3v4 = this->findRoute(prefix3v4);
  ASSERT_NE(nullptr, r3v4);
  EXPECT_TRUE(r3v4->isResolved());
  EXPECT_FALSE(r3v4->isUnresolvable());
  EXPECT_FALSE(r3v4->isConnected());
  EXPECT_FALSE(r3v4->needResolve());
  EXPECT_EQ(
      r3v4->getForwardInfo(),
      RouteNextHopEntry(
          RouteForwardAction::DROP, AdminDistance::MAX_ADMIN_DISTANCE));

  // Recursive resolution to CPU
  auto r4v4 = this->findRoute(prefix4v4);
  ASSERT_NE(nullptr, r4v4);
  EXPECT_TRUE(r4v4->isResolved());
  EXPECT_FALSE(r4v4->isUnresolvable());
  EXPECT_FALSE(r4v4->isConnected());
  EXPECT_FALSE(r4v4->needResolve());
  EXPECT_EQ(
      r4v4->getForwardInfo(),
      RouteNextHopEntry(
          RouteForwardAction::TO_CPU, AdminDistance::MAX_ADMIN_DISTANCE));

  auto r1v6 = this->findRoute(prefix1v6);
  ASSERT_NE(nullptr, r1v6);
  EXPECT_TRUE(r1v6->isResolved());
  EXPECT_FALSE(r1v6->isUnresolvable());
  EXPECT_FALSE(r1v6->isConnected());
  EXPECT_FALSE(r1v6->needResolve());
  EXPECT_EQ(
      r1v6->getForwardInfo(),
      RouteNextHopEntry(
          RouteForwardAction::DROP, AdminDistance::MAX_ADMIN_DISTANCE));

  auto r2v6 = this->findRoute(prefix2v6);
  ASSERT_NE(nullptr, r2v6);
  EXPECT_TRUE(r2v6->isResolved());
  EXPECT_FALSE(r2v6->isUnresolvable());
  EXPECT_FALSE(r2v6->isConnected());
  EXPECT_FALSE(r2v6->needResolve());
  EXPECT_EQ(
      r2v6->getForwardInfo(),
      RouteNextHopEntry(
          RouteForwardAction::TO_CPU, AdminDistance::MAX_ADMIN_DISTANCE));

  // Recursive resolution to RouteForwardAction::DROP
  auto r3v6 = this->findRoute(prefix3v6);
  ASSERT_NE(nullptr, r3v6);
  EXPECT_TRUE(r3v6->isResolved());
  EXPECT_FALSE(r3v6->isUnresolvable());
  EXPECT_FALSE(r3v6->isConnected());
  EXPECT_FALSE(r3v6->needResolve());
  EXPECT_EQ(
      r3v6->getForwardInfo(),
      RouteNextHopEntry(
          RouteForwardAction::DROP, AdminDistance::MAX_ADMIN_DISTANCE));

  // Recursive resolution to CPU
  auto r4v6 = this->findRoute(prefix4v6);
  ASSERT_NE(nullptr, r4v6);
  EXPECT_TRUE(r4v6->isResolved());
  EXPECT_FALSE(r4v6->isUnresolvable());
  EXPECT_FALSE(r4v6->isConnected());
  EXPECT_FALSE(r4v6->needResolve());
  EXPECT_EQ(
      r4v6->getForwardInfo(),
      RouteNextHopEntry(
          RouteForwardAction::TO_CPU, AdminDistance::MAX_ADMIN_DISTANCE));
  // Recursive resolution to CPU

  auto r5v6ToMpls1 = this->findRoute(prefix1v6ToMpls);
  ASSERT_NE(nullptr, r5v6ToMpls1);
  EXPECT_TRUE(r5v6ToMpls1->isResolved());
  EXPECT_FALSE(r5v6ToMpls1->isUnresolvable());
  EXPECT_FALSE(r5v6ToMpls1->isConnected());
  EXPECT_FALSE(r5v6ToMpls1->needResolve());
  EXPECT_EQ(
      r4v6->getForwardInfo(),
      RouteNextHopEntry(
          RouteForwardAction::TO_CPU, AdminDistance::MAX_ADMIN_DISTANCE));

  // Now blow away the static routes from config.
  cfg::SwitchConfig emptyConfig;
  this->sw_->applyConfig("Empty config", emptyConfig);
  auto [v4Routes, v6Routes] = getRouteCount(this->sw_->getState());
  // Only null routes remain
  EXPECT_EQ(1, v4Routes);
  EXPECT_EQ(1, v6Routes);
}

TEST(StaticRoutes, MplsStaticRoutes) {
  auto platform = createMockPlatform();
  auto stateV0 = make_shared<SwitchState>();

  cfg::SwitchConfig config0;
  config0.vlans_ref()->resize(1);
  config0.vlans_ref()[0].id_ref() = 1;
  config0.interfaces_ref()->resize(1);
  auto* intfConfig = &config0.interfaces_ref()[0];
  intfConfig->name_ref() = "fboss1";
  intfConfig->intfID_ref() = 1;
  intfConfig->vlanID_ref() = 1;
  intfConfig->mac_ref() = "00:02:00:11:22:33";
  intfConfig->ipAddresses_ref()->resize(2);
  intfConfig->ipAddresses_ref()[0] = "10.0.0.0/24";
  intfConfig->ipAddresses_ref()[1] = "1::/64";

  // try to set link local nhop without interface
  config0.staticMplsRoutesWithNhops_ref()->resize(1);
  config0.staticMplsRoutesWithNhops_ref()[0].ingressLabel_ref() = 100;
  std::vector<NextHopThrift> nexthops;
  nexthops.resize(1);
  MplsAction swap;
  swap.action_ref() = MplsActionCode::SWAP;
  swap.swapLabel_ref() = 101;
  nexthops[0].mplsAction_ref() = swap;
  nexthops[0].address_ref() =
      toBinaryAddress(folly::IPAddress("fe80:abcd:1234:dcab::1"));
  config0.staticMplsRoutesWithNhops_ref()[0].nexthops_ref() = nexthops;
  RoutingInformationBase rib;
  EXPECT_THROW(
      publishAndApplyConfig(stateV0, &config0, platform.get(), &rib),
      FbossError);

  // try to set non-link local without interface and unreachable via interface
  nexthops[0].address_ref() = toBinaryAddress(folly::IPAddress("2::1"));
  config0.staticMplsRoutesWithNhops_ref()[0].nexthops_ref() = nexthops;
  EXPECT_THROW(
      publishAndApplyConfig(stateV0, &config0, platform.get(), &rib),
      FbossError);

  // setup link local with interface and non-link local without interface
  // reachable via interface
  nexthops.resize(2);
  nexthops[0].address_ref() =
      toBinaryAddress(folly::IPAddress("fe80:abcd:1234:dcab::1"));
  nexthops[0].address_ref()->ifName_ref() = *intfConfig->name_ref();

  swap.swapLabel_ref() = 102;
  nexthops[1].mplsAction_ref() = swap;
  nexthops[1].address_ref() = toBinaryAddress(folly::IPAddress("1::10"));
  config0.staticMplsRoutesWithNhops_ref()[0].nexthops_ref() = nexthops;
  auto stateV1 = publishAndApplyConfig(stateV0, &config0, platform.get(), &rib);

  // setup non-link local with interface, still valid
  nexthops.resize(3);
  swap.swapLabel_ref() = 103;
  nexthops[2].mplsAction_ref() = swap;
  nexthops[2].address_ref() = toBinaryAddress(folly::IPAddress("2::1"));
  nexthops[0].address_ref()->ifName_ref() = *intfConfig->name_ref();
  publishAndApplyConfig(stateV1, &config0, platform.get(), &rib);
}
