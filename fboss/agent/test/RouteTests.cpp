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
#include "fboss/agent/if/gen-cpp2/mpls_types.h"
#include "fboss/agent/state/SwitchState-defs.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/test/HwTestHandle.h"
#include "fboss/agent/test/TestUtils.h"

#include "fboss/agent/AddressUtil.h"
#include "fboss/agent/if/gen-cpp2/common_types.h"

#include <folly/logging/xlog.h>
#include <gtest/gtest.h>
#include <optional>

using namespace facebook::fboss;
using facebook::network::toBinaryAddress;
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

const std::optional<RouteCounterID> kCounterID1("route.counter.0");
const std::optional<RouteCounterID> kCounterID2("route.counter.1");

const std::optional<cfg::AclLookupClass> kClassID1(
    cfg::AclLookupClass::DST_CLASS_L3_DPR);
const std::optional<cfg::AclLookupClass> kClassID2(
    cfg::AclLookupClass::DST_CLASS_L3_LOCAL_IP6);

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

RouteNextHopSet newNextHops(int n, std::string prefix) {
  RouteNextHopSet h;
  for (int i = 0; i < n; i++) {
    auto ipStr = prefix + std::to_string(i + 10);
    h.emplace(UnresolvedNextHop(IPAddress(ipStr), UCMP_DEFAULT_WEIGHT));
  }
  return h;
}

} // namespace

class RouteTest : public ::testing::Test {
 public:
  void SetUp() override {
    auto config = initialConfig();
    handle_ = createTestHandle(&config);
    sw_ = handle_->getSw();
  }
  virtual cfg::SwitchConfig initialConfig() const {
    cfg::SwitchConfig config;
    config.vlans()->resize(4);
    config.vlans()[0].id() = 1;
    config.vlans()[1].id() = 2;
    config.vlans()[2].id() = 3;
    config.vlans()[3].id() = 4;

    config.interfaces()->resize(4);
    config.interfaces()[0].intfID() = 1;
    config.interfaces()[0].vlanID() = 1;
    config.interfaces()[0].routerID() = 0;
    config.interfaces()[0].mac() = "00:00:00:00:00:11";
    config.interfaces()[0].ipAddresses()->resize(2);
    config.interfaces()[0].ipAddresses()[0] = "1.1.1.1/24";
    config.interfaces()[0].ipAddresses()[1] = "1::1/48";

    config.interfaces()[1].intfID() = 2;
    config.interfaces()[1].vlanID() = 2;
    config.interfaces()[1].routerID() = 0;
    config.interfaces()[1].mac() = "00:00:00:00:00:22";
    config.interfaces()[1].ipAddresses()->resize(2);
    config.interfaces()[1].ipAddresses()[0] = "2.2.2.2/24";
    config.interfaces()[1].ipAddresses()[1] = "2::1/48";

    config.interfaces()[2].intfID() = 3;
    config.interfaces()[2].vlanID() = 3;
    config.interfaces()[2].routerID() = 0;
    config.interfaces()[2].mac() = "00:00:00:00:00:33";
    config.interfaces()[2].ipAddresses()->resize(2);
    config.interfaces()[2].ipAddresses()[0] = "3.3.3.3/24";
    config.interfaces()[2].ipAddresses()[1] = "3::1/48";

    config.interfaces()[3].intfID() = 4;
    config.interfaces()[3].vlanID() = 4;
    config.interfaces()[3].routerID() = 0;
    config.interfaces()[3].mac() = "00:00:00:00:00:44";
    config.interfaces()[3].ipAddresses()->resize(2);
    config.interfaces()[3].ipAddresses()[0] = "4.4.4.4/24";
    config.interfaces()[3].ipAddresses()[1] = "4::1/48";
    return config;
  }

  std::shared_ptr<Route<IPAddressV4>> findRoute4(
      const std::shared_ptr<SwitchState>& state,
      RouterID rid,
      const RoutePrefixV4& prefix) {
    return findRouteImpl<IPAddressV4>(
        rid, {prefix.network(), prefix.mask()}, state);
  }
  std::shared_ptr<Route<IPAddressV6>> findRoute6(
      const std::shared_ptr<SwitchState>& state,
      RouterID rid,
      const RoutePrefixV6& prefix) {
    return findRouteImpl<IPAddressV6>(
        rid, {prefix.network(), prefix.mask()}, state);
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
    return findRoute<AddrT>(rid, prefix, state);
  }

 protected:
  SwSwitch* sw_;
  std::unique_ptr<HwTestHandle> handle_;
};

TEST_F(RouteTest, routeApi) {
  RoutePrefixV6 pfx6{folly::IPAddressV6("2001::"), 64};
  RouteNextHopSet nhops = makeNextHops({"2::10"});
  RouteNextHopEntry nhopEntry(nhops, DISTANCE);
  auto testRouteApi = [&](auto route) {
    EXPECT_TRUE(std::make_shared<RouteV6>(route.toThrift())->isSame(&route));
    EXPECT_EQ(pfx6, route.prefix());
    EXPECT_EQ(route.toRouteDetails(), route.toRouteDetails());
    EXPECT_EQ(route.str(), route.str());
    EXPECT_EQ(route.flags(), 0);
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
    // check counterID
    route.update(kClientA, nhopEntry);
    route.update(kClientB, nhopEntry2);
    EXPECT_EQ(std::nullopt, route.getForwardInfo().getCounterID());
    EXPECT_TRUE(!route.getEntryForClient(kClientA)->getCounterID());
    EXPECT_TRUE(!route.getEntryForClient(kClientB)->getCounterID());
    RouteNextHopEntry nhopEntry3(
        makeNextHops({"1.1.1.1"}), EBGP_DISTANCE, kCounterID1, kClassID1);
    RouteNextHopEntry nhopEntry4(
        makeNextHops({"2.2.2.2"}), EBGP_DISTANCE, kCounterID2, kClassID2);
    route.update(kClientA, nhopEntry3);
    route.update(kClientB, nhopEntry4);
    route.setResolved(nhopEntry3);
    EXPECT_EQ(kCounterID1, route.getForwardInfo().getCounterID());
    if (auto counter = route.getEntryForClient(kClientA)->getCounterID()) {
      EXPECT_EQ(kCounterID1, *counter);
    }
    if (auto counter = route.getEntryForClient(kClientB)->getCounterID()) {
      EXPECT_EQ(kCounterID2, *counter);
    }
    EXPECT_EQ(kClassID1, route.getForwardInfo().getClassID());
    if (auto classID = route.getEntryForClient(kClientA)->getClassID()) {
      EXPECT_EQ(kClassID1, *classID);
    }
    if (auto classID = route.getEntryForClient(kClientB)->getClassID()) {
      EXPECT_EQ(kClassID2, *classID);
    }
  };
  testRouteApi(RouteV6(RouteV6::makeThrift(pfx6, kClientA, nhopEntry)));
}

TEST_F(RouteTest, dedup) {
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

  auto u2 = this->sw_->getRouteUpdater();
  u2.addRoute(
      rid,
      r1.network(),
      r1.mask(),
      kClientA,
      RouteNextHopEntry(nhop1, DISTANCE));
  u2.addRoute(
      rid,
      r2.network(),
      r2.mask(),
      kClientA,
      RouteNextHopEntry(nhop2, DISTANCE));
  u2.addRoute(
      rid,
      r3.network(),
      r3.mask(),
      kClientA,
      RouteNextHopEntry(nhop1, DISTANCE));
  u2.addRoute(
      rid,
      r4.network(),
      r4.mask(),
      kClientA,
      RouteNextHopEntry(nhop2, DISTANCE));
  u2.program();
  auto stateV2 = this->sw_->getState();
  EXPECT_NE(stateV1, stateV2);
  // Re-add the same routes; expect no change
  auto u3 = this->sw_->getRouteUpdater();
  u3.addRoute(
      rid,
      r1.network(),
      r1.mask(),
      kClientA,
      RouteNextHopEntry(nhop1, DISTANCE));
  u3.addRoute(
      rid,
      r2.network(),
      r2.mask(),
      kClientA,
      RouteNextHopEntry(nhop2, DISTANCE));
  u3.addRoute(
      rid,
      r3.network(),
      r3.mask(),
      kClientA,
      RouteNextHopEntry(nhop1, DISTANCE));
  u3.addRoute(
      rid,
      r4.network(),
      r4.mask(),
      kClientA,
      RouteNextHopEntry(nhop2, DISTANCE));
  u3.program();

  auto stateV3 = this->sw_->getState();
  EXPECT_EQ(stateV2, stateV3);
  // Re-add the same routes, except for one difference.  Expect an update.
  auto u4 = this->sw_->getRouteUpdater();
  u4.addRoute(
      rid,
      r1.network(),
      r1.mask(),
      kClientA,
      RouteNextHopEntry(nhop1, DISTANCE));
  u4.addRoute(
      rid,
      r2.network(),
      r2.mask(),
      kClientA,
      RouteNextHopEntry(nhop1, DISTANCE));
  u4.addRoute(
      rid,
      r3.network(),
      r3.mask(),
      kClientA,
      RouteNextHopEntry(nhop1, DISTANCE));
  u4.addRoute(
      rid,
      r4.network(),
      r4.mask(),
      kClientA,
      RouteNextHopEntry(nhop2, DISTANCE));
  u4.program();
  auto stateV4 = this->sw_->getState();
  EXPECT_NE(stateV4, stateV3);

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

TEST_F(RouteTest, resolve) {
  auto rid = RouterID(0);
  auto stateV1 = this->sw_->getState();
  // recursive lookup
  {
    auto u1 = this->sw_->getRouteUpdater();
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
    auto u1 = this->sw_->getRouteUpdater();
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

    auto verifyPrefix = [&](std::string prefixStr) {
      auto route = this->findRoute4(stateV2, rid, prefixStr);
      // In standalone RIB, unresolved routes never make it to FIB
      EXPECT_EQ(route, nullptr);
    };
    verifyPrefix("10.0.0.0/8");
    verifyPrefix("20.0.0.0/8");
    verifyPrefix("30.0.0.0/8");
  }
  // recursive lookup across 2 updates
  {
    auto u1 = this->sw_->getRouteUpdater();
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
    auto u2 = this->sw_->getRouteUpdater();
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
        this->sw_->getRib(), rid, r31NextHops.begin()->addr().asV4(), stateV3);
    EXPECT_RESOLVED(r32);
    EXPECT_TRUE(r32->isConnected());

    // 50.0.0.0/8 should be resolved
    auto r33 = this->findRoute4(stateV3, rid, "50.0.0.0/8");
    EXPECT_RESOLVED(r33);
    EXPECT_FALSE(r33->isConnected());
  }
}

TEST_F(RouteTest, resolveDropToCPUMix) {
  auto rid = RouterID(0);

  // add a DROP route and a ToCPU route
  auto u1 = this->sw_->getRouteUpdater();
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
  auto u2 = this->sw_->getRouteUpdater();
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
  auto u3 = this->sw_->getRouteUpdater();
  RouteNextHopSet nhops3 = makeNextHops({"11.1.1.10"}); // DROP
  u3.addRoute(
      rid,
      IPAddress("8.8.8.0"),
      24,
      kClientA,
      RouteNextHopEntry(nhops3, DISTANCE));
  u3.program();
  auto stateV4 = this->sw_->getState();
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
TEST_F(RouteTest, addDel) {
  auto rid = RouterID(0);

  RouteNextHopSet nexthops = makeNextHops(
      {"1.1.1.10", // intf 1
       "2::2", // intf 2
       "1.1.2.10"}); // Drop (via default null route)
  RouteNextHopSet nexthops2 = makeNextHops(
      {"1.1.3.10", // Drop (via default null route)
       "11:11::1"}); // Drop (via default null route)

  auto u1 = this->sw_->getRouteUpdater();
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
  auto u2 = this->sw_->getRouteUpdater();
  u2.addRoute(
      rid,
      IPAddress("10.1.1.1"),
      24,
      kClientA,
      RouteNextHopEntry(nexthops2, DISTANCE));
  u2.program();
  auto stateV3 = this->sw_->getState();

  auto r3 = this->findRoute4(stateV3, rid, "10.1.1.0/24");
  ASSERT_NE(nullptr, r3);
  EXPECT_TRUE(r3->isResolved()); // Resolved to default NULL
  EXPECT_TRUE(r3->isDrop());
  EXPECT_FALSE(r3->isConnected());
  EXPECT_FALSE(r3->needResolve());

  // re-add the same route does not cause change
  auto u3 = this->sw_->getRouteUpdater();
  u3.addRoute(
      rid,
      IPAddress("10.1.1.1"),
      24,
      kClientA,
      RouteNextHopEntry(nexthops2, DISTANCE));
  u3.program();
  EXPECT_EQ(stateV3, this->sw_->getState());

  // now delete the V4 route
  auto u4 = this->sw_->getRouteUpdater();
  u4.delRoute(rid, IPAddress("10.1.1.1"), 24, kClientA);
  u4.program();

  auto r5 = this->findRoute4(this->sw_->getState(), rid, "10.1.1.0/24");
  EXPECT_EQ(nullptr, r5);

  // change an old route to punt to CPU, add a new route to DROP
  auto u5 = this->sw_->getRouteUpdater();
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

TEST_F(RouteTest, InterfaceRoutes) {
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
  config.interfaces()[1].ipAddresses()[0] = "1.1.1.1/24";
  config.interfaces()[1].ipAddresses()[1] = "1::1/48";
  config.interfaces()[0].ipAddresses()[0] = "2.2.2.2/24";
  config.interfaces()[0].ipAddresses()[1] = "2::1/48";
  this->sw_->applyConfig("New config", config);
  auto stateV2 = this->sw_->getState();
  EXPECT_NE(stateV1, stateV2);
  // With standalone rib config application will cause us to first
  // blow away all the interface and static routes and then add them
  // back based on new config. So generation will set back to 0
  auto expectedGen = 0;
  // verify the ipv4 route
  {
    auto rt = this->findRoute4(stateV2, rid, "1.1.1.0/24");
    EXPECT_EQ(expectedGen, rt->getGeneration());
    EXPECT_FWD_INFO(rt, InterfaceID(2), "1.1.1.1");
  }
  // verify the ipv6 route
  {
    auto rt = this->findRoute6(stateV2, rid, "2::0/48");
    EXPECT_EQ(expectedGen, rt->getGeneration());
    EXPECT_FWD_INFO(rt, InterfaceID(1), "2::1");
  }
}

TEST_F(RouteTest, dropRoutes) {
  auto rid = RouterID(0);
  auto u1 = this->sw_->getRouteUpdater();
  u1.addRoute(
      rid,
      IPAddress("10.10.10.10"),
      32,
      kClientA,
      RouteNextHopEntry(RouteForwardAction::DROP, DISTANCE));
  u1.addRoute(
      rid,
      IPAddress("2001::0"),
      128,
      kClientA,
      RouteNextHopEntry(RouteForwardAction::DROP, DISTANCE));
  // Check recursive resolution for drop routes
  RouteNextHopSet v4nexthops = makeNextHops({"10.10.10.10"});
  u1.addRoute(
      rid,
      IPAddress("20.20.20.0"),
      24,
      kClientA,
      RouteNextHopEntry(v4nexthops, DISTANCE));
  RouteNextHopSet v6nexthops = makeNextHops({"2001::0"});
  u1.addRoute(
      rid,
      IPAddress("2001:1::"),
      64,
      kClientA,
      RouteNextHopEntry(v6nexthops, DISTANCE));

  u1.program();
  auto state2 = this->sw_->getState();

  // Check routes
  auto r1 = this->findRoute4(state2, rid, "10.10.10.10/32");
  EXPECT_RESOLVED(r1);
  EXPECT_FALSE(r1->isConnected());
  EXPECT_TRUE(
      r1->has(kClientA, RouteNextHopEntry(RouteForwardAction::DROP, DISTANCE)));
  EXPECT_EQ(
      r1->getForwardInfo(),
      RouteNextHopEntry(RouteForwardAction::DROP, DISTANCE));

  auto r2 = this->findRoute4(state2, rid, "20.20.20.0/24");
  EXPECT_RESOLVED(r2);
  EXPECT_FALSE(r2->isConnected());
  EXPECT_FALSE(
      r2->has(kClientA, RouteNextHopEntry(RouteForwardAction::DROP, DISTANCE)));
  EXPECT_EQ(
      r2->getForwardInfo(),
      RouteNextHopEntry(RouteForwardAction::DROP, DISTANCE));

  auto r3 = this->findRoute6(state2, rid, "2001::0/128");
  EXPECT_RESOLVED(r3);
  EXPECT_FALSE(r3->isConnected());
  EXPECT_TRUE(
      r3->has(kClientA, RouteNextHopEntry(RouteForwardAction::DROP, DISTANCE)));
  EXPECT_EQ(
      r3->getForwardInfo(),
      RouteNextHopEntry(RouteForwardAction::DROP, DISTANCE));

  auto r4 = this->findRoute6(state2, rid, "2001:1::/64");
  EXPECT_RESOLVED(r4);
  EXPECT_FALSE(r4->isConnected());
  EXPECT_EQ(
      r4->getForwardInfo(),
      RouteNextHopEntry(RouteForwardAction::DROP, DISTANCE));
}

TEST_F(RouteTest, toCPURoutes) {
  auto rid = RouterID(0);
  auto u1 = this->sw_->getRouteUpdater();
  u1.addRoute(
      rid,
      IPAddress("10.10.10.10"),
      32,
      kClientA,
      RouteNextHopEntry(RouteForwardAction::TO_CPU, DISTANCE));
  u1.addRoute(
      rid,
      IPAddress("2001::0"),
      128,
      kClientA,
      RouteNextHopEntry(RouteForwardAction::TO_CPU, DISTANCE));
  // Check recursive resolution for to_cpu routes
  RouteNextHopSet v4nexthops = makeNextHops({"10.10.10.10"});
  u1.addRoute(
      rid,
      IPAddress("20.20.20.0"),
      24,
      kClientA,
      RouteNextHopEntry(v4nexthops, DISTANCE));
  RouteNextHopSet v6nexthops = makeNextHops({"2001::0"});
  u1.addRoute(
      rid,
      IPAddress("2001:1::"),
      64,
      kClientA,
      RouteNextHopEntry(v6nexthops, DISTANCE));

  u1.program();
  auto state2 = this->sw_->getState();

  // Check routes
  auto r1 = this->findRoute4(state2, rid, "10.10.10.10/32");
  EXPECT_RESOLVED(r1);
  EXPECT_FALSE(r1->isConnected());
  EXPECT_TRUE(r1->has(
      kClientA, RouteNextHopEntry(RouteForwardAction::TO_CPU, DISTANCE)));
  EXPECT_EQ(
      r1->getForwardInfo(),
      RouteNextHopEntry(RouteForwardAction::TO_CPU, DISTANCE));

  auto r2 = this->findRoute4(state2, rid, "20.20.20.0/24");
  EXPECT_RESOLVED(r2);
  EXPECT_FALSE(r2->isConnected());
  EXPECT_EQ(
      r2->getForwardInfo(),
      RouteNextHopEntry(RouteForwardAction::TO_CPU, DISTANCE));

  auto r3 = this->findRoute6(state2, rid, "2001::0/128");
  EXPECT_RESOLVED(r3);
  EXPECT_FALSE(r3->isConnected());
  EXPECT_TRUE(r3->has(
      kClientA, RouteNextHopEntry(RouteForwardAction::TO_CPU, DISTANCE)));
  EXPECT_EQ(
      r3->getForwardInfo(),
      RouteNextHopEntry(RouteForwardAction::TO_CPU, DISTANCE));

  auto r5 = this->findRoute6(state2, rid, "2001:1::/64");
  EXPECT_RESOLVED(r5);
  EXPECT_FALSE(r5->isConnected());
  EXPECT_EQ(
      r5->getForwardInfo(),
      RouteNextHopEntry(RouteForwardAction::TO_CPU, DISTANCE));
}

namespace TEMP {
struct Route {
  uint32_t vrf;
  IPAddress prefix;
  uint8_t len;
  Route(uint32_t vrf, IPAddress prefix, uint8_t len)
      : vrf(vrf), prefix(prefix), len(len) {}
  bool operator<(const Route& rt) const {
    return std::tie(vrf, len, prefix) < std::tie(rt.vrf, rt.len, rt.prefix);
  }
  bool operator==(const Route& rt) const {
    return vrf == rt.vrf && len == rt.len && prefix == rt.prefix;
  }
};
} // namespace TEMP

void checkChangedRoute(
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
      delta,
      [&](RouterID id, const auto& oldRt, const auto& newRt) {
        EXPECT_EQ(oldRt->prefix(), newRt->prefix());
        EXPECT_NE(oldRt, newRt);
        const auto prefix = newRt->prefix();
        auto ret = foundChanged.insert(
            TEMP::Route(id, IPAddress(prefix.network()), prefix.mask()));
        EXPECT_TRUE(ret.second);
      },
      [&](RouterID id, const auto& rt) {
        const auto prefix = rt->prefix();
        auto ret = foundAdded.insert(
            TEMP::Route(id, IPAddress(prefix.network()), prefix.mask()));
        EXPECT_TRUE(ret.second);
      },
      [&](RouterID id, const auto& rt) {
        const auto prefix = rt->prefix();
        auto ret = foundRemoved.insert(
            TEMP::Route(id, IPAddress(prefix.network()), prefix.mask()));
        EXPECT_TRUE(ret.second);
      });

  EXPECT_EQ(changedIDs, foundChanged);
  EXPECT_EQ(addedIDs, foundAdded);
  EXPECT_EQ(removedIDs, foundRemoved);
}

TEST_F(RouteTest, applyNewConfig) {
  auto config = this->initialConfig();
  config.vlans()->resize(6);
  config.vlans()[4].id() = 5;
  config.vlans()[5].id() = 6;
  config.interfaces()->resize(6);
  config.interfaces()[4].intfID() = 5;
  config.interfaces()[4].vlanID() = 5;
  config.interfaces()[4].routerID() = 0;
  config.interfaces()[4].mac() = "00:00:00:00:00:55";
  config.interfaces()[4].ipAddresses()->resize(4);
  config.interfaces()[4].ipAddresses()[0] = "5.1.1.1/24";
  config.interfaces()[4].ipAddresses()[1] = "5.1.1.2/24";
  config.interfaces()[4].ipAddresses()[2] = "5.1.1.10/24";
  config.interfaces()[4].ipAddresses()[3] = "::1/48";
  config.interfaces()[5].intfID() = 6;
  config.interfaces()[5].vlanID() = 6;
  config.interfaces()[5].routerID() = 1;
  config.interfaces()[5].mac() = "00:00:00:00:00:66";
  config.interfaces()[5].ipAddresses()->resize(2);
  config.interfaces()[5].ipAddresses()[0] = "5.1.1.1/24";
  config.interfaces()[5].ipAddresses()[1] = "::1/48";

  auto rib = this->sw_->getRib();
  auto stateV0 = this->sw_->getState();
  this->sw_->applyConfig("Apply config1", config);
  auto stateV1 = this->sw_->getState();
  ASSERT_NE(nullptr, stateV1);

  checkChangedRoute(
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
  config.interfaces()[4].ipAddresses()[3] = "11::11/48";

  this->sw_->applyConfig("Apply config2", config);
  auto stateV2 = this->sw_->getState();
  ASSERT_NE(nullptr, stateV2);

  checkChangedRoute(
      stateV1,
      stateV2,
      {},
      {TEMP::Route{0, IPAddress("11::0"), 48}},
      {TEMP::Route{0, IPAddress("::0"), 48}});

  // move one interface to cause same route prefix conflict
  config.interfaces()[5].routerID() = 0;
  EXPECT_THROW(this->sw_->applyConfig("Bogus config", config), FbossError);

  // add a new interface in a new VRF
  config.vlans()->resize(7);
  config.vlans()[6].id() = 7;
  config.interfaces()->resize(7);
  config.interfaces()[6].intfID() = 7;
  config.interfaces()[6].vlanID() = 7;
  config.interfaces()[6].routerID() = 2;
  config.interfaces()[6].mac() = "00:00:00:00:00:77";
  config.interfaces()[6].ipAddresses()->resize(2);
  config.interfaces()[6].ipAddresses()[0] = "1.1.1.1/24";
  config.interfaces()[6].ipAddresses()[1] = "::1/48";
  // and move one interface to another vrf and fix the address conflict
  config.interfaces()[5].routerID() = 0;
  config.interfaces()[5].ipAddresses()->resize(2);
  config.interfaces()[5].ipAddresses()[0] = "5.2.2.1/24";
  config.interfaces()[5].ipAddresses()[1] = "5::6/48";

  this->sw_->applyConfig("Apply config", config);
  auto stateV3 = this->sw_->getState();
  ASSERT_NE(nullptr, stateV3);
  checkChangedRoute(
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
  this->sw_->applyConfig("Apply config", config);
  auto stateV4 = this->sw_->getState();
  ASSERT_NE(nullptr, stateV4);
  if (stateV4) {
    // FIXME - reapplying the same config on standalone rib should yield null,
    // but it does result in new state. The state delta is empty though, so
    // no change will be programmed down
    checkChangedRoute(stateV3, stateV4, {}, {}, {});
  }
}

TEST_F(RouteTest, changedRoutesPostUpdate) {
  auto rid = RouterID(0);
  RouteNextHopSet nexthops = makeNextHops(
      {"1.1.1.10", // resolved by intf 1
       "2::2"}); // resolved by intf 2

  auto state1 = this->sw_->getState();
  // Add a couple of routes
  auto u1 = this->sw_->getRouteUpdater();
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
      state1,
      state2,
      {},
      {
          TEMP::Route{0, IPAddress("10.1.1.0"), 24},
          TEMP::Route{0, IPAddress("2001::0"), 48},
      },
      {});
  // Add 2 more routes
  auto u2 = this->sw_->getRouteUpdater();
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
      state2,
      state3,
      {},
      {
          TEMP::Route{0, IPAddress("10.10.1.0"), 24},
          TEMP::Route{0, IPAddress("2001:10::"), 48},
      },
      {});
}

TEST_F(RouteTest, PruneAddedRoutes) {
  auto rid0 = RouterID(0);
  auto updater = this->sw_->getRouteUpdater();
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

  auto newRouteEntry = findRoute<IPAddressV4>(
      rid0, prefix1.toCidrNetwork(), this->sw_->getState());
  EXPECT_NE(nullptr, newRouteEntry);
  EXPECT_EQ(
      newRouteEntry->prefix(), (RouteV4::Prefix{IPAddressV4("20.0.1.0"), 24}));

  EXPECT_TRUE(newRouteEntry->isPublished());
  auto revertState = this->sw_->getState();
  SwitchState::revertNewRouteEntry<IPAddressV4>(
      rid0, newRouteEntry, nullptr, &revertState);
  // Make sure that state3 changes as a result of pruning
  auto remainingRouteEntry =
      findRoute<IPAddressV4>(rid0, prefix1.toCidrNetwork(), revertState);
  // Route removed after delete
  EXPECT_EQ(nullptr, remainingRouteEntry);
}

// Test that pruning of changed routes happens correctly.
TEST_F(RouteTest, PruneChangedRoutes) {
  // Add routes
  // Change one of them
  // Prune the changed one
  // Check that the pruning happened correctly
  // state1
  //  ... Add route for prefix41
  //  ... Add route for prefix42 (RouteForwardAction::TO_CPU)
  // state2
  auto rid0 = RouterID(0);
  auto updater = this->sw_->getRouteUpdater();

  RouteV4::Prefix prefix41{IPAddressV4("20.0.21.41"), 32};
  auto nexthops41 = makeNextHops({"10.0.0.1", "face:b00c:0:21::41"});
  updater.addRoute(
      rid0,
      prefix41.network(),
      prefix41.mask(),
      kClientA,
      RouteNextHopEntry(nexthops41, DISTANCE));

  RouteV6::Prefix prefix42{IPAddressV6("facf:b00c:0:21::42"), 96};
  updater.addRoute(
      rid0,
      prefix42.network(),
      prefix42.mask(),
      kClientA,
      RouteNextHopEntry(RouteForwardAction::TO_CPU, DISTANCE));

  updater.program();

  auto oldEntry = findRoute<IPAddressV6>(
      rid0, prefix42.toCidrNetwork(), this->sw_->getState());
  EXPECT_NE(nullptr, oldEntry);
  EXPECT_TRUE(oldEntry->isToCPU());

  // state2
  //  ... Make route for prefix42 resolve to actual nexthops
  // state3

  auto nexthops42 = makeNextHops({"10.0.0.1", "face:b00c:0:21::42"});
  updater.addRoute(
      rid0,
      prefix42.network(),
      prefix42.mask(),
      kClientA,
      RouteNextHopEntry(nexthops42, DISTANCE));
  updater.program();

  auto newEntry = findRoute<IPAddressV6>(
      rid0, prefix42.toCidrNetwork(), this->sw_->getState());

  EXPECT_FALSE(newEntry->isToCPU());
  // state3
  //  ... revert route for prefix42
  // state4
  auto revertState = this->sw_->getState();
  SwitchState::revertNewRouteEntry(rid0, newEntry, oldEntry, &revertState);

  auto revertedEntry =
      findRoute<IPAddressV6>(rid0, prefix42.toCidrNetwork(), revertState);
  EXPECT_TRUE(revertedEntry->isToCPU());
}

// Test adding and deleting per-client nexthops lists
TEST_F(RouteTest, modRoutes) {
  auto u1 = this->sw_->getRouteUpdater();

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

  auto u2 = this->sw_->getRouteUpdater();
  u2.delRoute(kRid0, IPAddress("10.10.10.10"), 32, kClientA);
  u2.program();
  // 10.10.10.10 should now get clientBs nhops
  rt10 = this->findRoute4(this->sw_->getState(), kRid0, prefix10);
  rt99 = this->findRoute4(this->sw_->getState(), kRid0, prefix99);
  EXPECT_EQ(rt10->getForwardInfo().getNextHopSet(), nexthops2);
  EXPECT_EQ(rt99->getForwardInfo().getNextHopSet(), nexthops3);
  // Delete the second client/nexthop pair
  // The route & prefix should disappear altogether.

  auto u3 = this->sw_->getRouteUpdater();
  u3.delRoute(kRid0, IPAddress("10.10.10.10"), 32, kClientB);
  u3.program();
  rt10 = this->findRoute4(this->sw_->getState(), kRid0, prefix10);
  rt99 = this->findRoute4(this->sw_->getState(), kRid0, prefix99);
  EXPECT_EQ(rt10, nullptr);
  EXPECT_EQ(rt99->getForwardInfo().getNextHopSet(), nexthops3);
}

// Test adding empty nextHops lists
TEST_F(RouteTest, disallowEmptyNexthops) {
  auto u1 = this->sw_->getRouteUpdater();

  // It's illegal to add an empty nextHops list to a route

  // Test the case where the empty list is the first to be added to the Route
  EXPECT_THROW(
      u1.addRoute(
          kRid0,
          IPAddress("5.5.5.5"),
          32,
          kClientA,
          RouteNextHopEntry(newNextHops(0, "20.20.20."), DISTANCE)),
      FbossError);

  // Test the case where the empty list is the second to be added to the Route
  u1.addRoute(
      kRid0,
      IPAddress("10.10.10.10"),
      32,
      kClientA,
      RouteNextHopEntry(newNextHops(3, "10.10.10."), DISTANCE));
  EXPECT_THROW(
      u1.addRoute(
          kRid0,
          IPAddress("10.10.10.10"),
          32,
          kClientB,
          RouteNextHopEntry(newNextHops(0, "20.20.20."), DISTANCE)),
      FbossError);
}

bool stringStartsWith(std::string s1, std::string prefix) {
  return s1.compare(0, prefix.size(), prefix) == 0;
}

void expectFwdInfo(
    const SwSwitch* sw,
    RouteV4::Prefix prefix,
    std::string ipPrefix) {
  const auto& fwd =
      findRoute<IPAddressV4>(kRid0, prefix.toCidrNetwork(), sw->getState())
          ->getForwardInfo();
  const auto& nhops = fwd.getNextHopSet();
  // Expect the fwd'ing info to be 3 IPs all starting with 'ipPrefix'
  EXPECT_EQ(3, nhops.size());
  for (auto const& it : nhops) {
    EXPECT_TRUE(stringStartsWith(it.addr().str(), ipPrefix));
  }
}

void addNextHopsForClient(
    SwSwitch* sw,
    RouteV4::Prefix prefix,
    ClientID clientId,
    std::string ipPrefix,
    AdminDistance adminDistance = AdminDistance::MAX_ADMIN_DISTANCE) {
  auto u = sw->getRouteUpdater();
  u.addRoute(
      kRid0,
      prefix.network(),
      prefix.mask(),
      clientId,
      RouteNextHopEntry(newNextHops(3, ipPrefix), adminDistance));
  u.program();
}

void deleteNextHopsForClient(
    SwSwitch* sw,
    RouteV4::Prefix prefix,
    ClientID clientId) {
  auto u = sw->getRouteUpdater();
  u.delRoute(kRid0, prefix.network(), prefix.mask(), clientId);
  u.program();
}
// Add and remove per-client NextHop lists to the same route, and make sure
// the lowest admin distance is the one that determines the forwarding info
TEST_F(RouteTest, fwdInfoRanking) {
  // We'll be adding and removing a bunch of nexthops for this Network & Mask
  auto network = IPAddressV4("22.22.22.22");
  uint8_t mask = 32;
  RouteV4::Prefix prefix{network, mask};

  // Add client 30, plus an interface for resolution.
  auto u1 = this->sw_->getRouteUpdater();
  // This is the route all the others will resolve to.
  u1.addRoute(
      kRid0,
      IPAddress("10.10.0.0"),
      16,
      ClientID::INTERFACE_ROUTE,
      RouteNextHopEntry(
          static_cast<NextHop>(ResolvedNextHop(
              IPAddress("1.1.1.1"), InterfaceID(1), UCMP_DEFAULT_WEIGHT)),
          AdminDistance::DIRECTLY_CONNECTED));
  u1.addRoute(
      kRid0,
      network,
      mask,
      ClientID(30),
      RouteNextHopEntry(newNextHops(3, "10.10.30."), DISTANCE));
  u1.program();

  // Expect fwdInfo based on client 30
  expectFwdInfo(this->sw_, prefix, "10.10.30.");
  // Add client BGP
  addNextHopsForClient(
      this->sw_, prefix, ClientID::BGPD, "10.10.20.", AdminDistance::EBGP);

  // Expect fwdInfo based on client BGP
  expectFwdInfo(this->sw_, prefix, "10.10.20.");
  // Add client 40
  addNextHopsForClient(this->sw_, prefix, ClientID(40), "10.10.40.");

  // Expect fwdInfo still based on client BGP
  expectFwdInfo(this->sw_, prefix, "10.10.20.");
  // Add client STATIC_ROUTE
  addNextHopsForClient(
      this->sw_,
      prefix,
      ClientID::STATIC_ROUTE,
      "10.10.10.",
      AdminDistance::STATIC_ROUTE);

  // Expect fwdInfo based on client STATIC_ROUTE
  expectFwdInfo(this->sw_, prefix, "10.10.10.");
  // Remove client BGP
  deleteNextHopsForClient(this->sw_, prefix, ClientID::BGPD);

  // Winner should still be STATIC_ROUTE
  expectFwdInfo(this->sw_, prefix, "10.10.10.");

  // Remove client STATIC_ROUTE
  deleteNextHopsForClient(this->sw_, prefix, ClientID::STATIC_ROUTE);

  // Winner should now be 30
  expectFwdInfo(this->sw_, prefix, "10.10.30.");
  // Remove client 30
  deleteNextHopsForClient(this->sw_, prefix, ClientID(30));

  // Winner should now be 40
  expectFwdInfo(this->sw_, prefix, "10.10.40.");
}

TEST_F(RouteTest, StaticIp2MplsRoutes) {
  auto config = this->initialConfig();

  config.staticIp2MplsRoutes()->resize(2);
  config.staticIp2MplsRoutes()[0].prefix() = "10.0.0.0/24";
  config.staticIp2MplsRoutes()[0].nexthops()->resize(1);
  NextHopThrift v4Nexthop;
  v4Nexthop.address() = toBinaryAddress(folly::IPAddress("1.1.1.10"));
  MplsAction action;
  action.action() = MplsActionCode::PUSH;
  action.pushLabels() = {101, 102};
  v4Nexthop.mplsAction() = action;
  config.staticIp2MplsRoutes()[0].nexthops()[0] = v4Nexthop;

  config.staticIp2MplsRoutes()[1].prefix() = "1:1::/64";
  config.staticIp2MplsRoutes()[1].nexthops()->resize(1);
  NextHopThrift v6Nexthop;
  v6Nexthop.address() = toBinaryAddress(folly::IPAddress("1::10"));
  v6Nexthop.mplsAction() = action;
  config.staticIp2MplsRoutes()[1].nexthops()[0] = v6Nexthop;

  this->sw_->applyConfig("New config", config);

  auto state = this->sw_->getState();
  auto v4Route = this->findRoute4(state, RouterID(0), "10.0.0.0/24");
  EXPECT_RESOLVED(v4Route);
  EXPECT_FALSE(v4Route->isDrop());
  EXPECT_FALSE(v4Route->isToCPU());
  EXPECT_FALSE(v4Route->isConnected());

  const auto& v4Fwd = v4Route->getForwardInfo();
  EXPECT_EQ(RouteForwardAction::NEXTHOPS, v4Fwd.getAction());
  EXPECT_EQ(1, v4Fwd.getNextHopSet().size());
  for (auto& nexthop : v4Fwd.getNextHopSet()) {
    auto action = nexthop.labelForwardingAction();
    EXPECT_TRUE(action.has_value());
    EXPECT_EQ(action->type(), MplsActionCode::PUSH);
    EXPECT_EQ(action->pushStack(), MplsLabelStack({101, 102}));
  }

  // v6 route
  auto v6Route = this->findRoute6(state, RouterID(0), "1:1::/64");
  EXPECT_RESOLVED(v6Route);
  EXPECT_FALSE(v6Route->isDrop());
  EXPECT_FALSE(v6Route->isToCPU());
  EXPECT_FALSE(v6Route->isConnected());

  const auto& v6Fwd = v4Route->getForwardInfo();
  EXPECT_EQ(RouteForwardAction::NEXTHOPS, v6Fwd.getAction());
  EXPECT_EQ(1, v6Fwd.getNextHopSet().size());

  for (auto& nexthop : v6Fwd.getNextHopSet()) {
    auto action = nexthop.labelForwardingAction();
    EXPECT_TRUE(action.has_value());
    EXPECT_EQ(action->type(), MplsActionCode::PUSH);
    EXPECT_EQ(action->pushStack(), MplsLabelStack({101, 102}));
  }
}

// Test interface routes when we have more than one address per
// address family in an interface
class MultipleAddressInterfaceTest : public RouteTest {
 public:
  cfg::SwitchConfig initialConfig() const override {
    cfg::SwitchConfig config;
    config.vlans()->resize(1);
    config.vlans()[0].id() = 1;

    config.interfaces()->resize(1);
    config.interfaces()[0].intfID() = 1;
    config.interfaces()[0].vlanID() = 1;
    config.interfaces()[0].routerID() = 0;
    config.interfaces()[0].mac() = "00:00:00:00:00:11";
    config.interfaces()[0].ipAddresses()->resize(4);
    config.interfaces()[0].ipAddresses()[0] = "1.1.1.1/24";
    config.interfaces()[0].ipAddresses()[1] = "1.1.1.2/24";
    config.interfaces()[0].ipAddresses()[2] = "1::1/48";
    config.interfaces()[0].ipAddresses()[3] = "1::2/48";
    return config;
  }
};

TEST_F(MultipleAddressInterfaceTest, twoAddrsForInterface) {
  auto rid = RouterID(0);
  auto [v4Routes, v6Routes] = getRouteCount(this->sw_->getState());

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

TEST_F(RouteTest, withLabelForwardingAction) {
  auto rid = RouterID(0);

  std::array<folly::IPAddressV4, 4> nextHopAddrs{
      folly::IPAddressV4("1.1.1.0"),
      folly::IPAddressV4("1.1.2.0"),
      folly::IPAddressV4("1.1.3.0"),
      folly::IPAddressV4("1.1.4.0"),
  };

  std::array<LabelForwardingAction::LabelStack, 4> nextHopStacks{
      LabelForwardingAction::LabelStack({101, 201, 301}),
      LabelForwardingAction::LabelStack({102, 202, 302}),
      LabelForwardingAction::LabelStack({103, 203, 303}),
      LabelForwardingAction::LabelStack({104, 204, 304}),
  };

  std::map<folly::IPAddressV4, LabelForwardingAction::LabelStack>
      labeledNextHops;
  RouteNextHopSet nexthops;

  for (auto i = 0; i < 4; i++) {
    labeledNextHops.emplace(std::make_pair(nextHopAddrs[i], nextHopStacks[i]));
    nexthops.emplace(UnresolvedNextHop(
        nextHopAddrs[i],
        1,
        std::make_optional<LabelForwardingAction>(
            LabelForwardingAction::LabelForwardingType::PUSH,
            nextHopStacks[i])));
  }

  auto state = this->sw_->getState();

  auto updater = this->sw_->getRouteUpdater();
  updater.addRoute(
      rid,
      folly::IPAddressV4("1.1.0.0"),
      16,
      kClientA,
      RouteNextHopEntry(nexthops, DISTANCE));

  updater.program();
  const auto& route = findLongestMatchRoute(
      this->sw_->getRib(),
      rid,
      folly::IPAddressV4("1.1.2.2"),
      this->sw_->getState());

  EXPECT_EQ(route->has(kClientA, RouteNextHopEntry(nexthops, DISTANCE)), true);
  auto entry = route->getBestEntry();
  for (const auto& nh : entry.second->getNextHopSet()) {
    EXPECT_EQ(nh.labelForwardingAction().has_value(), true);
    EXPECT_EQ(
        nh.labelForwardingAction()->type(),
        LabelForwardingAction::LabelForwardingType::PUSH);
    EXPECT_EQ(nh.labelForwardingAction()->pushStack().has_value(), true);
    EXPECT_EQ(nh.labelForwardingAction()->pushStack()->size(), 3);
    EXPECT_EQ(
        labeledNextHops[nh.addr().asV4()],
        nh.labelForwardingAction()->pushStack().value());
  }
}

TEST_F(RouteTest, unresolvedWithRouteLabels) {
  auto rid = RouterID(0);
  RouteNextHopSet bgpNextHops;
  RouteNextHopSet bgpResolvedNextHops;

  for (auto i = 0; i < 4; i++) {
    // bgp next hops with some labels, for some prefix
    bgpNextHops.emplace(UnresolvedNextHop(
        kBgpNextHopAddrs[i],
        1,
        std::make_optional<LabelForwardingAction>(
            LabelForwardingAction::LabelForwardingType::PUSH,
            LabelForwardingAction::LabelStack{
                kLabelStacks[i].begin(), kLabelStacks[i].end()})));
  }
  auto updater = this->sw_->getRouteUpdater();
  // routes to remote prefix to bgp next hops
  updater.addRoute(
      rid,
      kDestPrefix.network(),
      kDestPrefix.mask(),
      ClientID::BGPD,
      RouteNextHopEntry(bgpNextHops, DISTANCE));
  updater.program();

  // route to remote destination under kDestPrefix advertised by bgp
  const auto& route = findLongestMatchRoute(
      this->sw_->getRib(), rid, kDestAddress, this->sw_->getState());

  EXPECT_EQ(
      route->has(
          ClientID::BGPD, RouteNextHopEntry(bgpNextHops, AdminDistance::EBGP)),
      true);

  // Will resolve to DROP null routes
  EXPECT_TRUE(route->isDrop());
}

TEST_F(RouteTest, withTunnelAndRouteLabels) {
  auto rid = RouterID(0);
  RouteNextHopSet bgpNextHops;

  for (auto i = 0; i < 4; i++) {
    // bgp next hops with some labels, for some prefix
    bgpNextHops.emplace(UnresolvedNextHop(
        kBgpNextHopAddrs[i],
        1,
        std::make_optional<LabelForwardingAction>(
            LabelForwardingAction::LabelForwardingType::PUSH,
            LabelForwardingAction::LabelStack{
                kLabelStacks[i].begin(), kLabelStacks[i].begin() + 2})));
  }

  auto updater = this->sw_->getRouteUpdater();
  // routes to remote prefix to bgp next hops
  updater.addRoute(
      rid,
      kDestPrefix.network(),
      kDestPrefix.mask(),
      ClientID::BGPD,
      RouteNextHopEntry(bgpNextHops, DISTANCE));

  std::vector<ResolvedNextHop> igpNextHops;
  for (auto i = 0; i < 4; i++) {
    igpNextHops.push_back(ResolvedNextHop(
        kIgpAddrs[i],
        kInterfaces[i],
        ECMP_WEIGHT,
        LabelForwardingAction(
            LabelForwardingAction::LabelForwardingType::PUSH,
            LabelForwardingAction::LabelStack{
                kLabelStacks[i].begin() + 2, kLabelStacks[i].begin() + 3})));
  }

  // igp routes to bgp nexthops,
  for (auto i = 0; i < 4; i++) {
    updater.addRoute(
        rid,
        kBgpNextHopAddrs[i],
        64,
        ClientID::OPENR,
        RouteNextHopEntry(
            static_cast<NextHop>(igpNextHops[i]),
            AdminDistance::DIRECTLY_CONNECTED));
  }

  updater.program();

  // route to remote destination under kDestPrefix advertised by bgp

  const auto& route = findLongestMatchRoute(
      this->sw_->getRib(), rid, kDestAddress, this->sw_->getState());

  EXPECT_EQ(
      route->has(
          ClientID::BGPD, RouteNextHopEntry(bgpNextHops, AdminDistance::EBGP)),
      true);

  EXPECT_TRUE(route->isResolved());

  for (const auto& nhop : route->getForwardInfo().getNextHopSet()) {
    EXPECT_TRUE(nhop.isResolved());
    EXPECT_TRUE(nhop.labelForwardingAction().has_value());
    EXPECT_EQ(
        nhop.labelForwardingAction()->type(),
        LabelForwardingAction::LabelForwardingType::PUSH);

    ASSERT_TRUE(
        nhop.intf() == kInterfaces[0] || nhop.intf() == kInterfaces[1] ||
        nhop.intf() == kInterfaces[2] || nhop.intf() == kInterfaces[3]);

    if (nhop.intf() == kInterfaces[0]) {
      EXPECT_EQ(
          nhop.labelForwardingAction()->pushStack().value(), kLabelStacks[0]);
    } else if (nhop.intf() == kInterfaces[1]) {
      EXPECT_EQ(
          nhop.labelForwardingAction()->pushStack().value(), kLabelStacks[1]);
    } else if (nhop.intf() == kInterfaces[2]) {
      EXPECT_EQ(
          nhop.labelForwardingAction()->pushStack().value(), kLabelStacks[2]);
    } else if (nhop.intf() == kInterfaces[3]) {
      EXPECT_EQ(
          nhop.labelForwardingAction()->pushStack().value(), kLabelStacks[3]);
    }
  }
}

TEST_F(RouteTest, withOnlyTunnelLabels) {
  auto rid = RouterID(0);
  RouteNextHopSet bgpNextHops;

  for (auto i = 0; i < 4; i++) {
    // bgp next hops with some labels, for some prefix
    bgpNextHops.emplace(UnresolvedNextHop(kBgpNextHopAddrs[i], 1));
  }

  auto updater = this->sw_->getRouteUpdater();
  // routes to remote prefix to bgp next hops
  updater.addRoute(
      rid,
      kDestPrefix.network(),
      kDestPrefix.mask(),
      ClientID::BGPD,
      RouteNextHopEntry(bgpNextHops, DISTANCE));

  std::vector<ResolvedNextHop> igpNextHops;
  for (auto i = 0; i < 4; i++) {
    igpNextHops.push_back(ResolvedNextHop(
        kIgpAddrs[i],
        kInterfaces[i],
        ECMP_WEIGHT,
        LabelForwardingAction(
            LabelForwardingAction::LabelForwardingType::PUSH,
            LabelForwardingAction::LabelStack{
                kLabelStacks[i].begin(), kLabelStacks[i].end()})));
  }

  // igp routes to bgp nexthops,
  for (auto i = 0; i < 4; i++) {
    updater.addRoute(
        rid,
        kBgpNextHopAddrs[i],
        64,
        ClientID::OPENR,
        RouteNextHopEntry(
            static_cast<NextHop>(igpNextHops[i]),
            AdminDistance::DIRECTLY_CONNECTED));
  }

  updater.program();

  // route to remote destination under kDestPrefix advertised by bgp
  const auto& route = findLongestMatchRoute(
      this->sw_->getRib(), rid, kDestAddress, this->sw_->getState());

  EXPECT_EQ(
      route->has(
          ClientID::BGPD, RouteNextHopEntry(bgpNextHops, AdminDistance::EBGP)),
      true);

  EXPECT_TRUE(route->isResolved());

  for (const auto& nhop : route->getForwardInfo().getNextHopSet()) {
    EXPECT_TRUE(nhop.isResolved());
    EXPECT_TRUE(nhop.labelForwardingAction().has_value());
    EXPECT_EQ(
        nhop.labelForwardingAction()->type(),
        LabelForwardingAction::LabelForwardingType::PUSH);

    ASSERT_TRUE(
        nhop.intf() == kInterfaces[0] || nhop.intf() == kInterfaces[1] ||
        nhop.intf() == kInterfaces[2] || nhop.intf() == kInterfaces[3]);

    if (nhop.intf() == kInterfaces[0]) {
      EXPECT_EQ(
          nhop.labelForwardingAction()->pushStack().value(), kLabelStacks[0]);
    } else if (nhop.intf() == kInterfaces[1]) {
      EXPECT_EQ(
          nhop.labelForwardingAction()->pushStack().value(), kLabelStacks[1]);
    } else if (nhop.intf() == kInterfaces[2]) {
      EXPECT_EQ(
          nhop.labelForwardingAction()->pushStack().value(), kLabelStacks[2]);
    } else if (nhop.intf() == kInterfaces[3]) {
      EXPECT_EQ(
          nhop.labelForwardingAction()->pushStack().value(), kLabelStacks[3]);
    }
  }
}

TEST_F(RouteTest, updateTunnelLabels) {
  auto rid = RouterID(0);
  RouteNextHopSet bgpNextHops;
  bgpNextHops.emplace(UnresolvedNextHop(
      kBgpNextHopAddrs[0],
      1,
      std::make_optional<LabelForwardingAction>(
          LabelForwardingAction::LabelForwardingType::PUSH,
          LabelForwardingAction::LabelStack{
              kLabelStacks[0].begin(), kLabelStacks[0].begin() + 2})));

  auto updater = this->sw_->getRouteUpdater();
  // routes to remote prefix to bgp next hops
  updater.addRoute(
      rid,
      kDestPrefix.network(),
      kDestPrefix.mask(),
      ClientID::BGPD,
      RouteNextHopEntry(bgpNextHops, DISTANCE));

  ResolvedNextHop igpNextHop{
      kIgpAddrs[0],
      kInterfaces[0],
      ECMP_WEIGHT,
      LabelForwardingAction(
          LabelForwardingAction::LabelForwardingType::PUSH,
          LabelForwardingAction::LabelStack{
              kLabelStacks[0].begin() + 2, kLabelStacks[0].begin() + 3})};
  // igp routes to bgp nexthops,
  updater.addRoute(
      rid,
      kBgpNextHopAddrs[0],
      64,
      ClientID::OPENR,
      RouteNextHopEntry(
          static_cast<NextHop>(igpNextHop), AdminDistance::DIRECTLY_CONNECTED));

  updater.program();

  // igp next hop is updated
  ResolvedNextHop updatedIgpNextHop{
      kIgpAddrs[0],
      kInterfaces[0],
      ECMP_WEIGHT,
      LabelForwardingAction(
          LabelForwardingAction::LabelForwardingType::PUSH,
          LabelForwardingAction::LabelStack{
              kLabelStacks[1].begin() + 2, kLabelStacks[1].begin() + 3})};

  auto anotherUpdater = this->sw_->getRouteUpdater();
  anotherUpdater.delRoute(rid, kBgpNextHopAddrs[0], 64, ClientID::OPENR);
  anotherUpdater.addRoute(
      rid,
      kBgpNextHopAddrs[0],
      64,
      ClientID::OPENR,
      RouteNextHopEntry(
          static_cast<NextHop>(updatedIgpNextHop),
          AdminDistance::DIRECTLY_CONNECTED));

  anotherUpdater.program();

  LabelForwardingAction::LabelStack updatedStack;
  updatedStack.push_back(*kLabelStacks[0].begin());
  updatedStack.push_back(*(kLabelStacks[0].begin() + 1));
  updatedStack.push_back(*(kLabelStacks[1].begin() + 2));

  // route to remote destination under kDestPrefix advertised by bgp
  const auto& route = findLongestMatchRoute(
      this->sw_->getRib(), rid, kDestAddress, this->sw_->getState());

  EXPECT_EQ(
      route->has(
          ClientID::BGPD, RouteNextHopEntry(bgpNextHops, AdminDistance::EBGP)),
      true);

  EXPECT_TRUE(route->isResolved());

  for (const auto& nhop : route->getForwardInfo().getNextHopSet()) {
    EXPECT_TRUE(nhop.isResolved());
    EXPECT_TRUE(nhop.labelForwardingAction().has_value());
    EXPECT_EQ(
        nhop.labelForwardingAction()->type(),
        LabelForwardingAction::LabelForwardingType::PUSH);

    ASSERT_TRUE(nhop.intf() == kInterfaces[0]);

    EXPECT_EQ(nhop.labelForwardingAction()->pushStack().value(), updatedStack);
  }
}

TEST_F(RouteTest, updateRouteLabels) {
  auto rid = RouterID(0);
  RouteNextHopSet bgpNextHops;
  bgpNextHops.emplace(UnresolvedNextHop(
      kBgpNextHopAddrs[0],
      1,
      std::make_optional<LabelForwardingAction>(
          LabelForwardingAction::LabelForwardingType::PUSH,
          LabelForwardingAction::LabelStack{
              kLabelStacks[0].begin(), kLabelStacks[0].begin() + 2})));

  auto updater = this->sw_->getRouteUpdater();
  // routes to remote prefix to bgp next hops
  updater.addRoute(
      rid,
      kDestPrefix.network(),
      kDestPrefix.mask(),
      ClientID::BGPD,
      RouteNextHopEntry(bgpNextHops, DISTANCE));

  ResolvedNextHop igpNextHop{
      kIgpAddrs[0],
      kInterfaces[0],
      ECMP_WEIGHT,
      LabelForwardingAction(
          LabelForwardingAction::LabelForwardingType::PUSH,
          LabelForwardingAction::LabelStack{
              kLabelStacks[0].begin() + 2, kLabelStacks[0].begin() + 3})};
  // igp routes to bgp nexthops,
  updater.addRoute(
      rid,
      kBgpNextHopAddrs[0],
      64,
      ClientID::OPENR,
      RouteNextHopEntry(
          static_cast<NextHop>(igpNextHop), AdminDistance::DIRECTLY_CONNECTED));

  updater.program();

  // igp next hop is updated
  UnresolvedNextHop updatedBgpNextHop{
      kBgpNextHopAddrs[0],
      1,
      std::make_optional<LabelForwardingAction>(
          LabelForwardingAction::LabelForwardingType::PUSH,
          LabelForwardingAction::LabelStack{
              kLabelStacks[1].begin(), kLabelStacks[1].begin() + 2})};

  auto anotherUpdater = this->sw_->getRouteUpdater();
  anotherUpdater.delRoute(
      rid, kDestPrefix.network(), kDestPrefix.mask(), ClientID::BGPD);
  anotherUpdater.program();

  anotherUpdater.addRoute(
      rid,
      kDestPrefix.network(),
      kDestPrefix.mask(),
      ClientID::BGPD,
      RouteNextHopEntry(static_cast<NextHop>(updatedBgpNextHop), DISTANCE));

  anotherUpdater.program();

  LabelForwardingAction::LabelStack updatedStack;
  updatedStack.push_back(*kLabelStacks[1].begin());
  updatedStack.push_back(*(kLabelStacks[1].begin() + 1));
  updatedStack.push_back(*(kLabelStacks[0].begin() + 2));

  // route to remote destination under kDestPrefix advertised by bgp
  const auto& route = findLongestMatchRoute(
      this->sw_->getRib(), rid, kDestAddress, this->sw_->getState());

  EXPECT_EQ(
      route->has(
          ClientID::BGPD,
          RouteNextHopEntry(
              static_cast<NextHop>(updatedBgpNextHop), EBGP_DISTANCE)),
      true);

  EXPECT_TRUE(route->isResolved());

  for (const auto& nhop : route->getForwardInfo().getNextHopSet()) {
    EXPECT_TRUE(nhop.isResolved());
    EXPECT_TRUE(nhop.labelForwardingAction().has_value());
    EXPECT_EQ(
        nhop.labelForwardingAction()->type(),
        LabelForwardingAction::LabelForwardingType::PUSH);

    ASSERT_TRUE(nhop.intf() == kInterfaces[0]);

    EXPECT_EQ(nhop.labelForwardingAction()->pushStack().value(), updatedStack);
  }
}

TEST_F(RouteTest, withNoLabelForwardingAction) {
  auto rid = RouterID(0);

  auto routeNextHopEntry = RouteNextHopEntry(
      makeNextHops({"1.1.1.1", "1.1.2.1", "1.1.3.1", "1.1.4.1"}), DISTANCE);
  auto updater = this->sw_->getRouteUpdater();
  updater.addRoute(
      rid, folly::IPAddressV4("1.1.0.0"), 16, kClientA, routeNextHopEntry);

  updater.program();
  const auto& route = findLongestMatchRoute(
      this->sw_->getRib(),
      rid,
      folly::IPAddressV4("1.1.2.2"),
      this->sw_->getState());

  EXPECT_EQ(route->has(kClientA, routeNextHopEntry), true);
  auto entry = route->getBestEntry();
  for (const auto& nh : entry.second->getNextHopSet()) {
    EXPECT_EQ(nh.labelForwardingAction().has_value(), false);
  }
  EXPECT_EQ(*entry.second, routeNextHopEntry);
}

TEST_F(RouteTest, withInvalidLabelForwardingAction) {
  auto rid = RouterID(0);

  std::array<folly::IPAddressV4, 5> nextHopAddrs{
      folly::IPAddressV4("1.1.1.0"),
      folly::IPAddressV4("1.1.2.0"),
      folly::IPAddressV4("1.1.3.0"),
      folly::IPAddressV4("1.1.4.0"),
      folly::IPAddressV4("1.1.5.0"),
  };

  std::array<LabelForwardingAction, 5> nextHopLabelActions{
      LabelForwardingAction(
          LabelForwardingAction::LabelForwardingType::PUSH,
          LabelForwardingAction::LabelStack({101, 201, 301})),
      LabelForwardingAction(
          LabelForwardingAction::LabelForwardingType::SWAP,
          LabelForwardingAction::Label{202}),
      LabelForwardingAction(LabelForwardingAction::LabelForwardingType::NOOP),
      LabelForwardingAction(
          LabelForwardingAction::LabelForwardingType::POP_AND_LOOKUP),
      LabelForwardingAction(LabelForwardingAction::LabelForwardingType::PHP),
  };

  std::map<folly::IPAddressV4, LabelForwardingAction::LabelStack>
      labeledNextHops;
  RouteNextHopSet nexthops;

  for (auto i = 0; i < 5; i++) {
    nexthops.emplace(
        UnresolvedNextHop(nextHopAddrs[i], 1, nextHopLabelActions[i]));
  }

  auto updater = this->sw_->getRouteUpdater();
  updater.addRoute(
      rid,
      folly::IPAddressV4("1.1.0.0"),
      16,
      kClientA,
      RouteNextHopEntry(nexthops, DISTANCE));
  EXPECT_THROW(updater.program(), FbossError);
}

TEST_F(RouteTest, serializeRouteTable) {
  auto rid = RouterID(0);
  // 2 different nexthops
  RouteNextHopSet nhop1 = makeNextHops({"1.1.1.10"}); // resolved by intf 1
  RouteNextHopSet nhop2 = makeNextHops({"2.2.2.10"}); // resolved by intf 2
  // 4 prefixes
  RouteV4::Prefix r1{IPAddressV4("10.1.1.0"), 24};
  RouteV4::Prefix r2{IPAddressV4("20.1.1.0"), 24};
  RouteV6::Prefix r3{IPAddressV6("1001::0"), 48};
  RouteV6::Prefix r4{IPAddressV6("2001::0"), 48};

  auto u2 = this->sw_->getRouteUpdater();
  u2.addRoute(
      rid,
      r1.network(),
      r1.mask(),
      kClientA,
      RouteNextHopEntry(nhop1, DISTANCE));
  u2.addRoute(
      rid,
      r2.network(),
      r2.mask(),
      kClientA,
      RouteNextHopEntry(nhop2, DISTANCE));
  u2.addRoute(
      rid,
      r3.network(),
      r3.mask(),
      kClientA,
      RouteNextHopEntry(nhop1, DISTANCE));
  u2.addRoute(
      rid,
      r4.network(),
      r4.mask(),
      kClientA,
      RouteNextHopEntry(nhop2, DISTANCE));
  u2.program();

  // to thrift
  auto obj = this->sw_->getState()->toThrift();
  // back to Route object
  auto deserState = SwitchState::fromThrift(obj);
  // In new rib  only FIB is part of the switch state
  auto dyn0 = this->sw_->getState()->getFibs()->toThrift();
  auto dyn1 = deserState->getFibs()->toThrift();
  EXPECT_EQ(dyn0, dyn1);
}

class StaticRoutesTest : public RouteTest {
 public:
  cfg::SwitchConfig initialConfig() const override {
    auto config = RouteTest::initialConfig();
    // Add v4/v6 static routes with nhops
    config.staticRoutesWithNhops()->resize(2);
    config.staticRoutesWithNhops()[0].nexthops()->resize(1);
    config.staticRoutesWithNhops()[0].prefix() = "2001::/64";
    config.staticRoutesWithNhops()[0].nexthops()[0] = "2::2";
    config.staticRoutesWithNhops()[1].nexthops()->resize(1);
    config.staticRoutesWithNhops()[1].prefix() = "20.20.20.0/24";
    config.staticRoutesWithNhops()[1].nexthops()[0] = "2.2.2.3";

    auto insertStaticNoNhopRoutes = [=](auto& staticRouteNoNhops,
                                        int prefixStartIdx) {
      staticRouteNoNhops.resize(2);
      staticRouteNoNhops[0].prefix() =
          folly::sformat("240{}::/64", prefixStartIdx);
      staticRouteNoNhops[1].prefix() =
          folly::sformat("30.30.{}.0/24", prefixStartIdx);
    };
    // Add v4/v6 static routes to CPU/NULL
    insertStaticNoNhopRoutes(*config.staticRoutesToCPU(), 1);
    insertStaticNoNhopRoutes(*config.staticRoutesToNull(), 2);

    return config;
  }
};

TEST_F(StaticRoutesTest, staticRoutesGetApplied) {
  auto [numV4Routes, numV6Routes] = getRouteCount(this->sw_->getState());
  EXPECT_EQ(8, numV4Routes);
  EXPECT_EQ(9, numV6Routes);
}

/*
 * Class that makes it easy to run tests with the following
 * configurable entities:
 * Four interfaces: I1, I2, I3, I4
 * Three routes which require resolution: R1, R2, R3
 */
class UcmpTest : public RouteTest {
 public:
  void resolveRoutes(std::vector<std::pair<folly::IPAddress, RouteNextHopSet>>
                         networkAndNextHops) {
    auto ru = this->sw_->getRouteUpdater();
    for (const auto& nnhs : networkAndNextHops) {
      folly::IPAddress net = nnhs.first;
      RouteNextHopSet nhs = nnhs.second;
      ru.addRoute(rid_, net, mask, kClientA, RouteNextHopEntry(nhs, DISTANCE));
    }
    ru.program();
    auto state = this->sw_->getState();
    ASSERT_NE(nullptr, state);
    for (const auto& nnhs : networkAndNextHops) {
      folly::IPAddress net = nnhs.first;
      auto pfx = folly::sformat("{}/{}", net.str(), mask);
      auto r = this->findRoute4(state, rid_, pfx);
      EXPECT_RESOLVED(r);
      EXPECT_FALSE(r->isConnected());
      resolvedRoutes.push_back(r);
    }
  }

  void runRecursiveTest(
      const std::vector<RouteNextHopSet>& routeUnresolvedNextHops,
      const std::vector<NextHopWeight>& resolvedWeights) {
    std::vector<std::pair<folly::IPAddress, RouteNextHopSet>>
        networkAndNextHops;
    auto netsIter = nets.begin();
    for (const auto& nhs : routeUnresolvedNextHops) {
      networkAndNextHops.push_back({*netsIter, nhs});
      netsIter++;
    }
    resolveRoutes(networkAndNextHops);

    RouteNextHopSet expFwd1;
    uint8_t i = 0;
    for (const auto& w : resolvedWeights) {
      expFwd1.emplace(ResolvedNextHop(intfIps[i], InterfaceID(i + 1), w));
      ++i;
    }
    EXPECT_EQ(expFwd1, resolvedRoutes[0]->getForwardInfo().getNextHopSet());
  }

  void runTwoDeepRecursiveTest(
      const std::vector<std::vector<NextHopWeight>>& unresolvedWeights,
      const std::vector<NextHopWeight>& resolvedWeights) {
    std::vector<RouteNextHopSet> routeUnresolvedNextHops;
    auto rsIter = rnhs.begin();
    for (const auto& ws : unresolvedWeights) {
      auto nhIter = rsIter->begin();
      RouteNextHopSet nexthops;
      for (const auto& w : ws) {
        nexthops.insert(UnresolvedNextHop(*nhIter, w));
        nhIter++;
      }
      routeUnresolvedNextHops.push_back(nexthops);
      rsIter++;
    }
    runRecursiveTest(routeUnresolvedNextHops, resolvedWeights);
  }

  void runVaryFromHundredTest(
      NextHopWeight w,
      const std::vector<NextHopWeight>& resolvedWeights) {
    runRecursiveTest(
        {{UnresolvedNextHop(intfIp1, 100),
          UnresolvedNextHop(intfIp2, 100),
          UnresolvedNextHop(intfIp3, 100),
          UnresolvedNextHop(intfIp4, w)}},
        resolvedWeights);
  }

  std::vector<std::shared_ptr<Route<folly::IPAddressV4>>> resolvedRoutes;
  const folly::IPAddress intfIp1{"1.1.1.10"};
  const folly::IPAddress intfIp2{"2.2.2.20"};
  const folly::IPAddress intfIp3{"3.3.3.30"};
  const folly::IPAddress intfIp4{"4.4.4.40"};
  const std::array<folly::IPAddress, 4> intfIps{
      {intfIp1, intfIp2, intfIp3, intfIp4}};
  const folly::IPAddress r2Nh{"42.42.42.42"};
  const folly::IPAddress r3Nh{"43.43.43.43"};
  std::array<folly::IPAddress, 2> r1Nhs{{r2Nh, r3Nh}};
  std::array<folly::IPAddress, 2> r2Nhs{{intfIp1, intfIp2}};
  std::array<folly::IPAddress, 2> r3Nhs{{intfIp3, intfIp4}};
  const std::array<std::array<folly::IPAddress, 2>, 3> rnhs{
      {r1Nhs, r2Nhs, r3Nhs}};
  const folly::IPAddress r1Net{"41.41.41.0"};
  const folly::IPAddress r2Net{"42.42.42.0"};
  const folly::IPAddress r3Net{"43.43.43.0"};
  const std::array<folly::IPAddress, 3> nets{{r1Net, r2Net, r3Net}};
  const uint8_t mask{24};

 private:
  RouterID rid_{0};
};

/*
 * Four interfaces: I1, I2, I3, I4
 * Three routes which require resolution: R1, R2, R3
 * R1 has R2 and R3 as next hops with weights 3 and 2
 * R2 has I1 and I2 as next hops with weights 5 and 4
 * R3 has I3 and I4 as next hops with weights 3 and 2
 * expect R1 to resolve to I1:25, I2:20, I3:18, I4:12
 */
TEST_F(UcmpTest, recursiveUcmp) {
  this->runTwoDeepRecursiveTest({{3, 2}, {5, 4}, {3, 2}}, {25, 20, 18, 12});
}

/*
 * Two interfaces: I1, I2
 * Three routes which require resolution: R1, R2, R3
 * R1 has R2 and R3 as next hops with weights 2 and 1
 * R2 has I1 and I2 as next hops with weights 1 and 1
 * R3 has I1 as next hop with weight 1
 * expect R1 to resolve to I1:2, I2:1
 */
TEST_F(UcmpTest, recursiveUcmpDuplicateIntf) {
  this->runRecursiveTest(
      {{UnresolvedNextHop(this->r2Nh, 2), UnresolvedNextHop(this->r3Nh, 1)},
       {UnresolvedNextHop(this->intfIp1, 1),
        UnresolvedNextHop(this->intfIp2, 1)},
       {UnresolvedNextHop(this->intfIp1, 1)}},
      {2, 1});
}

/*
 * Two interfaces: I1, I2
 * Three routes which require resolution: R1, R2, R3
 * R1 has R2 and R3 as next hops with ECMP
 * R2 has I1 and I2 as next hops with ECMP
 * R3 has I1 as next hop with weight 1
 * expect R1 to resolve to ECMP
 */
TEST_F(UcmpTest, recursiveEcmpDuplicateIntf) {
  this->runRecursiveTest(
      {{UnresolvedNextHop(this->r2Nh, ECMP_WEIGHT),
        UnresolvedNextHop(this->r3Nh, ECMP_WEIGHT)},
       {UnresolvedNextHop(this->intfIp1, ECMP_WEIGHT),
        UnresolvedNextHop(this->intfIp2, ECMP_WEIGHT)},
       {UnresolvedNextHop(this->intfIp1, 1)}},
      {ECMP_WEIGHT, ECMP_WEIGHT});
}

/*
 * Two interfaces: I1, I2
 * One route which requires resolution: R1
 * R1 has I1 and I2 as next hops with weights 0 (ECMP) and 1
 * expect R1 to resolve to ECMP between I1, I2
 */
TEST_F(UcmpTest, mixedUcmpVsEcmp_EcmpWins) {
  this->runRecursiveTest(
      {{UnresolvedNextHop(this->intfIp1, ECMP_WEIGHT),
        UnresolvedNextHop(this->intfIp2, 1)}},
      {ECMP_WEIGHT, ECMP_WEIGHT});
}

/*
 * Four interfaces: I1, I2, I3, I4
 * Three routes which require resolution: R1, R2, R3
 * R1 has R2 and R3 as next hops with weights 3 and 2
 * R2 has I1 and I2 as next hops with weights 5 and 4
 * R3 has I3 and I4 as next hops with ECMP
 * expect R1 to resolve to ECMP between I1, I2, I3, I4
 */
TEST_F(UcmpTest, recursiveEcmpPropagatesUp) {
  this->runTwoDeepRecursiveTest({{3, 2}, {5, 4}, {0, 0}}, {0, 0, 0, 0});
}

/*
 * Four interfaces: I1, I2, I3, I4
 * Three routes which require resolution: R1, R2, R3
 * R1 has R2 and R3 as next hops with weights 3 and 2
 * R2 has I1 and I2 as next hops with weights 5 and 4
 * R3 has I3 and I4 as next hops with weights 0 (ECMP) and 1
 * expect R1 to resolve to ECMP between I1, I2, I3, I4
 */
TEST_F(UcmpTest, recursiveMixedEcmpPropagatesUp) {
  this->runTwoDeepRecursiveTest({{3, 2}, {5, 4}, {0, 1}}, {0, 0, 0, 0});
}

/*
 * Four interfaces: I1, I2, I3, I4
 * Three routes which require resolution: R1, R2, R3
 * R1 has R2 and R3 as next hops with ECMP
 * R2 has I1 and I2 as next hops with weights 5 and 4
 * R3 has I3 and I4 as next hops with weights 3 and 2
 * expect R1 to resolve to ECMP between I1, I2, I3, I4
 */
TEST_F(UcmpTest, recursiveEcmpPropagatesDown) {
  this->runTwoDeepRecursiveTest({{0, 0}, {5, 4}, {3, 2}}, {0, 0, 0, 0});
}

/*
 * Two interfaces: I1, I2
 * Two routes which require resolution: R1, R2
 * R1 has I1 and I2 as next hops with ECMP
 * R2 has I1 and I2 as next hops with weights 2 and 1
 * expect R1 to resolve to ECMP between I1, I2
 * expect R2 to resolve to I1:2, I2: 1
 */
TEST_F(UcmpTest, separateEcmpUcmp) {
  this->runRecursiveTest(
      {{UnresolvedNextHop(this->intfIp1, ECMP_WEIGHT),
        UnresolvedNextHop(this->intfIp2, ECMP_WEIGHT)},
       {UnresolvedNextHop(this->intfIp1, 2),
        UnresolvedNextHop(this->intfIp2, 1)}},
      {ECMP_WEIGHT, ECMP_WEIGHT});
  RouteNextHopSet route2ExpFwd;
  route2ExpFwd.emplace(
      ResolvedNextHop(IPAddress("1.1.1.10"), InterfaceID(1), 2));
  route2ExpFwd.emplace(
      ResolvedNextHop(IPAddress("2.2.2.20"), InterfaceID(2), 1));
  EXPECT_EQ(
      route2ExpFwd, this->resolvedRoutes[1]->getForwardInfo().getNextHopSet());
}

// The following set of tests will start with 4 next hops all weight 100
// then vary one next hop by 10 weight increments to 90, 80, ... , 10

TEST_F(UcmpTest, Hundred) {
  this->runVaryFromHundredTest(100, {1, 1, 1, 1});
}

TEST_F(UcmpTest, Ninety) {
  this->runVaryFromHundredTest(90, {10, 10, 10, 9});
}

TEST_F(UcmpTest, Eighty) {
  this->runVaryFromHundredTest(80, {5, 5, 5, 4});
}

TEST_F(UcmpTest, Seventy) {
  this->runVaryFromHundredTest(70, {10, 10, 10, 7});
}

TEST_F(UcmpTest, Sixty) {
  this->runVaryFromHundredTest(60, {5, 5, 5, 3});
}

TEST_F(UcmpTest, Fifty) {
  this->runVaryFromHundredTest(50, {2, 2, 2, 1});
}

TEST_F(UcmpTest, Forty) {
  this->runVaryFromHundredTest(40, {5, 5, 5, 2});
}

TEST_F(UcmpTest, Thirty) {
  this->runVaryFromHundredTest(30, {10, 10, 10, 3});
}

TEST_F(UcmpTest, Twenty) {
  this->runVaryFromHundredTest(20, {5, 5, 5, 1});
}

TEST_F(UcmpTest, Ten) {
  this->runVaryFromHundredTest(10, {10, 10, 10, 1});
}

TEST_F(RouteTest, CounterIDTest) {
  auto u1 = this->sw_->getRouteUpdater();

  RouteV4::Prefix prefix10{IPAddressV4("10.10.10.10"), 32};

  RouteNextHopSet nexthops1 =
      makeResolvedNextHops({{InterfaceID(1), "1.1.1.1"}});

  // Add route with counter id
  u1.addRoute(
      kRid0,
      IPAddress("10.10.10.10"),
      32,
      kClientA,
      RouteNextHopEntry(
          nexthops1, DISTANCE, std::optional<RouteCounterID>(kCounterID1)));
  u1.program();

  auto rt10 = this->findRoute4(this->sw_->getState(), kRid0, prefix10);
  EXPECT_EQ(rt10->getForwardInfo().getCounterID(), kCounterID1);
  // EXPECT_EQ(rt10->getEntryForClient(kClientA)->getCounterID(), kCounterID1);

  // Modify counter id
  u1.addRoute(
      kRid0,
      IPAddress("10.10.10.10"),
      32,
      kClientA,
      RouteNextHopEntry(
          nexthops1, DISTANCE, std::optional<RouteCounterID>(kCounterID2)));
  u1.program();

  rt10 = this->findRoute4(this->sw_->getState(), kRid0, prefix10);
  EXPECT_EQ(rt10->getForwardInfo().getCounterID(), kCounterID2);
  EXPECT_EQ(*(rt10->getEntryForClient(kClientA)->getCounterID()), kCounterID2);

  // Modify route to remove counter id
  u1.addRoute(
      kRid0,
      IPAddress("10.10.10.10"),
      32,
      kClientA,
      RouteNextHopEntry(
          nexthops1, DISTANCE, std::optional<RouteCounterID>(std::nullopt)));
  u1.program();

  rt10 = this->findRoute4(this->sw_->getState(), kRid0, prefix10);
  EXPECT_EQ(rt10->getForwardInfo().getCounterID(), std::nullopt);
  EXPECT_TRUE(!rt10->getEntryForClient(kClientA)->getCounterID());
}

TEST_F(RouteTest, DropAndPuntRouteCounterID) {
  auto verifyCounter = [&](const auto& fwdAction) {
    auto u1 = sw_->getRouteUpdater();
    RouteV4::Prefix prefix10{IPAddressV4("10.10.10.10"), 32};
    // Add route with counter id
    u1.addRoute(
        kRid0,
        IPAddress("10.10.10.10"),
        32,
        kClientA,
        RouteNextHopEntry(
            fwdAction, DISTANCE, std::optional<RouteCounterID>(std::nullopt)));
    u1.program();

    auto rt10 = findRoute4(sw_->getState(), kRid0, prefix10);
    EXPECT_EQ(rt10->getForwardInfo().getCounterID(), std::nullopt);
    EXPECT_TRUE(!rt10->getEntryForClient(kClientA)->getCounterID());

    // Add counter id
    u1.addRoute(
        kRid0,
        IPAddress("10.10.10.10"),
        32,
        kClientA,
        RouteNextHopEntry(fwdAction, DISTANCE, kCounterID1));
    u1.program();

    rt10 = findRoute4(sw_->getState(), kRid0, prefix10);
    EXPECT_EQ(rt10->getForwardInfo().getCounterID(), kCounterID1);
    if (auto counter = rt10->getEntryForClient(kClientA)->getCounterID()) {
      EXPECT_EQ(counter, kCounterID1);
    }

    // Modify counter id
    u1.addRoute(
        kRid0,
        IPAddress("10.10.10.10"),
        32,
        kClientA,
        RouteNextHopEntry(fwdAction, DISTANCE, kCounterID2));
    u1.program();

    rt10 = findRoute4(sw_->getState(), kRid0, prefix10);
    EXPECT_EQ(rt10->getForwardInfo().getCounterID(), kCounterID2);
    if (auto counter = rt10->getEntryForClient(kClientA)->getCounterID()) {
      EXPECT_EQ(*counter, kCounterID2);
    }

    // Modify route to remove counter id
    u1.addRoute(
        kRid0,
        IPAddress("10.10.10.10"),
        32,
        kClientA,
        RouteNextHopEntry(
            fwdAction, DISTANCE, std::optional<RouteCounterID>(std::nullopt)));
    u1.program();

    rt10 = findRoute4(sw_->getState(), kRid0, prefix10);
    EXPECT_EQ(rt10->getForwardInfo().getCounterID(), std::nullopt);
    EXPECT_TRUE(!rt10->getEntryForClient(kClientA)->getCounterID());
  };
  verifyCounter(RouteForwardAction::DROP);
  verifyCounter(RouteForwardAction::TO_CPU);
}

TEST_F(RouteTest, ClassIDTest) {
  auto u1 = this->sw_->getRouteUpdater();

  RouteV4::Prefix prefix10{IPAddressV4("10.10.10.10"), 32};

  RouteNextHopSet nexthops1 =
      makeResolvedNextHops({{InterfaceID(1), "1.1.1.1"}});

  // Add route with class id
  u1.addRoute(
      kRid0,
      IPAddress("10.10.10.10"),
      32,
      kClientA,
      RouteNextHopEntry(
          nexthops1,
          DISTANCE,
          std::optional<RouteCounterID>(std ::nullopt),
          std::optional<cfg::AclLookupClass>(kClassID1)));
  u1.program();

  auto rt10 = this->findRoute4(this->sw_->getState(), kRid0, prefix10);
  EXPECT_EQ(rt10->getClassID(), kClassID1);
  EXPECT_EQ(rt10->getForwardInfo().getClassID(), kClassID1);
  if (auto classID = rt10->getEntryForClient(kClientA)->getClassID()) {
    EXPECT_EQ(*classID, kClassID1);
  }

  // Modify class id
  u1.addRoute(
      kRid0,
      IPAddress("10.10.10.10"),
      32,
      kClientA,
      RouteNextHopEntry(
          nexthops1,
          DISTANCE,
          std::optional<RouteCounterID>(std::nullopt),
          std::optional<cfg::AclLookupClass>(kClassID2)));
  u1.program();

  rt10 = this->findRoute4(this->sw_->getState(), kRid0, prefix10);
  EXPECT_EQ(rt10->getClassID(), kClassID2);
  EXPECT_EQ(rt10->getForwardInfo().getClassID(), kClassID2);
  if (auto classID = rt10->getEntryForClient(kClientA)->getClassID()) {
    EXPECT_EQ(*classID, kClassID2);
  }

  // Modify route to remove class id
  u1.addRoute(
      kRid0,
      IPAddress("10.10.10.10"),
      32,
      kClientA,
      RouteNextHopEntry(
          nexthops1,
          DISTANCE,
          std::optional<RouteCounterID>(std::nullopt),
          std::optional<cfg::AclLookupClass>(std::nullopt)));
  u1.program();

  rt10 = this->findRoute4(this->sw_->getState(), kRid0, prefix10);
  EXPECT_EQ(rt10->getClassID(), std::nullopt);
  EXPECT_EQ(rt10->getForwardInfo().getClassID(), std::nullopt);
  EXPECT_TRUE(!rt10->getEntryForClient(kClientA)->getClassID());
}
