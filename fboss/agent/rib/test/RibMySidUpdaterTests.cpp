/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include <gtest/gtest.h>

#include <common/network/if/gen-cpp2/Address_types.h>
#include "fboss/agent/rib/NetworkToRouteMap.h"
#include "fboss/agent/rib/NextHopIDManager.h"
#include "fboss/agent/rib/RibMySidUpdater.h"
#include "fboss/agent/rib/RouteUpdater.h"
#include "fboss/agent/state/MySid.h"
#include "fboss/agent/state/Route.h"
#include "fboss/agent/state/RouteNextHop.h"
#include "fboss/agent/state/RouteNextHopEntry.h"
#include "fboss/agent/types.h"

#include <folly/IPAddress.h>
#include <folly/IPAddressV6.h>
#include <memory>

namespace facebook::fboss {

namespace {

std::shared_ptr<MySid> makeMySid(
    const std::string& sidPrefix,
    uint8_t prefixLen,
    MySidType type = MySidType::NODE_MICRO_SID) {
  state::MySidFields fields;
  fields.type() = type;
  facebook::network::thrift::IPPrefix thriftPrefix;
  thriftPrefix.prefixAddress() =
      facebook::network::toBinaryAddress(folly::IPAddress(sidPrefix));
  thriftPrefix.prefixLength() = prefixLen;
  fields.mySid() = thriftPrefix;
  return std::make_shared<MySid>(fields);
}

std::shared_ptr<Route<folly::IPAddressV6>> makeResolvedV6Route(
    const folly::IPAddressV6& network,
    uint8_t mask,
    const RouteNextHopSet& nhops) {
  RoutePrefix<folly::IPAddressV6> prefix{network, mask};
  auto thrift = Route<folly::IPAddressV6>::makeThrift(prefix);
  auto route = std::make_shared<Route<folly::IPAddressV6>>(thrift);
  route->setResolved(RouteNextHopEntry(nhops, AdminDistance::EBGP));
  route->publish();
  return route;
}

std::shared_ptr<Route<folly::IPAddressV6>> makeConnectedV6Route(
    const folly::IPAddressV6& network,
    uint8_t mask,
    InterfaceID intfId) {
  RoutePrefix<folly::IPAddressV6> prefix{network, mask};
  auto thrift = Route<folly::IPAddressV6>::makeThrift(prefix);
  auto route = std::make_shared<Route<folly::IPAddressV6>>(thrift);
  RouteNextHopSet nhops{
      ResolvedNextHop(folly::IPAddress(network), intfId, ECMP_WEIGHT)};
  route->setResolved(
      RouteNextHopEntry(nhops, AdminDistance::DIRECTLY_CONNECTED));
  route->setConnected();
  route->publish();
  return route;
}

RouteNextHopSet makeResolvedNhops(
    const std::vector<std::pair<std::string, InterfaceID>>& nhops) {
  RouteNextHopSet result;
  for (const auto& [addr, intfId] : nhops) {
    result.emplace(
        ResolvedNextHop(folly::IPAddress(addr), intfId, ECMP_WEIGHT));
  }
  return result;
}

} // namespace

class RibMySidUpdaterTest : public ::testing::Test {
 protected:
  void SetUp() override {
    manager_ = std::make_unique<NextHopIDManager>();
  }

  NextHopIDManager& manager() {
    return *manager_;
  }

  NextHopSetID allocUnresolvedSet(const RouteNextHopSet& nhops) {
    return manager()
        .getOrAllocRouteNextHopSetID(nhops)
        .nextHopIdSetIter->second.id;
  }

  std::unique_ptr<NextHopIDManager> manager_;
  IPv4NetworkToRouteMap v4Routes_;
  IPv6NetworkToRouteMap v6Routes_;
  MySidTable mySidTable_;
};

TEST_F(RibMySidUpdaterTest, noUnresolvedId_entrySkipped) {
  // MySid without unresolvedNextHopsId should not get a resolvedNextHopsId.
  auto mySid = makeMySid("fc00:100::1", 48);
  const folly::CIDRNetworkV6 key{folly::IPAddressV6("fc00:100::1"), 48};
  mySidTable_[key] = mySid;

  RibMySidUpdater updater({{&v4Routes_, &v6Routes_}}, &manager(), &mySidTable_);
  updater.resolve();

  EXPECT_FALSE(mySidTable_.at(key)->getResolvedNextHopsId().has_value());
}

TEST_F(RibMySidUpdaterTest, nhopWithIntfId_resolvedSetIdAllocated) {
  // Nexthop with InterfaceID is already resolved → resolvedNextHopsId set.
  const auto nhops = makeResolvedNhops({{"fe80::1", InterfaceID(1)}});
  const auto unresolvedId = allocUnresolvedSet(nhops);

  auto mySid = makeMySid("fc00:100::1", 48);
  mySid->setUnresolveNextHopsId(unresolvedId);
  const folly::CIDRNetworkV6 key{folly::IPAddressV6("fc00:100::1"), 48};
  mySidTable_[key] = mySid;

  RibMySidUpdater updater({{&v4Routes_, &v6Routes_}}, &manager(), &mySidTable_);
  updater.resolve();

  const auto resolvedId = mySidTable_.at(key)->getResolvedNextHopsId();
  ASSERT_TRUE(resolvedId.has_value());
  EXPECT_TRUE(manager().getNextHopsIf(*resolvedId).has_value());
}

TEST_F(RibMySidUpdaterTest, nhopRefCountBumped_afterResolvingNhopWithIntfId) {
  // Each nexthop in the resolved set should have its ref count incremented.
  // Verified by checking the nexthop ID is present in idToNextHop after
  // resolve.
  const auto nhops = makeResolvedNhops({{"fe80::1", InterfaceID(1)}});
  const auto unresolvedId = allocUnresolvedSet(nhops);

  auto mySid = makeMySid("fc00:100::1", 48);
  mySid->setUnresolveNextHopsId(unresolvedId);
  const folly::CIDRNetworkV6 key{folly::IPAddressV6("fc00:100::1"), 48};
  mySidTable_[key] = mySid;

  RibMySidUpdater updater({{&v4Routes_, &v6Routes_}}, &manager(), &mySidTable_);
  updater.resolve();

  const auto resolvedId = mySidTable_.at(key)->getResolvedNextHopsId();
  ASSERT_TRUE(resolvedId.has_value());

  // Verify individual nexthop IDs are tracked (ref count > 0)
  const auto& nhIdSet = manager().getIdToNextHopIdSet().at(*resolvedId);
  const auto& idToNhop = manager().getIdToNextHop();
  EXPECT_FALSE(nhIdSet.empty());
  for (const auto& nhId : nhIdSet) {
    EXPECT_NE(idToNhop.find(nhId), idToNhop.end());
  }
  EXPECT_EQ(manager().getNextHopRefCount(*nhops.begin()), 2);
}

TEST_F(RibMySidUpdaterTest, gatewayNhopMatchingV6Route_resolvedSetIdAllocated) {
  // Gateway nexthop (no intfID) resolved via v6 route LPM lookup.
  const auto routeNhops = makeResolvedNhops(
      {{"fe80::1", InterfaceID(1)}, {"fe80::2", InterfaceID(2)}});
  auto v6Route =
      makeResolvedV6Route(folly::IPAddressV6("2001:db8::"), 32, routeNhops);
  v6Routes_.insert(v6Route->prefix(), v6Route);

  const RouteNextHopSet unresolvedNhops{
      UnresolvedNextHop(folly::IPAddress("2001:db8::1"), ECMP_WEIGHT)};
  const auto unresolvedId = allocUnresolvedSet(unresolvedNhops);

  auto mySid = makeMySid("fc00:100::1", 48);
  mySid->setUnresolveNextHopsId(unresolvedId);
  const folly::CIDRNetworkV6 key{folly::IPAddressV6("fc00:100::1"), 48};
  mySidTable_[key] = mySid;

  RibMySidUpdater updater({{&v4Routes_, &v6Routes_}}, &manager(), &mySidTable_);
  updater.resolve();

  const auto resolvedId = mySidTable_.at(key)->getResolvedNextHopsId();
  ASSERT_TRUE(resolvedId.has_value());
  const auto resolvedNhops = manager().getNextHopsIf(*resolvedId);
  ASSERT_TRUE(resolvedNhops.has_value());
  // Resolved set comes from normalizedNextHops(), so only verify size matches.
  EXPECT_EQ(resolvedNhops->size(), routeNhops.size());
}

TEST_F(RibMySidUpdaterTest, gatewayNhopNoRouteMatch_noResolvedSetId) {
  // Gateway nexthop with no matching route → empty resolved set → no ID.
  const RouteNextHopSet unresolvedNhops{
      UnresolvedNextHop(folly::IPAddress("2001:db8::1"), ECMP_WEIGHT)};
  const auto unresolvedId = allocUnresolvedSet(unresolvedNhops);

  auto mySid = makeMySid("fc00:100::1", 48);
  mySid->setUnresolveNextHopsId(unresolvedId);
  const folly::CIDRNetworkV6 key{folly::IPAddressV6("fc00:100::1"), 48};
  mySidTable_[key] = mySid;

  RibMySidUpdater updater({{&v4Routes_, &v6Routes_}}, &manager(), &mySidTable_);
  updater.resolve();

  EXPECT_FALSE(mySidTable_.at(key)->getResolvedNextHopsId().has_value());
}

TEST_F(RibMySidUpdaterTest, secondResolve_differentNhops_oldSetIdDeallocated) {
  // After re-resolving with different nexthops, the old resolvedSetId should
  // be deallocated (ref count reaches 0) and a new one allocated.
  // Use gateway nexthops resolved via routes so unresolvedId != resolvedId.
  const auto routeNhops1 = makeResolvedNhops({{"fe80::1", InterfaceID(1)}});
  const auto routeNhops2 = makeResolvedNhops({{"fe80::2", InterfaceID(2)}});
  auto v6Route1 =
      makeResolvedV6Route(folly::IPAddressV6("2001:db8:1::"), 48, routeNhops1);
  auto v6Route2 =
      makeResolvedV6Route(folly::IPAddressV6("2001:db8:2::"), 48, routeNhops2);
  v6Routes_.insert(v6Route1->prefix(), v6Route1);
  v6Routes_.insert(v6Route2->prefix(), v6Route2);

  const RouteNextHopSet unresolvedNhops1{
      UnresolvedNextHop(folly::IPAddress("2001:db8:1::1"), ECMP_WEIGHT)};
  const RouteNextHopSet unresolvedNhops2{
      UnresolvedNextHop(folly::IPAddress("2001:db8:2::1"), ECMP_WEIGHT)};
  const auto unresolvedId1 = allocUnresolvedSet(unresolvedNhops1);
  const auto unresolvedId2 = allocUnresolvedSet(unresolvedNhops2);

  auto mySid = makeMySid("fc00:100::1", 48);
  mySid->setUnresolveNextHopsId(unresolvedId1);
  const folly::CIDRNetworkV6 key{folly::IPAddressV6("fc00:100::1"), 48};
  mySidTable_[key] = mySid;

  // First resolve: allocates resolvedId pointing to routeNhops1.
  RibMySidUpdater updater1(
      {{&v4Routes_, &v6Routes_}}, &manager(), &mySidTable_);
  updater1.resolve();

  const auto firstResolvedId = mySidTable_.at(key)->getResolvedNextHopsId();
  ASSERT_TRUE(firstResolvedId.has_value());

  // Swap unresolvedId to point at the second route (simulating a route update).
  mySidTable_[key] = mySidTable_[key]->clone();
  mySidTable_[key]->setUnresolveNextHopsId(unresolvedId2);

  // Second resolve: old resolvedId decremented, new one allocated.
  RibMySidUpdater updater2(
      {{&v4Routes_, &v6Routes_}}, &manager(), &mySidTable_);
  updater2.resolve();

  const auto secondResolvedId = mySidTable_.at(key)->getResolvedNextHopsId();
  ASSERT_TRUE(secondResolvedId.has_value());
  EXPECT_NE(*firstResolvedId, *secondResolvedId);
  // Old set ID should be deallocated (ref count hit 0).
  EXPECT_FALSE(manager().getNextHopsIf(*firstResolvedId).has_value());
  // New set ID should be alive.
  EXPECT_TRUE(manager().getNextHopsIf(*secondResolvedId).has_value());
}

TEST_F(
    RibMySidUpdaterTest,
    twoEntriesSameNhops_resolvedSetIdSharedAndRefCountIsTwo) {
  // Two MySid entries with different gateway nexthops that both resolve via
  // the same route. They should share one NextHopSetID with ref count 2.
  const auto sharedRouteNhops =
      makeResolvedNhops({{"fe80::1", InterfaceID(1)}});
  auto v6Route = makeResolvedV6Route(
      folly::IPAddressV6("2001:db8::"), 32, sharedRouteNhops);
  v6Routes_.insert(v6Route->prefix(), v6Route);

  const RouteNextHopSet unresolvedNhops1{
      UnresolvedNextHop(folly::IPAddress("2001:db8::1"), ECMP_WEIGHT)};
  const RouteNextHopSet unresolvedNhops2{
      UnresolvedNextHop(folly::IPAddress("2001:db8::2"), ECMP_WEIGHT)};
  const auto unresolvedId1 = allocUnresolvedSet(unresolvedNhops1);
  const auto unresolvedId2 = allocUnresolvedSet(unresolvedNhops2);

  auto mySid1 = makeMySid("fc00:100::1", 48);
  mySid1->setUnresolveNextHopsId(unresolvedId1);
  const folly::CIDRNetworkV6 key1{folly::IPAddressV6("fc00:100::1"), 48};
  mySidTable_[key1] = mySid1;

  auto mySid2 = makeMySid("fc00:200::1", 48);
  mySid2->setUnresolveNextHopsId(unresolvedId2);
  const folly::CIDRNetworkV6 key2{folly::IPAddressV6("fc00:200::1"), 48};
  mySidTable_[key2] = mySid2;

  RibMySidUpdater updater({{&v4Routes_, &v6Routes_}}, &manager(), &mySidTable_);
  updater.resolve();

  const auto id1 = mySidTable_.at(key1)->getResolvedNextHopsId();
  const auto id2 = mySidTable_.at(key2)->getResolvedNextHopsId();
  ASSERT_TRUE(id1.has_value());
  ASSERT_TRUE(id2.has_value());
  EXPECT_EQ(*id1, *id2);

  // Ref count of the shared set should be 2 (one for each MySid entry).
  const auto& nhIdSet = manager().getIdToNextHopIdSet().at(*id1);
  EXPECT_EQ(manager().getNextHopIDSetRefCount(nhIdSet), 2);
}

TEST_F(RibMySidUpdaterTest, resolveFiltered_onlyMatchingEntryResolved) {
  // Two MySid entries; only the one in the filter set should be resolved.
  const auto nhops1 = makeResolvedNhops({{"fe80::1", InterfaceID(1)}});
  const auto nhops2 = makeResolvedNhops({{"fe80::2", InterfaceID(2)}});
  const auto unresolvedId1 = allocUnresolvedSet(nhops1);
  const auto unresolvedId2 = allocUnresolvedSet(nhops2);

  const folly::CIDRNetworkV6 key1{folly::IPAddressV6("fc00:100::1"), 48};
  const folly::CIDRNetworkV6 key2{folly::IPAddressV6("fc00:200::1"), 48};

  auto mySid1 = makeMySid("fc00:100::1", 48);
  mySid1->setUnresolveNextHopsId(unresolvedId1);
  mySidTable_[key1] = mySid1;

  auto mySid2 = makeMySid("fc00:200::1", 48);
  mySid2->setUnresolveNextHopsId(unresolvedId2);
  mySidTable_[key2] = mySid2;

  const std::set<folly::CIDRNetwork> filter{
      {folly::IPAddress("fc00:100::1"), 48}};

  RibMySidUpdater updater({{&v4Routes_, &v6Routes_}}, &manager(), &mySidTable_);
  updater.resolve(filter);

  EXPECT_TRUE(mySidTable_.at(key1)->getResolvedNextHopsId().has_value());
  EXPECT_FALSE(mySidTable_.at(key2)->getResolvedNextHopsId().has_value());
}

TEST_F(RibMySidUpdaterTest, resolveFiltered_cidrNotInTable_skipped) {
  // A CIDR in the filter that does not exist in the MySidTable should be
  // gracefully skipped (no crash, table unchanged).
  const folly::CIDRNetworkV6 key{folly::IPAddressV6("fc00:100::1"), 48};
  mySidTable_[key] = makeMySid("fc00:100::1", 48);

  const std::set<folly::CIDRNetwork> filter{
      {folly::IPAddress("fc00:999::1"), 48}};

  RibMySidUpdater updater({{&v4Routes_, &v6Routes_}}, &manager(), &mySidTable_);
  updater.resolve(filter);

  EXPECT_FALSE(mySidTable_.at(key)->getResolvedNextHopsId().has_value());
}

TEST_F(RibMySidUpdaterTest, resolveFiltered_entryWithNoUnresolvedId_skipped) {
  // An entry in the filter that has no unresolvedNextHopsId should be skipped.
  const folly::CIDRNetworkV6 key{folly::IPAddressV6("fc00:100::1"), 48};
  mySidTable_[key] = makeMySid("fc00:100::1", 48); // no unresolvedId set

  const std::set<folly::CIDRNetwork> filter{
      {folly::IPAddress("fc00:100::1"), 48}};

  RibMySidUpdater updater({{&v4Routes_, &v6Routes_}}, &manager(), &mySidTable_);
  updater.resolve(filter);

  EXPECT_FALSE(mySidTable_.at(key)->getResolvedNextHopsId().has_value());
}

TEST_F(
    RibMySidUpdaterTest,
    gatewayNhopMatchingConnectedRoute_resolvedWithGatewayAddr) {
  // Gateway nexthop resolved via a connected (interface) route. The resolved
  // next hop must use the original gateway address paired with the connected
  // route's interface — not the route's own forwarding next hop address.
  const InterfaceID connectedIntf{10};
  const folly::IPAddress gatewayAddr("2001:db8::1");

  auto connectedRoute =
      makeConnectedV6Route(folly::IPAddressV6("2001:db8::"), 32, connectedIntf);
  v6Routes_.insert(connectedRoute->prefix(), connectedRoute);

  const RouteNextHopSet unresolvedNhops{
      UnresolvedNextHop(gatewayAddr, ECMP_WEIGHT)};
  const auto unresolvedId = allocUnresolvedSet(unresolvedNhops);

  auto mySid = makeMySid("fc00:100::1", 48);
  mySid->setUnresolveNextHopsId(unresolvedId);
  const folly::CIDRNetworkV6 key{folly::IPAddressV6("fc00:100::1"), 48};
  mySidTable_[key] = mySid;

  RibMySidUpdater updater({{&v4Routes_, &v6Routes_}}, &manager(), &mySidTable_);
  updater.resolve();

  const auto resolvedId = mySidTable_.at(key)->getResolvedNextHopsId();
  ASSERT_TRUE(resolvedId.has_value());
  const auto resolvedNhops = manager().getNextHopsIf(*resolvedId);
  ASSERT_TRUE(resolvedNhops.has_value());
  ASSERT_EQ(resolvedNhops->size(), 1);

  // normalizedNextHops() converts ECMP_WEIGHT(0) to 1 for a single next hop.
  const RouteNextHopSet expected{
      ResolvedNextHop(gatewayAddr, connectedIntf, NextHopWeight(1))};
  EXPECT_EQ(*resolvedNhops, expected);
}

TEST_F(
    RibMySidUpdaterTest,
    multiVrf_nhopResolvedViaSecondVrf_resolvedSetIdAllocated) {
  // Gateway nexthop that has no match in the first VRF's route table but
  // matches a route in the second VRF. The updater should resolve via the
  // second VRF.
  IPv4NetworkToRouteMap v4Routes2;
  IPv6NetworkToRouteMap v6Routes2;

  const auto routeNhops = makeResolvedNhops(
      {{"fe80::1", InterfaceID(1)}, {"fe80::2", InterfaceID(2)}});
  auto v6Route =
      makeResolvedV6Route(folly::IPAddressV6("2001:db8::"), 32, routeNhops);
  // Route lives only in the second VRF's table.
  v6Routes2.insert(v6Route->prefix(), v6Route);

  const RouteNextHopSet unresolvedNhops{
      UnresolvedNextHop(folly::IPAddress("2001:db8::1"), ECMP_WEIGHT)};
  const auto unresolvedId = allocUnresolvedSet(unresolvedNhops);

  auto mySid = makeMySid("fc00:100::1", 48);
  mySid->setUnresolveNextHopsId(unresolvedId);
  const folly::CIDRNetworkV6 key{folly::IPAddressV6("fc00:100::1"), 48};
  mySidTable_[key] = mySid;

  // Pass two VRFs: first has empty tables, second has the matching route.
  RibMySidUpdater updater(
      {{&v4Routes_, &v6Routes_}, {&v4Routes2, &v6Routes2}},
      &manager(),
      &mySidTable_);
  updater.resolve();

  const auto resolvedId = mySidTable_.at(key)->getResolvedNextHopsId();
  ASSERT_TRUE(resolvedId.has_value());
  const auto resolvedNhops = manager().getNextHopsIf(*resolvedId);
  ASSERT_TRUE(resolvedNhops.has_value());
  EXPECT_EQ(resolvedNhops->size(), routeNhops.size());
}

TEST_F(RibMySidUpdaterTest, multiVrf_nhopMatchInFirstVrf_firstVrfWins) {
  // When both VRFs have a matching route, the first VRF's route should be used.
  IPv4NetworkToRouteMap v4Routes2;
  IPv6NetworkToRouteMap v6Routes2;

  const auto nhops1 = makeResolvedNhops({{"fe80::1", InterfaceID(1)}});
  const auto nhops2 = makeResolvedNhops({{"fe80::2", InterfaceID(2)}});
  auto v6Route1 =
      makeResolvedV6Route(folly::IPAddressV6("2001:db8::"), 32, nhops1);
  auto v6Route2 =
      makeResolvedV6Route(folly::IPAddressV6("2001:db8::"), 32, nhops2);
  v6Routes_.insert(v6Route1->prefix(), v6Route1);
  v6Routes2.insert(v6Route2->prefix(), v6Route2);

  const RouteNextHopSet unresolvedNhops{
      UnresolvedNextHop(folly::IPAddress("2001:db8::1"), ECMP_WEIGHT)};
  const auto unresolvedId = allocUnresolvedSet(unresolvedNhops);

  auto mySid = makeMySid("fc00:100::1", 48);
  mySid->setUnresolveNextHopsId(unresolvedId);
  const folly::CIDRNetworkV6 key{folly::IPAddressV6("fc00:100::1"), 48};
  mySidTable_[key] = mySid;

  RibMySidUpdater updater(
      {{&v4Routes_, &v6Routes_}, {&v4Routes2, &v6Routes2}},
      &manager(),
      &mySidTable_);
  updater.resolve();

  const auto resolvedId = mySidTable_.at(key)->getResolvedNextHopsId();
  ASSERT_TRUE(resolvedId.has_value());
  const auto resolvedNhops = manager().getNextHopsIf(*resolvedId);
  ASSERT_TRUE(resolvedNhops.has_value());
  // Should resolve via the first VRF's route (fe80::1 on intf 1, not fe80::2).
  ASSERT_EQ(resolvedNhops->size(), 1);
  EXPECT_EQ(resolvedNhops->begin()->addr(), folly::IPAddress("fe80::1"));
}

} // namespace facebook::fboss
