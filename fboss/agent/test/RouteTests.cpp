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
#include "fboss/agent/FibHelpers.h"
#include "fboss/agent/SwSwitchRouteUpdateWrapper.h"
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/test/HwTestHandle.h"
#include "fboss/agent/test/TestUtils.h"

#include <folly/logging/xlog.h>
#include <gtest/gtest.h>
#include <optional>

using namespace facebook::fboss;
using folly::IPAddress;
using folly::IPAddressV4;
using folly::IPAddressV6;
using std::make_shared;
using std::shared_ptr;
using ::testing::Return;

namespace {
const auto kDestPrefix =
    RouteV6::Prefix{folly::IPAddressV6("2401:bad:cad:dad::"), 64};
const auto kDestAddress = folly::IPAddressV6("2401:bad:cad:dad::beef");

// bgp next hops
const std::array<folly::IPAddressV6, 4> kBgpNextHopAddrs{
    folly::IPAddressV6("2801::1"),
    folly::IPAddressV6("2802::1"),
    folly::IPAddressV6("2803::1"),
    folly::IPAddressV6("2804::1"),
};

// igp next hops
const std::array<folly::IPAddressV6, 4> kIgpAddrs{
    folly::IPAddressV6("fe80::101"),
    folly::IPAddressV6("fe80::102"),
    folly::IPAddressV6("fe80::103"),
    folly::IPAddressV6("fe80::104"),
};

// label stacks
const std::array<LabelForwardingAction::LabelStack, 4> kLabelStacks{
    LabelForwardingAction::LabelStack({101, 201, 301}),
    LabelForwardingAction::LabelStack({102, 202, 302}),
    LabelForwardingAction::LabelStack({103, 203, 303}),
    LabelForwardingAction::LabelStack({104, 204, 304}),
};
const std::array<InterfaceID, 4> kInterfaces{
    InterfaceID(1),
    InterfaceID(2),
    InterfaceID(3),
    InterfaceID(4),
};
const ClientID kClientA = ClientID(1001);

constexpr AdminDistance DISTANCE = AdminDistance::MAX_ADMIN_DISTANCE;
//
// Helper functions
//
template <typename AddrT>
void EXPECT_FWD_INFO(
    std::shared_ptr<Route<AddrT>> rt,
    InterfaceID intf,
    std::string ipStr) {
  const auto& fwds = rt->getForwardInfo().getNextHopSet();
  EXPECT_EQ(1, fwds.size());
  const auto& fwd = *fwds.begin();
  EXPECT_EQ(intf, fwd.intf());
  EXPECT_EQ(IPAddress(ipStr), fwd.addr());
}

template <typename AddrT>
void EXPECT_RESOLVED(std::shared_ptr<Route<AddrT>> rt) {
  ASSERT_NE(nullptr, rt);
  EXPECT_TRUE(rt->isResolved());
  EXPECT_FALSE(rt->isUnresolvable());
  EXPECT_FALSE(rt->needResolve());
}

template <typename AddrT>
void EXPECT_NODEMAP_MATCH_LEGACY_RIB(
    const std::shared_ptr<RouteTableRib<AddrT>>& rib) {
  const auto& radixTree = rib->routesRadixTree();
  EXPECT_EQ(rib->size(), radixTree.size());
  for (const auto& route : *(rib->routes())) {
    auto match =
        radixTree.exactMatch(route->prefix().network, route->prefix().mask);
    ASSERT_NE(match, radixTree.end());
    // should be the same shared_ptr
    EXPECT_EQ(route, match->value());
  }
}

void EXPECT_NODEMAP_MATCH_LEGACY_RIB(
    const std::shared_ptr<RouteTableMap>& routeTables) {
  for (const auto& rt : *routeTables) {
    if (rt->getRibV4()) {
      EXPECT_NODEMAP_MATCH_LEGACY_RIB<IPAddressV4>(rt->getRibV4());
    }
    if (rt->getRibV6()) {
      EXPECT_NODEMAP_MATCH_LEGACY_RIB<IPAddressV6>(rt->getRibV6());
    }
  }
}

void EXPECT_NODEMAP_MATCH(const SwSwitch* sw) {
  if (sw->isStandaloneRibEnabled()) {
    // TODO - compare RIB And FIB?
    return;
  } else {
    EXPECT_NODEMAP_MATCH_LEGACY_RIB(sw->getState()->getRouteTables());
  }
}
} // namespace

template <typename StandAloneRib>
class RouteTest : public ::testing::Test {
 public:
  void SetUp() override {
    cfg::SwitchConfig config;
    config.vlans_ref()->resize(4);
    config.vlans_ref()[0].id_ref() = 1;
    config.vlans_ref()[1].id_ref() = 2;
    config.vlans_ref()[2].id_ref() = 3;
    config.vlans_ref()[3].id_ref() = 4;

    config.interfaces_ref()->resize(4);
    config.interfaces_ref()[0].intfID_ref() = 1;
    config.interfaces_ref()[0].vlanID_ref() = 1;
    config.interfaces_ref()[0].routerID_ref() = 0;
    config.interfaces_ref()[0].mac_ref() = "00:00:00:00:00:11";
    config.interfaces_ref()[0].ipAddresses_ref()->resize(2);
    config.interfaces_ref()[0].ipAddresses_ref()[0] = "1.1.1.1/24";
    config.interfaces_ref()[0].ipAddresses_ref()[1] = "1::1/48";

    config.interfaces_ref()[1].intfID_ref() = 2;
    config.interfaces_ref()[1].vlanID_ref() = 2;
    config.interfaces_ref()[1].routerID_ref() = 0;
    config.interfaces_ref()[1].mac_ref() = "00:00:00:00:00:22";
    config.interfaces_ref()[1].ipAddresses_ref()->resize(2);
    config.interfaces_ref()[1].ipAddresses_ref()[0] = "2.2.2.2/24";
    config.interfaces_ref()[1].ipAddresses_ref()[1] = "2::1/48";

    config.interfaces_ref()[2].intfID_ref() = 3;
    config.interfaces_ref()[2].vlanID_ref() = 3;
    config.interfaces_ref()[2].routerID_ref() = 0;
    config.interfaces_ref()[2].mac_ref() = "00:00:00:00:00:33";
    config.interfaces_ref()[2].ipAddresses_ref()->resize(2);
    config.interfaces_ref()[2].ipAddresses_ref()[0] = "3.3.3.3/24";
    config.interfaces_ref()[2].ipAddresses_ref()[1] = "3::1/48";

    config.interfaces_ref()[3].intfID_ref() = 4;
    config.interfaces_ref()[3].vlanID_ref() = 4;
    config.interfaces_ref()[3].routerID_ref() = 0;
    config.interfaces_ref()[3].mac_ref() = "00:00:00:00:00:44";
    config.interfaces_ref()[3].ipAddresses_ref()->resize(2);
    config.interfaces_ref()[3].ipAddresses_ref()[0] = "4.4.4.4/24";
    config.interfaces_ref()[3].ipAddresses_ref()[1] = "4::1/48";
    auto flags = StandAloneRib::hasStandAloneRib
        ? SwitchFlags::ENABLE_STANDALONE_RIB
        : SwitchFlags::DEFAULT;
    handle_ = createTestHandle(&config, flags);
    sw_ = handle_->getSw();
  }
  std::shared_ptr<Route<IPAddressV4>> findRoute4(
      const std::shared_ptr<SwitchState>& state,
      RouterID rid,
      const RoutePrefixV4& prefix) {
    return findRouteImpl<IPAddressV4>(
        rid, {prefix.network, prefix.mask}, state);
  }
  std::shared_ptr<Route<IPAddressV6>> findRoute6(
      const std::shared_ptr<SwitchState>& state,
      RouterID rid,
      const RoutePrefixV6& prefix) {
    return findRouteImpl<IPAddressV6>(
        rid, {prefix.network, prefix.mask}, state);
  }

 private:
  template <typename AddrT>
  std::shared_ptr<Route<AddrT>> findRouteImpl(
      RouterID rid,
      const folly::CIDRNetwork& prefix,
      const std::shared_ptr<SwitchState>& state) {
    return findRoute<AddrT>(sw_->isStandaloneRibEnabled(), rid, prefix, state);
  }

 protected:
  SwSwitch* sw_;
  std::unique_ptr<HwTestHandle> handle_;
};

using RouteTestTypes = ::testing::Types<NoRib, Rib>;

TYPED_TEST_CASE(RouteTest, RouteTestTypes);

TYPED_TEST(RouteTest, dedup) {
  EXPECT_NODEMAP_MATCH(this->sw_);

  auto stateV1 = this->sw_->getState();
  auto rid = RouterID(0);
  // 2 different nexthops
  RouteNextHopSet nhop1 = makeNextHops({"1.1.1.10"}); // resolved by intf 1
  RouteNextHopSet nhop2 = makeNextHops({"2.2.2.10"}); // resolved by intf 2
  // 4 prefixes
  RouteV4::Prefix r1{IPAddressV4("10.1.1.0"), 24};
  RouteV4::Prefix r2{IPAddressV4("20.1.1.0"), 24};
  RouteV6::Prefix r3{IPAddressV6("1001::0"), 48};
  RouteV6::Prefix r4{IPAddressV6("2001::0"), 48};

  SwSwitchRouteUpdateWrapper u2(this->sw_);
  u2.addRoute(
      rid, r1.network, r1.mask, kClientA, RouteNextHopEntry(nhop1, DISTANCE));
  u2.addRoute(
      rid, r2.network, r2.mask, kClientA, RouteNextHopEntry(nhop2, DISTANCE));
  u2.addRoute(
      rid, r3.network, r3.mask, kClientA, RouteNextHopEntry(nhop1, DISTANCE));
  u2.addRoute(
      rid, r4.network, r4.mask, kClientA, RouteNextHopEntry(nhop2, DISTANCE));
  u2.program();
  EXPECT_NODEMAP_MATCH(this->sw_);
  auto stateV2 = this->sw_->getState();
  EXPECT_NE(stateV1, stateV2);
  // Re-add the same routes; expect no change
  SwSwitchRouteUpdateWrapper u3(this->sw_);
  u3.addRoute(
      rid, r1.network, r1.mask, kClientA, RouteNextHopEntry(nhop1, DISTANCE));
  u3.addRoute(
      rid, r2.network, r2.mask, kClientA, RouteNextHopEntry(nhop2, DISTANCE));
  u3.addRoute(
      rid, r3.network, r3.mask, kClientA, RouteNextHopEntry(nhop1, DISTANCE));
  u3.addRoute(
      rid, r4.network, r4.mask, kClientA, RouteNextHopEntry(nhop2, DISTANCE));
  u3.program();

  auto stateV3 = this->sw_->getState();
  EXPECT_EQ(stateV2, stateV3);
  // Re-add the same routes, except for one difference.  Expect an update.
  SwSwitchRouteUpdateWrapper u4(this->sw_);
  u4.addRoute(
      rid, r1.network, r1.mask, kClientA, RouteNextHopEntry(nhop1, DISTANCE));
  u4.addRoute(
      rid, r2.network, r2.mask, kClientA, RouteNextHopEntry(nhop1, DISTANCE));
  u4.addRoute(
      rid, r3.network, r3.mask, kClientA, RouteNextHopEntry(nhop1, DISTANCE));
  u4.addRoute(
      rid, r4.network, r4.mask, kClientA, RouteNextHopEntry(nhop2, DISTANCE));
  u4.program();
  auto stateV4 = this->sw_->getState();
  EXPECT_NE(stateV4, stateV3);
  EXPECT_NODEMAP_MATCH(this->sw_);

  // get all 4 routes from stateV2
  auto stateV2r1 = this->findRoute4(stateV2, rid, r1);
  auto stateV2r2 = this->findRoute4(stateV2, rid, r2);
  auto stateV2r3 = this->findRoute6(stateV2, rid, r3);
  auto stateV2r4 = this->findRoute6(stateV2, rid, r4);

  // get all 4 routes from stateV4
  auto stateV4r1 = this->findRoute4(stateV4, rid, r1);
  auto stateV4r2 = this->findRoute4(stateV4, rid, r2);
  auto stateV4r3 = this->findRoute6(stateV4, rid, r3);
  auto stateV4r4 = this->findRoute6(stateV4, rid, r4);

  EXPECT_EQ(stateV2r1, stateV4r1);
  EXPECT_NE(stateV2r2, stateV4r2); // different routes
  EXPECT_EQ(stateV2r2->getGeneration() + 1, stateV4r2->getGeneration());
  EXPECT_EQ(stateV2r3, stateV4r3);
  EXPECT_EQ(stateV2r4, stateV4r4);
}
