/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/ApplyThriftConfig.h"
#include "fboss/agent/FbossError.h"
#include "fboss/agent/FibHelpers.h"
#include "fboss/agent/SwSwitchRouteUpdateWrapper.h"
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/state/SwitchState-defs.h"
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
const ClientID kClientB = ClientID(2001);

constexpr AdminDistance DISTANCE = AdminDistance::MAX_ADMIN_DISTANCE;
constexpr AdminDistance EBGP_DISTANCE = AdminDistance::EBGP;
const RouterID kRid0 = RouterID(0);

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
RouteNextHopSet newNextHops(int n, std::string prefix) {
  RouteNextHopSet h;
  for (int i = 0; i < n; i++) {
    auto ipStr = prefix + std::to_string(i + 10);
    h.emplace(UnresolvedNextHop(IPAddress(ipStr), UCMP_DEFAULT_WEIGHT));
  }
  return h;
}

} // namespace

template <typename StandAloneRib>
class RouteTest : public ::testing::Test {
 public:
  void SetUp() override {
    auto flags = StandAloneRib::hasStandAloneRib
        ? SwitchFlags::ENABLE_STANDALONE_RIB
        : SwitchFlags::DEFAULT;
    auto config = initialConfig();
    handle_ = createTestHandle(&config, flags);
    sw_ = handle_->getSw();
  }
  virtual cfg::SwitchConfig initialConfig() const {
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
    return config;
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
  std::shared_ptr<Route<IPAddressV4>> findRoute4(
      const std::shared_ptr<SwitchState>& state,
      RouterID rid,
      const std::string& prefixStr) {
    return findRoute4(state, rid, makePrefixV4(prefixStr));
  }
  std::shared_ptr<Route<IPAddressV6>> findRoute6(
      const std::shared_ptr<SwitchState>& state,
      RouterID rid,
      const std::string& prefixStr) {
    return findRoute6(state, rid, makePrefixV6(prefixStr));
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

TYPED_TEST(RouteTest, routeApi) {
  RoutePrefixV6 pfx6{folly::IPAddressV6("2001::"), 64};
  RouteNextHopSet nhops = makeNextHops({"2::10"});
  RouteNextHopEntry nhopEntry(nhops, DISTANCE);
  auto testRouteApi = [&](auto route) {
    if constexpr (TypeParam::hasStandAloneRib) {
      EXPECT_TRUE(
          route.fromFollyDynamic(route.toFollyDynamic()).isSame(&route));
    } else {
      EXPECT_TRUE(
          route.fromFollyDynamic(route.toFollyDynamic())->isSame(&route));
    }
    EXPECT_EQ(pfx6, route.prefix());
    EXPECT_EQ(route.toRouteDetails(), route.getFields()->toRouteDetails());
    EXPECT_EQ(route.str(), route.getFields()->str());
    EXPECT_EQ(route.getFields()->flags, 0);
    EXPECT_FALSE(route.isResolved());
    EXPECT_FALSE(route.isUnresolvable());
    EXPECT_FALSE(route.isConnected());
    EXPECT_FALSE(route.isProcessing());
    EXPECT_FALSE(route.isDrop());
    EXPECT_FALSE(route.isToCPU());
    EXPECT_TRUE(route.needResolve());
    EXPECT_EQ(
        route.getForwardInfo(),
        RouteNextHopEntry(
            RouteForwardAction::DROP, AdminDistance::MAX_ADMIN_DISTANCE));

    EXPECT_EQ(nhopEntry, *route.getEntryForClient(kClientA));
    auto [bestClient, bestNhopEntry] = route.getBestEntry();
    EXPECT_EQ(bestClient, kClientA);
    EXPECT_EQ(*bestNhopEntry, nhopEntry);
    EXPECT_FALSE(route.hasNoEntry());
    EXPECT_TRUE(route.has(kClientA, nhopEntry));
    EXPECT_FALSE(route.has(kClientB, nhopEntry));
    RouteNextHopEntry nhopEntry2(makeNextHops({"1.1.1.1"}), EBGP_DISTANCE);
    route.update(kClientB, nhopEntry2);
    EXPECT_TRUE(route.has(kClientB, nhopEntry2));
    EXPECT_FALSE(route.has(kClientA, nhopEntry2));
    std::tie(bestClient, bestNhopEntry) = route.getBestEntry();
    EXPECT_EQ(bestClient, kClientB);
    EXPECT_EQ(*bestNhopEntry, nhopEntry2);
    // del entry for client, should not be found after
    route.delEntryForClient(kClientA);
    EXPECT_FALSE(route.has(kClientA, nhopEntry));
    // get, set classID
    EXPECT_EQ(std::nullopt, route.getClassID());
    route.updateClassID(cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_1);
    EXPECT_EQ(
        cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_1, route.getClassID());
  };
  if constexpr (TypeParam::hasStandAloneRib) {
    testRouteApi(RibRouteV6(pfx6, kClientA, nhopEntry));
  } else {
    testRouteApi(RouteV6(pfx6, kClientA, nhopEntry));
  }
}

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

TYPED_TEST(RouteTest, resolve) {
  auto rid = RouterID(0);
  auto stateV1 = this->sw_->getState();
  // recursive lookup
  {
    SwSwitchRouteUpdateWrapper u1(this->sw_);
    RouteNextHopSet nexthops1 =
        makeNextHops({"1.1.1.10"}); // resolved by intf 1
    u1.addRoute(
        rid,
        IPAddress("1.1.3.0"),
        24,
        kClientA,
        RouteNextHopEntry(nexthops1, DISTANCE));
    RouteNextHopSet nexthops2 = makeNextHops({"1.1.3.10"}); // rslvd. by
                                                            // '1.1.3/24'
    u1.addRoute(
        rid,
        IPAddress("8.8.8.0"),
        24,
        kClientA,
        RouteNextHopEntry(nexthops2, DISTANCE));
    u1.program();
    auto stateV2 = this->sw_->getState();
    EXPECT_NE(stateV1, stateV2);
    EXPECT_NODEMAP_MATCH(this->sw_);

    auto r21 = this->findRoute4(stateV2, rid, "1.1.3.0/24");
    EXPECT_RESOLVED(r21);
    EXPECT_FALSE(r21->isConnected());

    auto r22 = this->findRoute4(stateV2, rid, "8.8.8.0/24");
    EXPECT_RESOLVED(r22);
    EXPECT_FALSE(r22->isConnected());
    // r21 and r22 are different routes
    EXPECT_NE(r21, r22);
    EXPECT_NE(r21->prefix(), r22->prefix());
    // check the forwarding info
    RouteNextHopSet expFwd2;
    expFwd2.emplace(
        ResolvedNextHop(IPAddress("1.1.1.10"), InterfaceID(1), ECMP_WEIGHT));
    EXPECT_EQ(expFwd2, r21->getForwardInfo().getNextHopSet());
    EXPECT_EQ(expFwd2, r22->getForwardInfo().getNextHopSet());
  }
  // recursive lookup loop
  {
    // create a route table w/ the following 3 routes
    // 1. 30/8 -> 20.1.1.1
    // 2. 20/8 -> 10.1.1.1
    // 3. 10/8 -> 30.1.1.1
    // The above 3 routes causes lookup loop, which should result in
    // all unresolvable.
    SwSwitchRouteUpdateWrapper u1(this->sw_);
    u1.addRoute(
        rid,
        IPAddress("30.0.0.0"),
        8,
        kClientA,
        RouteNextHopEntry(makeNextHops({"20.1.1.1"}), DISTANCE));
    u1.addRoute(
        rid,
        IPAddress("20.0.0.0"),
        8,
        kClientA,
        RouteNextHopEntry(makeNextHops({"10.1.1.1"}), DISTANCE));
    u1.addRoute(
        rid,
        IPAddress("10.0.0.0"),
        8,
        kClientA,
        RouteNextHopEntry(makeNextHops({"30.1.1.1"}), DISTANCE));
    u1.program();
    auto stateV2 = this->sw_->getState();
    EXPECT_NE(stateV1, stateV2);
    EXPECT_NODEMAP_MATCH(this->sw_);

    auto verifyPrefix = [&](std::string prefixStr) {
      auto route = this->findRoute4(stateV2, rid, prefixStr);
      if (TypeParam::hasStandAloneRib) {
        // In standalone RIB, unresolved routes never make it to FIB
        EXPECT_EQ(route, nullptr);
      } else {
        EXPECT_FALSE(route->isResolved());
        EXPECT_TRUE(route->isUnresolvable());
        EXPECT_FALSE(route->isConnected());
        EXPECT_FALSE(route->needResolve());
        EXPECT_FALSE(route->isProcessing());
      }
    };
    verifyPrefix("10.0.0.0/8");
    verifyPrefix("20.0.0.0/8");
    verifyPrefix("30.0.0.0/8");
  }
  // recursive lookup across 2 updates
  {
    SwSwitchRouteUpdateWrapper u1(this->sw_);
    RouteNextHopSet nexthops1 = makeNextHops({"50.0.0.1"});
    u1.addRoute(
        rid,
        IPAddress("40.0.0.0"),
        8,
        kClientA,
        RouteNextHopEntry(nexthops1, DISTANCE));

    u1.program();

    auto stateV2 = this->sw_->getState();
    // 40.0.0.0/8 -> 50.0.0.1 which should be resolved by default
    // NULL route
    auto r21 = this->findRoute4(stateV2, rid, "40.0.0.0/8");
    EXPECT_TRUE(r21->isResolved());
    EXPECT_TRUE(r21->isDrop());
    EXPECT_FALSE(r21->isConnected());
    EXPECT_FALSE(r21->needResolve());

    // Resolve 50.0.0.1 this should also resolve 40.0.0.0/8
    SwSwitchRouteUpdateWrapper u2(this->sw_);
    u2.addRoute(
        rid,
        IPAddress("50.0.0.0"),
        8,
        kClientA,
        RouteNextHopEntry(makeNextHops({"1.1.1.1"}), DISTANCE));
    u2.program();

    // 40.0.0.0/8 should be resolved
    auto stateV3 = this->sw_->getState();
    auto r31 = this->findRoute4(stateV3, rid, "40.0.0.0/8");
    EXPECT_RESOLVED(r31);
    EXPECT_FALSE(r31->isConnected());

    // 50.0.0.1/32 will recurse to 50.0.0.0/8->1.1.1.1 (connected)
    auto r31NextHops = r31->getForwardInfo().getNextHopSet();
    EXPECT_EQ(1, r31NextHops.size());
    auto r32 = findLongestMatchRoute(
        this->sw_->isStandaloneRibEnabled(),
        rid,
        r31NextHops.begin()->addr().asV4(),
        stateV3);
    EXPECT_RESOLVED(r32);
    EXPECT_TRUE(r32->isConnected());

    // 50.0.0.0/8 should be resolved
    auto r33 = this->findRoute4(stateV3, rid, "50.0.0.0/8");
    EXPECT_RESOLVED(r33);
    EXPECT_FALSE(r33->isConnected());
  }
}

TYPED_TEST(RouteTest, resolveDropToCPUMix) {
  auto rid = RouterID(0);

  // add a DROP route and a ToCPU route
  SwSwitchRouteUpdateWrapper u1(this->sw_);
  u1.addRoute(
      rid,
      IPAddress("11.1.1.0"),
      24,
      kClientA,
      RouteNextHopEntry(RouteForwardAction::DROP, DISTANCE));
  u1.addRoute(
      rid,
      IPAddress("22.1.1.0"),
      24,
      kClientA,
      RouteNextHopEntry(RouteForwardAction::TO_CPU, DISTANCE));
  // then, add a route with 4 nexthops. One to each interface, one
  // to the DROP and one to the ToCPU
  RouteNextHopSet nhops = makeNextHops(
      {"1.1.1.10", // intf 1
       "2.2.2.10", // intf 2
       "11.1.1.10", // DROP
       "22.1.1.10"}); // ToCPU
  u1.addRoute(
      rid,
      IPAddress("8.8.8.0"),
      24,
      kClientA,
      RouteNextHopEntry(nhops, DISTANCE));
  u1.program();
  EXPECT_NODEMAP_MATCH(this->sw_);
  auto stateV2 = this->sw_->getState();
  {
    auto r2 = this->findRoute4(stateV2, rid, "8.8.8.0/24");
    EXPECT_RESOLVED(r2);
    EXPECT_FALSE(r2->isDrop());
    EXPECT_FALSE(r2->isToCPU());
    EXPECT_FALSE(r2->isConnected());
    const auto& fwd = r2->getForwardInfo();
    EXPECT_EQ(RouteForwardAction::NEXTHOPS, fwd.getAction());
    EXPECT_EQ(2, fwd.getNextHopSet().size());
  }

  // now update the route with just DROP and ToCPU, expect ToCPU to win
  SwSwitchRouteUpdateWrapper u2(this->sw_);
  RouteNextHopSet nhops2 = makeNextHops(
      {"11.1.1.10", // DROP
       "22.1.1.10"}); // ToCPU
  u2.addRoute(
      rid,
      IPAddress("8.8.8.0"),
      24,
      kClientA,
      RouteNextHopEntry(nhops2, DISTANCE));
  u2.program();
  auto stateV3 = this->sw_->getState();
  EXPECT_NODEMAP_MATCH(this->sw_);
  {
    auto r2 = this->findRoute4(stateV3, rid, "8.8.8.0/24");
    EXPECT_RESOLVED(r2);
    EXPECT_FALSE(r2->isDrop());
    EXPECT_TRUE(r2->isToCPU());
    EXPECT_FALSE(r2->isConnected());
    const auto& fwd = r2->getForwardInfo();
    EXPECT_EQ(RouteForwardAction::TO_CPU, fwd.getAction());
    EXPECT_EQ(0, fwd.getNextHopSet().size());
  }
  // now update the route with just DROP
  SwSwitchRouteUpdateWrapper u3(this->sw_);
  RouteNextHopSet nhops3 = makeNextHops({"11.1.1.10"}); // DROP
  u3.addRoute(
      rid,
      IPAddress("8.8.8.0"),
      24,
      kClientA,
      RouteNextHopEntry(nhops3, DISTANCE));
  u3.program();
  auto stateV4 = this->sw_->getState();
  EXPECT_NODEMAP_MATCH(this->sw_);
  {
    auto r2 = this->findRoute4(stateV4, rid, "8.8.8.0/24");
    EXPECT_RESOLVED(r2);
    EXPECT_TRUE(r2->isDrop());
    EXPECT_FALSE(r2->isToCPU());
    EXPECT_FALSE(r2->isConnected());
    const auto& fwd = r2->getForwardInfo();
    EXPECT_EQ(RouteForwardAction::DROP, fwd.getAction());
    EXPECT_EQ(0, fwd.getNextHopSet().size());
  }
}

// Testing add and delete ECMP routes
TYPED_TEST(RouteTest, addDel) {
  auto rid = RouterID(0);

  RouteNextHopSet nexthops = makeNextHops(
      {"1.1.1.10", // intf 1
       "2::2", // intf 2
       "1.1.2.10"}); // Drop (via default null route)
  RouteNextHopSet nexthops2 = makeNextHops(
      {"1.1.3.10", // Drop (via default null route)
       "11:11::1"}); // Drop (via default null route)

  SwSwitchRouteUpdateWrapper u1(this->sw_);
  u1.addRoute(
      rid,
      IPAddress("10.1.1.1"),
      24,
      kClientA,
      RouteNextHopEntry(nexthops, DISTANCE));
  u1.addRoute(
      rid,
      IPAddress("2001::1"),
      48,
      kClientA,
      RouteNextHopEntry(nexthops, DISTANCE));
  u1.program();

  auto stateV2 = this->sw_->getState();
  // v4 route
  auto r2 = this->findRoute4(stateV2, rid, "10.1.1.0/24");
  EXPECT_RESOLVED(r2);
  EXPECT_FALSE(r2->isDrop());
  EXPECT_FALSE(r2->isToCPU());
  EXPECT_FALSE(r2->isConnected());
  // v6 route
  auto r2v6 = this->findRoute6(stateV2, rid, "2001::0/48");
  EXPECT_RESOLVED(r2v6);
  EXPECT_FALSE(r2v6->isDrop());
  EXPECT_FALSE(r2v6->isToCPU());
  EXPECT_FALSE(r2v6->isConnected());
  // forwarding info
  EXPECT_EQ(RouteForwardAction::NEXTHOPS, r2->getForwardInfo().getAction());
  EXPECT_EQ(RouteForwardAction::NEXTHOPS, r2v6->getForwardInfo().getAction());
  const auto& fwd2 = r2->getForwardInfo().getNextHopSet();
  const auto& fwd2v6 = r2v6->getForwardInfo().getNextHopSet();
  EXPECT_EQ(2, fwd2.size());
  EXPECT_EQ(2, fwd2v6.size());
  RouteNextHopSet expFwd2;
  expFwd2.emplace(
      ResolvedNextHop(IPAddress("1.1.1.10"), InterfaceID(1), ECMP_WEIGHT));
  expFwd2.emplace(
      ResolvedNextHop(IPAddress("2::2"), InterfaceID(2), ECMP_WEIGHT));
  EXPECT_EQ(expFwd2, fwd2);
  EXPECT_EQ(expFwd2, fwd2v6);

  // change the nexthops of the V4 route
  SwSwitchRouteUpdateWrapper u2(this->sw_);
  u2.addRoute(
      rid,
      IPAddress("10.1.1.1"),
      24,
      kClientA,
      RouteNextHopEntry(nexthops2, DISTANCE));
  u2.program();
  EXPECT_NODEMAP_MATCH(this->sw_);
  auto stateV3 = this->sw_->getState();

  auto r3 = this->findRoute4(stateV3, rid, "10.1.1.0/24");
  ASSERT_NE(nullptr, r3);
  EXPECT_TRUE(r3->isResolved()); // Resolved to default NULL
  EXPECT_TRUE(r3->isDrop());
  EXPECT_FALSE(r3->isConnected());
  EXPECT_FALSE(r3->needResolve());

  // re-add the same route does not cause change
  SwSwitchRouteUpdateWrapper u3(this->sw_);
  u3.addRoute(
      rid,
      IPAddress("10.1.1.1"),
      24,
      kClientA,
      RouteNextHopEntry(nexthops2, DISTANCE));
  u3.program();
  EXPECT_EQ(stateV3, this->sw_->getState());

  // now delete the V4 route
  SwSwitchRouteUpdateWrapper u4(this->sw_);
  u4.delRoute(rid, IPAddress("10.1.1.1"), 24, kClientA);
  u4.program();
  EXPECT_NODEMAP_MATCH(this->sw_);

  auto r5 = this->findRoute4(this->sw_->getState(), rid, "10.1.1.0/24");
  EXPECT_EQ(nullptr, r5);

  // change an old route to punt to CPU, add a new route to DROP
  SwSwitchRouteUpdateWrapper u5(this->sw_);
  u5.addRoute(
      rid,
      IPAddress("10.1.1.0"),
      24,
      kClientA,
      RouteNextHopEntry(RouteForwardAction::TO_CPU, DISTANCE));
  u5.addRoute(
      rid,
      IPAddress("10.1.2.0"),
      24,
      kClientA,
      RouteNextHopEntry(RouteForwardAction::DROP, DISTANCE));
  u5.program();
  EXPECT_NODEMAP_MATCH(this->sw_);
  auto stateV6 = this->sw_->getState();

  auto r6_1 = this->findRoute4(stateV6, rid, "10.1.1.0/24");
  EXPECT_RESOLVED(r6_1);
  EXPECT_FALSE(r6_1->isConnected());
  EXPECT_TRUE(r6_1->isToCPU());
  EXPECT_FALSE(r6_1->isDrop());
  EXPECT_EQ(RouteForwardAction::TO_CPU, r6_1->getForwardInfo().getAction());

  auto r6_2 = this->findRoute4(stateV6, rid, "10.1.2.0/24");
  EXPECT_RESOLVED(r6_2);
  EXPECT_FALSE(r6_2->isConnected());
  EXPECT_FALSE(r6_2->isToCPU());
  EXPECT_TRUE(r6_2->isDrop());
  EXPECT_EQ(RouteForwardAction::DROP, r6_2->getForwardInfo().getAction());
}

TYPED_TEST(RouteTest, InterfaceRoutes) {
  RouterID rid = RouterID(0);
  auto stateV1 = this->sw_->getState();
  // verify the ipv4 interface route
  {
    auto rt = this->findRoute4(stateV1, rid, "1.1.1.0/24");
    EXPECT_EQ(0, rt->getGeneration());
    EXPECT_RESOLVED(rt);
    EXPECT_TRUE(rt->isConnected());
    EXPECT_FALSE(rt->isToCPU());
    EXPECT_FALSE(rt->isDrop());
    EXPECT_EQ(RouteForwardAction::NEXTHOPS, rt->getForwardInfo().getAction());
    EXPECT_FWD_INFO(rt, InterfaceID(1), "1.1.1.1");
  }
  // verify the ipv6 interface route
  {
    auto rt = this->findRoute6(stateV1, rid, "2::0/48");
    EXPECT_EQ(0, rt->getGeneration());
    EXPECT_RESOLVED(rt);
    EXPECT_TRUE(rt->isConnected());
    EXPECT_FALSE(rt->isToCPU());
    EXPECT_FALSE(rt->isDrop());
    EXPECT_EQ(RouteForwardAction::NEXTHOPS, rt->getForwardInfo().getAction());
    EXPECT_FWD_INFO(rt, InterfaceID(2), "2::1");
  }

  {
    // verify v6 link local route
    auto rt = this->findRoute6(stateV1, rid, "fe80::/64");
    EXPECT_EQ(0, rt->getGeneration());
    EXPECT_RESOLVED(rt);
    EXPECT_FALSE(rt->isConnected());
    EXPECT_TRUE(rt->isToCPU());
    EXPECT_EQ(RouteForwardAction::TO_CPU, rt->getForwardInfo().getAction());
    const auto& fwds = rt->getForwardInfo().getNextHopSet();
    EXPECT_EQ(0, fwds.size());
  }

  auto config = this->initialConfig();
  // swap the interface addresses which causes route change
  config.interfaces_ref()[1].ipAddresses_ref()[0] = "1.1.1.1/24";
  config.interfaces_ref()[1].ipAddresses_ref()[1] = "1::1/48";
  config.interfaces_ref()[0].ipAddresses_ref()[0] = "2.2.2.2/24";
  config.interfaces_ref()[0].ipAddresses_ref()[1] = "2::1/48";
  auto updateFn = [&config, this](const std::shared_ptr<SwitchState>& in) {
    return applyThriftConfig(
        in,
        &config,
        this->sw_->getPlatform(),
        TypeParam::hasStandAloneRib ? this->sw_->getRib() : nullptr);
  };
  this->sw_->updateStateBlocking("New config", updateFn);
  auto stateV2 = this->sw_->getState();
  EXPECT_NE(stateV1, stateV2);
  // verify the ipv4 route
  {
    auto rt = this->findRoute4(stateV2, rid, "1.1.1.0/24");
    EXPECT_EQ(1, rt->getGeneration());
    EXPECT_FWD_INFO(rt, InterfaceID(2), "1.1.1.1");
  }
  // verify the ipv6 route
  {
    auto rt = this->findRoute6(stateV2, rid, "2::0/48");
    EXPECT_EQ(1, rt->getGeneration());
    EXPECT_FWD_INFO(rt, InterfaceID(1), "2::1");
  }
}

namespace TEMP {
struct Route {
  uint32_t vrf;
  IPAddress prefix;
  uint8_t len;
  Route(uint32_t vrf, IPAddress prefix, uint8_t len)
      : vrf(vrf), prefix(prefix), len(len) {}
  bool operator<(const Route& rt) const {
    if (vrf < rt.vrf) {
      return true;
    } else if (vrf > rt.vrf) {
      return false;
    }
    if (len < rt.len) {
      return true;
    } else if (len > rt.len) {
      return false;
    }
    return prefix < rt.prefix;
  }
  bool operator==(const Route& rt) const {
    return vrf == rt.vrf && len == rt.len && prefix == rt.prefix;
  }
};
} // namespace TEMP

void checkChangedRoute(
    bool isStandaloneRibEnabled,
    const shared_ptr<SwitchState>& oldState,
    const shared_ptr<SwitchState>& newState,
    const std::set<TEMP::Route> changedIDs,
    const std::set<TEMP::Route> addedIDs,
    const std::set<TEMP::Route> removedIDs) {
  std::set<TEMP::Route> foundChanged;
  std::set<TEMP::Route> foundAdded;
  std::set<TEMP::Route> foundRemoved;
  StateDelta delta(oldState, newState);

  forEachChangedRoute(
      isStandaloneRibEnabled,
      delta,
      [&](RouterID id, const auto& oldRt, const auto& newRt) {
        EXPECT_EQ(oldRt->prefix(), newRt->prefix());
        EXPECT_NE(oldRt, newRt);
        const auto prefix = newRt->prefix();
        auto ret = foundChanged.insert(
            TEMP::Route(id, IPAddress(prefix.network), prefix.mask));
        EXPECT_TRUE(ret.second);
      },
      [&](RouterID id, const auto& rt) {
        const auto prefix = rt->prefix();
        auto ret = foundAdded.insert(
            TEMP::Route(id, IPAddress(prefix.network), prefix.mask));
        EXPECT_TRUE(ret.second);
      },
      [&](RouterID id, const auto& rt) {
        const auto prefix = rt->prefix();
        auto ret = foundRemoved.insert(
            TEMP::Route(id, IPAddress(prefix.network), prefix.mask));
        EXPECT_TRUE(ret.second);
      });

  EXPECT_EQ(changedIDs, foundChanged);
  EXPECT_EQ(addedIDs, foundAdded);
  EXPECT_EQ(removedIDs, foundRemoved);
}

TYPED_TEST(RouteTest, applyNewConfig) {
  auto config = this->initialConfig();
  config.vlans_ref()->resize(6);
  config.vlans_ref()[4].id_ref() = 5;
  config.vlans_ref()[5].id_ref() = 6;
  config.interfaces_ref()->resize(6);
  config.interfaces_ref()[4].intfID_ref() = 5;
  config.interfaces_ref()[4].vlanID_ref() = 5;
  config.interfaces_ref()[4].routerID_ref() = 0;
  config.interfaces_ref()[4].mac_ref() = "00:00:00:00:00:55";
  config.interfaces_ref()[4].ipAddresses_ref()->resize(4);
  config.interfaces_ref()[4].ipAddresses_ref()[0] = "5.1.1.1/24";
  config.interfaces_ref()[4].ipAddresses_ref()[1] = "5.1.1.2/24";
  config.interfaces_ref()[4].ipAddresses_ref()[2] = "5.1.1.10/24";
  config.interfaces_ref()[4].ipAddresses_ref()[3] = "::1/48";
  config.interfaces_ref()[5].intfID_ref() = 6;
  config.interfaces_ref()[5].vlanID_ref() = 6;
  config.interfaces_ref()[5].routerID_ref() = 1;
  config.interfaces_ref()[5].mac_ref() = "00:00:00:00:00:66";
  config.interfaces_ref()[5].ipAddresses_ref()->resize(2);
  config.interfaces_ref()[5].ipAddresses_ref()[0] = "5.1.1.1/24";
  config.interfaces_ref()[5].ipAddresses_ref()[1] = "::1/48";

  auto platform = this->sw_->getPlatform();
  auto rib = this->sw_->getRib();
  XLOG(INFO) << " RIB: " << (rib ? "YES" : "NO");
  auto stateV0 = this->sw_->getState();
  auto stateV1 =
      publishAndApplyConfig(this->sw_->getState(), &config, platform, rib);

  ASSERT_NE(nullptr, stateV1);
  stateV1->publish();

  checkChangedRoute(
      TypeParam::hasStandAloneRib,
      stateV0,
      stateV1,
      {},
      {
          TEMP::Route{0, IPAddress("5.1.1.0"), 24},
          TEMP::Route{0, IPAddress("::0"), 48},
          TEMP::Route{1, IPAddress("5.1.1.0"), 24},
          TEMP::Route{1, IPAddress("::0"), 48},
          TEMP::Route{1, IPAddress("fe80::"), 64},
      },
      {});
  // change an interface address
  config.interfaces_ref()[4].ipAddresses_ref()[3] = "11::11/48";

  auto stateV2 = publishAndApplyConfig(stateV1, &config, platform, rib);
  ASSERT_NE(nullptr, stateV2);
  stateV2->publish();

  checkChangedRoute(
      TypeParam::hasStandAloneRib,
      stateV1,
      stateV2,
      {},
      {TEMP::Route{0, IPAddress("11::0"), 48}},
      {TEMP::Route{0, IPAddress("::0"), 48}});

  // move one interface to cause same route prefix conflict
  config.interfaces_ref()[5].routerID_ref() = 0;
  EXPECT_THROW(
      publishAndApplyConfig(stateV2, &config, platform, rib), FbossError);

  // add a new interface in a new VRF
  config.vlans_ref()->resize(7);
  config.vlans_ref()[6].id_ref() = 7;
  config.interfaces_ref()->resize(7);
  config.interfaces_ref()[6].intfID_ref() = 7;
  config.interfaces_ref()[6].vlanID_ref() = 7;
  config.interfaces_ref()[6].routerID_ref() = 2;
  config.interfaces_ref()[6].mac_ref() = "00:00:00:00:00:77";
  config.interfaces_ref()[6].ipAddresses_ref()->resize(2);
  config.interfaces_ref()[6].ipAddresses_ref()[0] = "1.1.1.1/24";
  config.interfaces_ref()[6].ipAddresses_ref()[1] = "::1/48";
  // and move one interface to another vrf and fix the address conflict
  config.interfaces_ref()[5].routerID_ref() = 0;
  config.interfaces_ref()[5].ipAddresses_ref()->resize(2);
  config.interfaces_ref()[5].ipAddresses_ref()[0] = "5.2.2.1/24";
  config.interfaces_ref()[5].ipAddresses_ref()[1] = "5::6/48";

  auto stateV3 = publishAndApplyConfig(stateV2, &config, platform, rib);
  ASSERT_NE(nullptr, stateV3);
  checkChangedRoute(
      TypeParam::hasStandAloneRib,
      stateV2,
      stateV3,
      {},
      {
          TEMP::Route{0, IPAddress("5.2.2.0"), 24},
          TEMP::Route{0, IPAddress("5::0"), 48},
          TEMP::Route{2, IPAddress("1.1.1.0"), 24},
          TEMP::Route{2, IPAddress("::0"), 48},
          TEMP::Route{2, IPAddress("fe80::"), 64},
      },
      {
          TEMP::Route{1, IPAddress("5.1.1.0"), 24},
          TEMP::Route{1, IPAddress("::0"), 48},
          TEMP::Route{1, IPAddress("fe80::"), 64},
      });

  // re-apply the same configure generates no change
  auto stateV4 = publishAndApplyConfig(stateV3, &config, platform, rib);
  if (stateV4) {
    // FIXME - reapplying the same config on standalone rib should yield null,
    // but it does result in new state. The state delta is empty though, so
    // no change will be programmed down
    checkChangedRoute(
        TypeParam::hasStandAloneRib, stateV3, stateV4, {}, {}, {});
  }
}

TYPED_TEST(RouteTest, changedRoutesPostUpdate) {
  auto rid = RouterID(0);
  RouteNextHopSet nexthops = makeNextHops(
      {"1.1.1.10", // resolved by intf 1
       "2::2"}); // resolved by intf 2

  auto state1 = this->sw_->getState();
  // Add a couple of routes
  SwSwitchRouteUpdateWrapper u1(this->sw_);
  u1.addRoute(
      rid,
      IPAddress("10.1.1.0"),
      24,
      kClientA,
      RouteNextHopEntry(nexthops, DISTANCE));
  u1.addRoute(
      rid,
      IPAddress("2001::0"),
      48,
      kClientA,
      RouteNextHopEntry(nexthops, DISTANCE));
  u1.program();
  auto state2 = this->sw_->getState();
  // v4 route
  auto rtV4 = this->findRoute4(state2, rid, "10.1.1.0/24");
  EXPECT_TRUE(rtV4->isResolved());
  EXPECT_FALSE(rtV4->isConnected());
  // v6 route
  auto rtV6 = this->findRoute6(state2, rid, "2001::/48");
  EXPECT_TRUE(rtV6->isResolved());
  EXPECT_FALSE(rtV6->isConnected());
  checkChangedRoute(
      TypeParam::hasStandAloneRib,
      state1,
      state2,
      {},
      {
          TEMP::Route{0, IPAddress("10.1.1.0"), 24},
          TEMP::Route{0, IPAddress("2001::0"), 48},
      },
      {});
  // Add 2 more routes
  SwSwitchRouteUpdateWrapper u2(this->sw_);
  u2.addRoute(
      rid,
      IPAddress("10.10.1.0"),
      24,
      kClientA,
      RouteNextHopEntry(nexthops, DISTANCE));
  u2.addRoute(
      rid,
      IPAddress("2001:10::0"),
      48,
      kClientA,
      RouteNextHopEntry(nexthops, DISTANCE));
  u2.program();
  auto state3 = this->sw_->getState();
  // v4 route
  rtV4 = this->findRoute4(state3, rid, "10.10.1.0/24");
  EXPECT_TRUE(rtV4->isResolved());
  EXPECT_FALSE(rtV4->isConnected());
  // v6 route
  rtV6 = this->findRoute6(state3, rid, "2001:10::/48");
  EXPECT_TRUE(rtV6->isResolved());
  EXPECT_FALSE(rtV6->isConnected());
  checkChangedRoute(
      TypeParam::hasStandAloneRib,
      state2,
      state3,
      {},
      {
          TEMP::Route{0, IPAddress("10.10.1.0"), 24},
          TEMP::Route{0, IPAddress("2001:10::"), 48},
      },
      {});
}

TYPED_TEST(RouteTest, PruneAddedRoutes) {
  auto rid0 = RouterID(0);
  SwSwitchRouteUpdateWrapper updater(this->sw_);
  auto r1prefix = IPAddressV4("20.0.1.51");
  auto r1prefixLen = 24;
  auto r1nexthops = makeNextHops({"10.0.0.1", "30.0.21.51" /* unresolved */});
  updater.addRoute(
      rid0,
      r1prefix,
      r1prefixLen,
      kClientA,
      RouteNextHopEntry(r1nexthops, DISTANCE));

  updater.program();
  RouteV4::Prefix prefix1{IPAddressV4("20.0.1.51"), 24};

  auto newRouteEntry = findLongestMatchRoute<IPAddressV4>(
      TypeParam::hasStandAloneRib,
      rid0,
      prefix1.network,
      this->sw_->getState());
  EXPECT_NE(nullptr, newRouteEntry);
  EXPECT_EQ(
      newRouteEntry->prefix(), (RouteV4::Prefix{IPAddressV4("20.0.1.0"), 24}));

  EXPECT_TRUE(newRouteEntry->isPublished());
  auto revertState = this->sw_->getState();
  SwitchState::revertNewRouteEntry<IPAddressV4>(
      TypeParam::hasStandAloneRib, rid0, newRouteEntry, nullptr, &revertState);
  // Make sure that state3 changes as a result of pruning
  auto remainingRouteEntry = findLongestMatchRoute<IPAddressV4>(
      TypeParam::hasStandAloneRib, rid0, prefix1.network, revertState);
  // Will match default route after delete
  EXPECT_EQ(
      remainingRouteEntry->prefix(),
      (RouteV4::Prefix{IPAddressV4("0.0.0.0"), 0}));
}

// Test that pruning of changed routes happens correctly.
TYPED_TEST(RouteTest, PruneChangedRoutes) {
  // Add routes
  // Change one of them
  // Prune the changed one
  // Check that the pruning happened correctly
  // state1
  //  ... Add route for prefix41
  //  ... Add route for prefix42 (RouteForwardAction::TO_CPU)
  // state2
  auto rid0 = RouterID(0);
  SwSwitchRouteUpdateWrapper updater(this->sw_);

  RouteV4::Prefix prefix41{IPAddressV4("20.0.21.41"), 32};
  auto nexthops41 = makeNextHops({"10.0.0.1", "face:b00c:0:21::41"});
  updater.addRoute(
      rid0,
      prefix41.network,
      prefix41.mask,
      kClientA,
      RouteNextHopEntry(nexthops41, DISTANCE));

  RouteV6::Prefix prefix42{IPAddressV6("facf:b00c:0:21::42"), 96};
  updater.addRoute(
      rid0,
      prefix42.network,
      prefix42.mask,
      kClientA,
      RouteNextHopEntry(RouteForwardAction::TO_CPU, DISTANCE));

  updater.program();

  auto oldEntry = findLongestMatchRoute<IPAddressV6>(
      TypeParam::hasStandAloneRib,
      rid0,
      prefix42.network,
      this->sw_->getState());
  EXPECT_NE(nullptr, oldEntry);
  EXPECT_TRUE(oldEntry->isToCPU());

  // state2
  //  ... Make route for prefix42 resolve to actual nexthops
  // state3

  auto nexthops42 = makeNextHops({"10.0.0.1", "face:b00c:0:21::42"});
  updater.addRoute(
      rid0,
      prefix42.network,
      prefix42.mask,
      kClientA,
      RouteNextHopEntry(nexthops42, DISTANCE));
  updater.program();

  auto newEntry = findLongestMatchRoute<IPAddressV6>(
      TypeParam::hasStandAloneRib,
      rid0,
      prefix42.network,
      this->sw_->getState());

  EXPECT_FALSE(newEntry->isToCPU());
  // state3
  //  ... revert route for prefix42
  // state4
  auto revertState = this->sw_->getState();
  SwitchState::revertNewRouteEntry(
      TypeParam::hasStandAloneRib, rid0, newEntry, oldEntry, &revertState);

  auto revertedEntry = findLongestMatchRoute<IPAddressV6>(
      TypeParam::hasStandAloneRib, rid0, prefix42.network, revertState);
  EXPECT_TRUE(revertedEntry->isToCPU());
}

// Test adding and deleting per-client nexthops lists
TYPED_TEST(RouteTest, modRoutes) {
  SwSwitchRouteUpdateWrapper u1(this->sw_);

  RouteV4::Prefix prefix10{IPAddressV4("10.10.10.10"), 32};
  RouteV4::Prefix prefix99{IPAddressV4("99.99.99.99"), 32};

  RouteNextHopSet nexthops1 =
      makeResolvedNextHops({{InterfaceID(1), "1.1.1.1"}});
  RouteNextHopSet nexthops2 =
      makeResolvedNextHops({{InterfaceID(2), "2.2.2.2"}});
  RouteNextHopSet nexthops3 =
      makeResolvedNextHops({{InterfaceID(3), "3.3.3.3"}});

  u1.addRoute(
      kRid0,
      IPAddress("10.10.10.10"),
      32,
      kClientB,
      RouteNextHopEntry(nexthops2, DISTANCE));
  u1.addRoute(
      kRid0,
      IPAddress("10.10.10.10"),
      32,
      kClientA,
      RouteNextHopEntry(nexthops1, DISTANCE));
  u1.addRoute(
      kRid0,
      IPAddress("99.99.99.99"),
      32,
      kClientA,
      RouteNextHopEntry(nexthops3, DISTANCE));
  u1.program();

  auto rt10 = this->findRoute4(this->sw_->getState(), kRid0, prefix10);
  auto rt99 = this->findRoute4(this->sw_->getState(), kRid0, prefix99);
  EXPECT_EQ(rt10->getForwardInfo().getNextHopSet(), nexthops1);
  EXPECT_EQ(rt99->getForwardInfo().getNextHopSet(), nexthops3);

  SwSwitchRouteUpdateWrapper u2(this->sw_);
  u2.delRoute(kRid0, IPAddress("10.10.10.10"), 32, kClientA);
  u2.program();
  // 10.10.10.10 should now get clientBs nhops
  rt10 = this->findRoute4(this->sw_->getState(), kRid0, prefix10);
  rt99 = this->findRoute4(this->sw_->getState(), kRid0, prefix99);
  EXPECT_EQ(rt10->getForwardInfo().getNextHopSet(), nexthops2);
  EXPECT_EQ(rt99->getForwardInfo().getNextHopSet(), nexthops3);
  // Delete the second client/nexthop pair
  // The route & prefix should disappear altogether.

  SwSwitchRouteUpdateWrapper u3(this->sw_);
  u3.delRoute(kRid0, IPAddress("10.10.10.10"), 32, kClientB);
  u3.program();
  rt10 = this->findRoute4(this->sw_->getState(), kRid0, prefix10);
  rt99 = this->findRoute4(this->sw_->getState(), kRid0, prefix99);
  EXPECT_EQ(rt10, nullptr);
  EXPECT_EQ(rt99->getForwardInfo().getNextHopSet(), nexthops3);
}
// Test interface routes when we have more than one address per
// address family in an interface
template <typename StandAloneRib>
class MultipleAddressInterfaceTest : public RouteTest<StandAloneRib> {
 public:
  cfg::SwitchConfig initialConfig() const override {
    cfg::SwitchConfig config;
    config.vlans_ref()->resize(1);
    config.vlans_ref()[0].id_ref() = 1;

    config.interfaces_ref()->resize(1);
    config.interfaces_ref()[0].intfID_ref() = 1;
    config.interfaces_ref()[0].vlanID_ref() = 1;
    config.interfaces_ref()[0].routerID_ref() = 0;
    config.interfaces_ref()[0].mac_ref() = "00:00:00:00:00:11";
    config.interfaces_ref()[0].ipAddresses_ref()->resize(4);
    config.interfaces_ref()[0].ipAddresses_ref()[0] = "1.1.1.1/24";
    config.interfaces_ref()[0].ipAddresses_ref()[1] = "1.1.1.2/24";
    config.interfaces_ref()[0].ipAddresses_ref()[2] = "1::1/48";
    config.interfaces_ref()[0].ipAddresses_ref()[3] = "1::2/48";
    return config;
  }
};
TYPED_TEST_CASE(MultipleAddressInterfaceTest, RouteTestTypes);

TYPED_TEST(MultipleAddressInterfaceTest, twoAddrsForInterface) {
  auto rid = RouterID(0);
  auto [v4Routes, v6Routes] =
      getRouteCount(TypeParam::hasStandAloneRib, this->sw_->getState());

  EXPECT_EQ(2, v4Routes); // ALPM default + intf routes
  EXPECT_EQ(3, v6Routes); // ALPM default + intf + link local routes
  {
    auto rt = this->findRoute4(this->sw_->getState(), rid, "1.1.1.0/24");
    EXPECT_EQ(0, rt->getGeneration());
    EXPECT_TRUE(rt->isResolved());
    EXPECT_TRUE(rt->isConnected());
    EXPECT_FALSE(rt->isToCPU());
    EXPECT_FALSE(rt->isDrop());
    EXPECT_FWD_INFO(rt, InterfaceID(1), "1.1.1.2");
  }
  {
    auto rt = this->findRoute6(this->sw_->getState(), rid, "1::0/48");
    EXPECT_EQ(0, rt->getGeneration());
    EXPECT_TRUE(rt->isResolved());
    EXPECT_TRUE(rt->isConnected());
    EXPECT_FALSE(rt->isToCPU());
    EXPECT_FALSE(rt->isDrop());
    EXPECT_FWD_INFO(rt, InterfaceID(1), "1::2");
  }
}

template <typename StandAloneRib>
class StaticRoutesTest : public RouteTest<StandAloneRib> {
 public:
  cfg::SwitchConfig initialConfig() const override {
    auto config = RouteTest<StandAloneRib>::initialConfig();
    // Add v4/v6 static routes with nhops
    config.staticRoutesWithNhops_ref()->resize(2);
    config.staticRoutesWithNhops_ref()[0].nexthops_ref()->resize(1);
    config.staticRoutesWithNhops_ref()[0].prefix_ref() = "2001::/64";
    config.staticRoutesWithNhops_ref()[0].nexthops_ref()[0] = "2::2";
    config.staticRoutesWithNhops_ref()[1].nexthops_ref()->resize(1);
    config.staticRoutesWithNhops_ref()[1].prefix_ref() = "20.20.20.0/24";
    config.staticRoutesWithNhops_ref()[1].nexthops_ref()[0] = "2.2.2.3";

    auto insertStaticNoNhopRoutes = [=](auto& staticRouteNoNhops,
                                        int prefixStartIdx) {
      staticRouteNoNhops.resize(2);
      staticRouteNoNhops[0].prefix_ref() =
          folly::sformat("240{}::/64", prefixStartIdx);
      staticRouteNoNhops[1].prefix_ref() =
          folly::sformat("30.30.{}.0/24", prefixStartIdx);
    };
    // Add v4/v6 static routes to CPU/NULL
    insertStaticNoNhopRoutes(*config.staticRoutesToCPU_ref(), 1);
    insertStaticNoNhopRoutes(*config.staticRoutesToNull_ref(), 2);

    return config;
  }
};
TYPED_TEST_CASE(StaticRoutesTest, RouteTestTypes);

TYPED_TEST(StaticRoutesTest, staticRoutesGetApplied) {
  auto [numV4Routes, numV6Routes] =
      getRouteCount(TypeParam::hasStandAloneRib, this->sw_->getState());
  EXPECT_EQ(8, numV4Routes);
  EXPECT_EQ(9, numV6Routes);
}
