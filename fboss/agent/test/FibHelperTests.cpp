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

#include "fboss/agent/FibHelpers.h"
#include "fboss/agent/LookupClassRouteUpdater.h"
#include "fboss/agent/NeighborUpdater.h"
#include "fboss/agent/SwSwitchRouteUpdateWrapper.h"
#include "fboss/agent/state/FibInfo.h"
#include "fboss/agent/state/FibInfoMap.h"
#include "fboss/agent/state/RouteNextHopEntry.h"

#include "fboss/agent/test/HwTestHandle.h"
#include "fboss/agent/test/TestUtils.h"

#include <folly/IPAddressV4.h>
#include <folly/IPAddressV6.h>

DECLARE_bool(enable_nexthop_id_manager);
DECLARE_bool(resolve_nexthops_from_id);

using folly::IPAddressV4;
using folly::IPAddressV6;

namespace facebook::fboss {

namespace {
HwSwitchMatcher scope() {
  return HwSwitchMatcher{std::unordered_set<SwitchID>{SwitchID(0)}};
}
} // namespace

template <typename AddrType, bool ResolveFromId = false>
struct FibHelperTestParams {
  using AddrT = AddrType;
  static constexpr bool resolveFromId = ResolveFromId;
};

template <typename Params>
class FibHelperTest : public ::testing::Test {
 public:
  using Func = std::function<void()>;
  using StateUpdateFn = SwSwitch::StateUpdateFn;
  using AddrT = typename Params::AddrT;
  static constexpr bool hasStandAloneRib = true;
  static constexpr bool resolveFromId = Params::resolveFromId;

  void SetUp() override {
    FLAGS_enable_nexthop_id_manager = true;
    FLAGS_resolve_nexthops_from_id = resolveFromId;
    auto config = testConfigA();
    handle_ = createTestHandle(&config);
    sw_ = handle_->getSw();

    programRoute(kPrefix1());
  }

  void programRoute(const folly::CIDRNetwork& prefix) {
    auto routeUpdater = sw_->getRouteUpdater();
    RouteNextHopSet nexthops{
        UnresolvedNextHop(kIpAddressA(), UCMP_DEFAULT_WEIGHT)};
    routeUpdater.addRoute(
        kRid(),
        prefix.first,
        prefix.second,
        kClientID(),
        RouteNextHopEntry(nexthops, AdminDistance::MAX_ADMIN_DISTANCE));
    routeUpdater.program();
  }

  void programRouteWithNexthops(
      const folly::CIDRNetwork& prefix,
      const std::vector<std::string>& nexthopStrs) {
    auto routeUpdater = sw_->getRouteUpdater();
    RouteNextHopSet nexthops;
    for (const auto& nexthopStr : nexthopStrs) {
      nexthops.emplace(
          UnresolvedNextHop(folly::IPAddress(nexthopStr), ECMP_WEIGHT));
    }
    routeUpdater.addRoute(
        kRid(),
        prefix.first,
        prefix.second,
        kClientID(),
        RouteNextHopEntry(nexthops, AdminDistance::EBGP));
    routeUpdater.program();
  }

  std::shared_ptr<FibInfo> getFibInfo() {
    auto state = sw_->getState();
    return state->getFibsInfoMap()->getFibInfo(scope());
  }

  void updateState(folly::StringPiece name, StateUpdateFn func) {
    sw_->updateStateBlocking(name, func);
  }

  VlanID kVlan() const {
    return VlanID(1);
  }

  PortID kPortID() const {
    return PortID(1);
  }

  ClientID kClientID() const {
    return ClientID(1);
  }

  RouterID kRid() const {
    return RouterID{0};
  }

  InterfaceID kInterfaceID() const {
    return InterfaceID(1);
  }

  MacAddress kMacAddressA() const {
    return MacAddress("01:02:03:04:05:06");
  }

  AddrT kIpAddressA() {
    if constexpr (std::is_same_v<AddrT, folly::IPAddressV4>) {
      return IPAddressV4("10.0.0.2");
    } else {
      return IPAddressV6("2401:db00:2110:3001::0002");
    }
  }

  AddrT kIpAddressB() {
    if constexpr (std::is_same_v<AddrT, folly::IPAddressV4>) {
      return IPAddressV4("10.0.0.3");
    } else {
      return IPAddressV6("2401:db00:2110:3001::0003");
    }
  }

  folly::CIDRNetwork kPrefix1() const {
    if constexpr (std::is_same_v<AddrT, folly::IPAddressV4>) {
      return {folly::IPAddressV4{"10.1.4.0"}, 24};
    } else {
      return {folly::IPAddressV6{"2803:6080:d038:3066::"}, 64};
    }
  }
  AddrT kInSubnetIp() const {
    if constexpr (std::is_same_v<AddrT, folly::IPAddressV4>) {
      return folly::IPAddressV4{"10.1.4.1"};
    } else {
      return folly::IPAddressV6{"2803:6080:d038:3066::1"};
    }
  }
  AddrT kNotInSubnetIp() const {
    if constexpr (std::is_same_v<AddrT, folly::IPAddressV4>) {
      return folly::IPAddressV4{"10.1.5.1"};
    } else {
      return folly::IPAddressV6{"2803:6080:d038:4066::1"};
    }
  }

  folly::CIDRNetwork kPrefix2() const {
    if constexpr (std::is_same_v<AddrT, folly::IPAddressV4>) {
      return {folly::IPAddressV4{"11.1.4.0"}, 24};
    } else {
      return {folly::IPAddressV6{"2803:6080:d038:3067::"}, 64};
    }
  }

  std::shared_ptr<ForwardingInformationBase<AddrT>> getFib(
      const std::shared_ptr<ForwardingInformationBaseContainer>& fibContainer) {
    if constexpr (std::is_same_v<AddrT, folly::IPAddressV4>) {
      return fibContainer->getFibV4();
    } else {
      return fibContainer->getFibV6();
    }
  }

 protected:
  std::unique_ptr<HwTestHandle> handle_;
  SwSwitch* sw_;
};

using FibHelperTestTypes = ::testing::Types<
    FibHelperTestParams<folly::IPAddressV4, false>,
    FibHelperTestParams<folly::IPAddressV4, true>,
    FibHelperTestParams<folly::IPAddressV6, false>,
    FibHelperTestParams<folly::IPAddressV6, true>>;

TYPED_TEST_CASE(FibHelperTest, FibHelperTestTypes);

TYPED_TEST(FibHelperTest, findRoute) {
  auto route = findRoute<typename TestFixture::AddrT>(
      this->kRid(), this->kPrefix1(), this->sw_->getState());
  EXPECT_NE(route, nullptr);

  route = findRoute<typename TestFixture::AddrT>(
      this->kRid(), this->kPrefix2(), this->sw_->getState());
  EXPECT_EQ(route, nullptr);
}

TYPED_TEST(FibHelperTest, findLongestMatchRoute) {
  auto route = findRoute<typename TestFixture::AddrT>(
      this->kRid(), this->kPrefix1(), this->sw_->getState());
  EXPECT_NE(route, nullptr);
  auto route2 = findLongestMatchRoute<typename TestFixture::AddrT>(
      this->sw_->getRib(),
      this->kRid(),
      this->kInSubnetIp(),
      this->sw_->getState());

  EXPECT_EQ(route, route2);

  auto route3 = findLongestMatchRoute<typename TestFixture::AddrT>(
      this->sw_->getRib(),
      this->kRid(),
      this->kNotInSubnetIp(),
      this->sw_->getState());
  EXPECT_NE(route2, route3);
}

TYPED_TEST(FibHelperTest, forAllRoutes) {
  int count = 0;
  auto countRoutes = [&count](RouterID rid, auto& route) { ++count; };
  forAllRoutes(this->sw_->getState(), countRoutes);
  auto prevCount = count;
  // Program prefix1 again, should not change the count
  this->programRoute(this->kPrefix1());
  count = 0;
  forAllRoutes(this->sw_->getState(), countRoutes);
  EXPECT_EQ(count, prevCount);
  // Add one more route, now count should increment
  this->programRoute(this->kPrefix2());
  count = 0;
  forAllRoutes(this->sw_->getState(), countRoutes);
  EXPECT_EQ(count, prevCount + 1);
}

TYPED_TEST(FibHelperTest, getNexthopsResolvesRouteIds) {
  // Route with two nexthops
  this->programRouteWithNexthops(
      this->kPrefix2(), {this->kIpAddressA().str(), this->kIpAddressB().str()});

  auto state = this->sw_->getState();
  auto fibInfo = this->getFibInfo();
  ASSERT_NE(fibInfo, nullptr);

  auto fibContainer = fibInfo->getFibContainerIf(this->kRid());
  ASSERT_NE(fibContainer, nullptr);

  // Verify routes can be resolved using getNextHops
  auto fib = this->getFib(fibContainer);
  for (const auto& [_, route] : std::as_const(*fib)) {
    const auto& fwdInfo = route->getForwardInfo();
    auto resolvedSetId = fwdInfo.getResolvedNextHopSetID();
    const auto& expectedNextHopSet = getNextHops(state, fwdInfo);

    // Skip routes without nexthops (TO_CPU/DROP actions)
    if (expectedNextHopSet.empty()) {
      continue;
    }

    ASSERT_TRUE(resolvedSetId.has_value())
        << "Route should have resolvedNextHopSetID";

    // Use the FibHelper getNextHops function to resolve the ID
    auto resolvedNextHops =
        getNextHops(state, static_cast<NextHopSetId>(*resolvedSetId));

    // Sort for comparison
    std::sort(resolvedNextHops.begin(), resolvedNextHops.end());
    std::vector<NextHop> expectedNextHops(
        expectedNextHopSet.begin(), expectedNextHopSet.end());

    EXPECT_EQ(resolvedNextHops, expectedNextHops)
        << "NextHops mismatch for route " << route->prefix().str();
  }
}

TYPED_TEST(FibHelperTest, getNexthopsInvalidIdThrows) {
  auto state = this->sw_->getState();

  // Try to resolve a non-existent NextHopSetId
  NextHopSetId invalidId = 999999;
  EXPECT_THROW(getNextHops(state, invalidId), FbossError);
}

TYPED_TEST(FibHelperTest, getNextHopsFromEntry) {
  this->programRouteWithNexthops(
      this->kPrefix2(), {this->kIpAddressA().str(), this->kIpAddressB().str()});

  auto state = this->sw_->getState();
  auto fibContainer = this->getFibInfo()->getFibContainerIf(this->kRid());
  auto fib = this->getFib(fibContainer);

  for (const auto& [_, route] : std::as_const(*fib)) {
    const auto& fwdInfo = route->getForwardInfo();
    if (fwdInfo.getNextHopSet().empty()) {
      continue;
    }
    auto result = getNextHops(state, fwdInfo);
    EXPECT_EQ(result, fwdInfo.getNextHopSet());
  }
}

TYPED_TEST(FibHelperTest, getNonOverrideNormalizedNextHopsFromEntry) {
  this->programRouteWithNexthops(
      this->kPrefix2(), {this->kIpAddressA().str(), this->kIpAddressB().str()});

  auto state = this->sw_->getState();
  auto fibContainer = this->getFibInfo()->getFibContainerIf(this->kRid());
  auto fib = this->getFib(fibContainer);

  for (const auto& [_, route] : std::as_const(*fib)) {
    const auto& fwdInfo = route->getForwardInfo();
    if (fwdInfo.getNextHopSet().empty()) {
      continue;
    }
    auto result = getNonOverrideNormalizedNextHops(state, fwdInfo);
    EXPECT_EQ(result, fwdInfo.nonOverrideNormalizedNextHops());
  }
}

TYPED_TEST(FibHelperTest, getNewStateWithOldFibInfoPreservesOldFibs) {
  using AddrT = typename TestFixture::AddrT;
  // Program routes into oldState
  this->programRouteWithNexthops(
      this->kPrefix2(), {this->kIpAddressA().str(), this->kIpAddressB().str()});
  auto oldState = this->sw_->getState();

  // Program a different route into newState
  this->programRoute(this->kPrefix2());
  auto newState = this->sw_->getState();

  auto result = getNewStateWithOldFibInfo(oldState, newState);

  // Every route in the result's FIB should be present in the old state's FIB
  // with matching nexthops
  auto resultFibContainer =
      result->getFibsInfoMap()->getFibInfo(scope())->getFibContainerIf(
          this->kRid());
  ASSERT_NE(resultFibContainer, nullptr);
  auto resultFib = this->getFib(resultFibContainer);
  for (const auto& [_, resultRoute] : std::as_const(*resultFib)) {
    auto oldRoute = findRoute<AddrT>(
        this->kRid(), resultRoute->prefix().toCidrNetwork(), oldState);
    ASSERT_NE(oldRoute, nullptr) << "Route " << resultRoute->prefix().str()
                                 << " in result but not in oldState";
    EXPECT_EQ(
        resultRoute->getForwardInfo().getNextHopSet(),
        oldRoute->getForwardInfo().getNextHopSet())
        << "Nexthops mismatch for route " << resultRoute->prefix().str();
  }
}

TYPED_TEST(FibHelperTest, getNewStateWithOldFibInfoEmptyOldState) {
  // Create an empty old state (simulates warmboot/rollback)
  auto emptyOldState = std::make_shared<SwitchState>();
  auto newState = this->sw_->getState();

  auto result = getNewStateWithOldFibInfo(emptyOldState, newState);

  // Result should have empty FIBs but matching structure
  auto resultFibsInfoMap = result->getFibsInfoMap();
  ASSERT_NE(resultFibsInfoMap, nullptr);
  auto resultFibInfo = resultFibsInfoMap->getFibInfo(scope());
  ASSERT_NE(resultFibInfo, nullptr);
  auto resultFibContainer = resultFibInfo->getFibContainerIf(this->kRid());
  ASSERT_NE(resultFibContainer, nullptr);
  auto resultFib = this->getFib(resultFibContainer);
  EXPECT_EQ(resultFib->size(), 0);
}

TYPED_TEST(FibHelperTest, getNewStateWithOldFibInfoPreservesIdMaps) {
  this->programRouteWithNexthops(
      this->kPrefix2(), {this->kIpAddressA().str(), this->kIpAddressB().str()});
  auto oldState = this->sw_->getState();

  // Modify existing route and add a new route with different nexthops
  // in new state, so newState has different/additional ID map entries
  this->programRoute(this->kPrefix2());
  this->programRouteWithNexthops(
      this->kPrefix1(), {this->kIpAddressA().str(), this->kIpAddressB().str()});
  auto newState = this->sw_->getState();

  auto result = getNewStateWithOldFibInfo(oldState, newState);

  // ID maps should come from old state — compare every entry
  auto oldFibInfo = oldState->getFibsInfoMap()->getFibInfo(scope());
  auto resultFibInfo = result->getFibsInfoMap()->getFibInfo(scope());

  // Compare idToNextHopIdSetMap
  auto oldIdToSetMap = oldFibInfo->getIdToNextHopIdSetMap();
  auto resultIdToSetMap = resultFibInfo->getIdToNextHopIdSetMap();
  ASSERT_NE(resultIdToSetMap, nullptr);
  EXPECT_EQ(resultIdToSetMap->size(), oldIdToSetMap->size());
  for (const auto& [setId, resultSetNode] : std::as_const(*resultIdToSetMap)) {
    auto oldSetNode = oldIdToSetMap->getNextHopIdSetIf(setId);
    ASSERT_NE(oldSetNode, nullptr)
        << "NextHopSetId " << setId << " in result but not in oldState";
    std::set<NextHopId> oldNhIds, resultNhIds;
    for (const auto& elem : std::as_const(*oldSetNode)) {
      oldNhIds.insert((*elem).toThrift());
    }
    for (const auto& elem : std::as_const(*resultSetNode)) {
      resultNhIds.insert((*elem).toThrift());
    }
    EXPECT_EQ(resultNhIds, oldNhIds)
        << "NextHopId set mismatch for NextHopSetId " << setId;
  }

  // Compare idToNextHopMap
  auto oldIdToNhMap = oldFibInfo->getIdToNextHopMap();
  auto resultIdToNhMap = resultFibInfo->getIdToNextHopMap();
  ASSERT_NE(resultIdToNhMap, nullptr);
  EXPECT_EQ(resultIdToNhMap->size(), oldIdToNhMap->size());
  for (const auto& [nhId, resultNhNode] : std::as_const(*resultIdToNhMap)) {
    auto oldNhNode = oldIdToNhMap->getNextHopIf(nhId);
    ASSERT_NE(oldNhNode, nullptr)
        << "NextHopId " << nhId << " in result but not in oldState";
    EXPECT_EQ(resultNhNode->toThrift(), oldNhNode->toThrift())
        << "NextHop mismatch for NextHopId " << nhId;
  }
}

TYPED_TEST(FibHelperTest, populateIdMapsForRouteCopiesIds) {
  // Program a route with two nexthops to get ID maps populated
  this->programRouteWithNexthops(
      this->kPrefix2(), {this->kIpAddressA().str(), this->kIpAddressB().str()});
  auto sourceState = this->sw_->getState();

  // Create empty mutable maps (simulating what ERM's initCachedIdMaps does)
  auto dstIdToSetMap = std::make_shared<IdToNextHopIdSetMap>();
  auto dstIdToNhMap = std::make_shared<IdToNextHopMap>();

  auto route = findRoute<typename TestFixture::AddrT>(
      this->kRid(), this->kPrefix2(), sourceState);
  ASSERT_NE(route, nullptr);

  // Populate using mutable maps overload
  populateIdMapsForRoute(route, sourceState, dstIdToSetMap, dstIdToNhMap);

  // Verify the route's IDs can be resolved from the populated maps
  const auto& fwdInfo = route->getForwardInfo();
  auto resolvedSetId = fwdInfo.getResolvedNextHopSetID();
  ASSERT_TRUE(resolvedSetId.has_value());
  EXPECT_NE(
      dstIdToSetMap->getNextHopIdSetIf(
          static_cast<NextHopSetId>(*resolvedSetId)),
      nullptr);
  EXPECT_GT(dstIdToNhMap->size(), 0);

  // Program a second route with different nexthops, populate into same maps
  this->programRouteWithNexthops(this->kPrefix1(), {this->kIpAddressB().str()});
  auto updatedSourceState = this->sw_->getState();
  auto route2 = findRoute<typename TestFixture::AddrT>(
      this->kRid(), this->kPrefix1(), updatedSourceState);
  ASSERT_NE(route2, nullptr);

  populateIdMapsForRoute(
      route2, updatedSourceState, dstIdToSetMap, dstIdToNhMap);

  // Second route's IDs should also be resolvable from the same maps
  auto resolvedSetId2 = route2->getForwardInfo().getResolvedNextHopSetID();
  ASSERT_TRUE(resolvedSetId2.has_value());
  EXPECT_NE(
      dstIdToSetMap->getNextHopIdSetIf(
          static_cast<NextHopSetId>(*resolvedSetId2)),
      nullptr);
}

TYPED_TEST(FibHelperTest, populateIdMapsForRouteNoOps) {
  // Test 1: Skip when IDs already exist in destination
  this->programRouteWithNexthops(
      this->kPrefix2(), {this->kIpAddressA().str(), this->kIpAddressB().str()});
  auto sourceState = this->sw_->getState();

  // Clone maps from source (IDs already present)
  auto srcFibInfo = sourceState->getFibsInfoMap()->getFibInfo(scope());
  auto dstIdToSetMap = srcFibInfo->getIdToNextHopIdSetMap()->clone();
  auto dstIdToNhMap = srcFibInfo->getIdToNextHopMap()->clone();

  auto route = findRoute<typename TestFixture::AddrT>(
      this->kRid(), this->kPrefix2(), sourceState);
  ASSERT_NE(route, nullptr);

  auto sizeBefore = dstIdToSetMap->size();
  populateIdMapsForRoute(route, sourceState, dstIdToSetMap, dstIdToNhMap);

  // Size should be unchanged — IDs already exist
  EXPECT_EQ(dstIdToSetMap->size(), sizeBefore);

  // Test 2: Skip DROP routes (no IDs)
  auto routeUpdater = this->sw_->getRouteUpdater();
  routeUpdater.addRoute(
      this->kRid(),
      this->kPrefix2().first,
      this->kPrefix2().second,
      this->kClientID(),
      RouteNextHopEntry(
          RouteForwardAction::DROP, AdminDistance::MAX_ADMIN_DISTANCE));
  routeUpdater.program();

  sourceState = this->sw_->getState();
  auto dropRoute = findRoute<typename TestFixture::AddrT>(
      this->kRid(), this->kPrefix2(), sourceState);
  ASSERT_NE(dropRoute, nullptr);
  EXPECT_FALSE(
      dropRoute->getForwardInfo().getResolvedNextHopSetID().has_value());
  EXPECT_FALSE(dropRoute->getForwardInfo()
                   .getNormalizedResolvedNextHopSetID()
                   .has_value());

  auto emptySetMap = std::make_shared<IdToNextHopIdSetMap>();
  auto emptyNhMap = std::make_shared<IdToNextHopMap>();
  populateIdMapsForRoute(dropRoute, sourceState, emptySetMap, emptyNhMap);
  EXPECT_EQ(emptySetMap->size(), 0);
  EXPECT_EQ(emptyNhMap->size(), 0);
}

TYPED_TEST(FibHelperTest, syncIdMapsFromStateCopiesMaps) {
  // Program routes to populate ID maps in source state
  this->programRouteWithNexthops(
      this->kPrefix2(), {this->kIpAddressA().str(), this->kIpAddressB().str()});
  auto sourceState = this->sw_->getState();

  // Create a destination state with empty ID maps
  auto dstState = sourceState->clone();
  auto dstFibInfo =
      dstState->getFibsInfoMap()->getFibInfo(scope())->modify(&dstState);
  dstFibInfo->setIdToNextHopIdSetMap(nullptr);
  dstFibInfo->setIdToNextHopMap(nullptr);

  auto result = syncIdMapsFromState(sourceState, dstState);

  // Verify ID maps match source
  auto srcFibInfo = sourceState->getFibsInfoMap()->getFibInfo(scope());
  auto resultFibInfo = result->getFibsInfoMap()->getFibInfo(scope());
  EXPECT_EQ(
      resultFibInfo->getIdToNextHopIdSetMap(),
      srcFibInfo->getIdToNextHopIdSetMap());
  EXPECT_EQ(
      resultFibInfo->getIdToNextHopMap(), srcFibInfo->getIdToNextHopMap());
}

TYPED_TEST(FibHelperTest, syncIdMapsFromStateClearsWhenSourceEmpty) {
  // Source has no ID maps, destination has populated maps — should clear
  this->programRouteWithNexthops(
      this->kPrefix2(), {this->kIpAddressA().str(), this->kIpAddressB().str()});
  auto dstState = this->sw_->getState();

  // Create a source state with cleared ID maps
  auto sourceState = dstState->clone();
  auto srcFibInfo =
      sourceState->getFibsInfoMap()->getFibInfo(scope())->modify(&sourceState);
  srcFibInfo->setIdToNextHopIdSetMap(nullptr);
  srcFibInfo->setIdToNextHopMap(nullptr);

  auto result = syncIdMapsFromState(sourceState, dstState);

  auto resultFibInfo = result->getFibsInfoMap()->getFibInfo(scope());
  EXPECT_EQ(resultFibInfo->getIdToNextHopIdSetMap(), nullptr);
  EXPECT_EQ(resultFibInfo->getIdToNextHopMap(), nullptr);
}

TYPED_TEST(FibHelperTest, getNormalizedNextHopsFromEntry) {
  this->programRouteWithNexthops(
      this->kPrefix2(), {this->kIpAddressA().str(), this->kIpAddressB().str()});
  auto state = this->sw_->getState();
  auto fibContainer = this->getFibInfo()->getFibContainerIf(this->kRid());
  auto fib = this->getFib(fibContainer);
  for (const auto& [_, route] : std::as_const(*fib)) {
    const auto& fwdInfo = route->getForwardInfo();
    if (fwdInfo.getNextHopSet().empty()) {
      continue;
    }
    auto result = getNormalizedNextHops(state, fwdInfo);
    EXPECT_EQ(result, fwdInfo.normalizedNextHops());
    EXPECT_EQ(result, getNonOverrideNormalizedNextHops(state, fwdInfo));
  }
}

TYPED_TEST(FibHelperTest, getNormalizedNextHopsWithOverrides) {
  this->programRouteWithNexthops(
      this->kPrefix2(), {this->kIpAddressA().str(), this->kIpAddressB().str()});

  // Set override nexthops (subset of original) on kPrefix2 route in state
  RouteNextHopSet overrideNhops;
  overrideNhops.emplace(
      ResolvedNextHop(this->kIpAddressA(), this->kInterfaceID(), 1));
  this->updateState("set overrides", [&](const auto& state) {
    auto newState = state->clone();
    auto route = findRoute<typename TestFixture::AddrT>(
        this->kRid(), this->kPrefix2(), newState);
    auto clonedRoute = route->clone();
    RouteNextHopEntry updatedFwd;
    updatedFwd.fromThrift(clonedRoute->getForwardInfo().toThrift());
    updatedFwd.setOverrideNextHops(overrideNhops);
    clonedRoute->setResolved(updatedFwd);
    auto fib = newState->getFibsInfoMap()
                   ->getFibContainer(this->kRid())
                   ->template getFib<typename TestFixture::AddrT>();
    auto modifiedFib = fib->modify(this->kRid(), &newState);
    modifiedFib->updateNode(clonedRoute);
    return newState;
  });

  // Verify all routes in FIB
  auto state = this->sw_->getState();
  auto fibContainer = this->getFibInfo()->getFibContainerIf(this->kRid());
  auto fib = this->getFib(fibContainer);
  for (const auto& [_, route] : std::as_const(*fib)) {
    const auto& fwdInfo = route->getForwardInfo();
    if (fwdInfo.getNextHopSet().empty()) {
      continue;
    }
    auto result = getNormalizedNextHops(state, fwdInfo);
    EXPECT_EQ(result, fwdInfo.normalizedNextHops());
    auto nonOverrideResult = getNonOverrideNormalizedNextHops(state, fwdInfo);
    EXPECT_EQ(nonOverrideResult, fwdInfo.nonOverrideNormalizedNextHops());

    if (fwdInfo.getOverrideNextHops().has_value()) {
      // This is kPrefix2 — override (A only) should differ from
      // non-override (A and B)
      EXPECT_EQ(result, overrideNhops);
      EXPECT_NE(result, nonOverrideResult);
    }
  }
}

} // namespace facebook::fboss
