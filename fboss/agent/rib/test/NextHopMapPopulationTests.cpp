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

#include "fboss/agent/SwitchIdScopeResolver.h"
#include "fboss/agent/rib/ForwardingInformationBaseUpdater.h"
#include "fboss/agent/rib/NetworkToRouteMap.h"
#include "fboss/agent/rib/NextHopIDManager.h"
#include "fboss/agent/state/FibInfo.h"
#include "fboss/agent/state/FibInfoMap.h"
#include "fboss/agent/state/ForwardingInformationBase.h"
#include "fboss/agent/state/ForwardingInformationBaseContainer.h"
#include "fboss/agent/state/ForwardingInformationBaseMap.h"
#include "fboss/agent/state/NextHopIdMaps.h"
#include "fboss/agent/state/Route.h"
#include "fboss/agent/state/RouteNextHopEntry.h"
#include "fboss/agent/state/RouteTypes.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/test/TestUtils.h"
#include "fboss/agent/types.h"

#include <folly/IPAddress.h>
#include "fboss/agent/AddressUtil.h"

namespace facebook::fboss {

namespace {

std::map<int64_t, cfg::SwitchInfo> getTestSwitchInfo() {
  std::map<int64_t, cfg::SwitchInfo> map;
  cfg::SwitchInfo info{};
  info.switchType() = cfg::SwitchType::NPU;
  info.asicType() = cfg::AsicType::ASIC_TYPE_FAKE;
  info.switchIndex() = 0;
  map.emplace(10, info);
  return map;
}

const SwitchIdScopeResolver* scopeResolver() {
  static const SwitchIdScopeResolver kSwitchIdScopeResolver(
      getTestSwitchInfo());
  return &kSwitchIdScopeResolver;
}

HwSwitchMatcher scope() {
  return HwSwitchMatcher{std::unordered_set<SwitchID>{SwitchID(10)}};
}

std::shared_ptr<SwitchState> createInitialState(RouterID vrf) {
  auto v4Fib = std::make_shared<ForwardingInformationBaseV4>();
  auto v6Fib = std::make_shared<ForwardingInformationBaseV6>();

  auto fibContainer = std::make_shared<ForwardingInformationBaseContainer>(vrf);
  fibContainer->ref<switch_state_tags::fibV4>() = v4Fib;
  fibContainer->ref<switch_state_tags::fibV6>() = v6Fib;

  auto fibsMap = std::make_shared<ForwardingInformationBaseMap>();
  fibsMap->updateForwardingInformationBaseContainer(fibContainer);

  auto fibInfo = std::make_shared<FibInfo>();
  fibInfo->ref<switch_state_tags::fibsMap>() = fibsMap;

  auto fibInfoMap = std::make_shared<MultiSwitchFibInfoMap>();
  fibInfoMap->updateFibInfo(fibInfo, scope());

  auto state = std::make_shared<SwitchState>();
  state->resetFibsInfoMap(fibInfoMap);
  return state;
}

std::shared_ptr<FibInfo> getFibInfo(const std::shared_ptr<SwitchState>& state) {
  return state->getFibsInfoMap()->getFibInfo(scope());
}

} // namespace

class NextHopMapPopulationTest : public ::testing::Test {
 public:
  void SetUp() override {
    vrf_ = RouterID(0);
    state_ = createInitialState(vrf_);
    state_->publish();
    nextHopIDManager_ = std::make_unique<NextHopIDManager>();
  }

 protected:
  void runFibUpdater() {
    ForwardingInformationBaseUpdater updater(
        scopeResolver(),
        vrf_,
        v4Rib_,
        v6Rib_,
        labelRib_,
        nextHopIDManager_.get());
    state_ = updater(state_);
  }

  void addV4Route(
      const std::string& prefixStr,
      const std::vector<std::string>& nexthopStrs) {
    auto prefix = makePrefixV4(prefixStr);
    RouteNextHopSet nhops;
    for (const auto& nhStr : nexthopStrs) {
      nhops.emplace(ResolvedNextHop(
          folly::IPAddress(nhStr), InterfaceID(1), UCMP_DEFAULT_WEIGHT));
    }
    auto route = std::make_shared<Route<folly::IPAddressV4>>(
        Route<folly::IPAddressV4>::makeThrift(prefix));
    route->update(
        ClientID::BGPD, RouteNextHopEntry(nhops, AdminDistance::EBGP));
    route->setResolved(RouteNextHopEntry(nhops, AdminDistance::EBGP));
    route->publish();
    v4Rib_.insert(prefix, route);
  }

  void addV6Route(
      const std::string& prefixStr,
      const std::vector<std::string>& nexthopStrs) {
    auto prefix = makePrefixV6(prefixStr);
    RouteNextHopSet nhops;
    for (const auto& nhStr : nexthopStrs) {
      nhops.emplace(ResolvedNextHop(
          folly::IPAddress(nhStr), InterfaceID(1), UCMP_DEFAULT_WEIGHT));
    }
    auto route = std::make_shared<Route<folly::IPAddressV6>>(
        Route<folly::IPAddressV6>::makeThrift(prefix));
    route->update(
        ClientID::BGPD, RouteNextHopEntry(nhops, AdminDistance::EBGP));
    route->setResolved(RouteNextHopEntry(nhops, AdminDistance::EBGP));
    route->publish();
    v6Rib_.insert(prefix, route);
  }

  void removeV4Route(const std::string& prefixStr) {
    auto prefix = makePrefixV4(prefixStr);
    auto iter = v4Rib_.exactMatch(prefix.network(), prefix.mask());
    if (iter != v4Rib_.end()) {
      v4Rib_.erase(iter);
    }
  }

  void removeV6Route(const std::string& prefixStr) {
    auto prefix = makePrefixV6(prefixStr);
    auto iter = v6Rib_.exactMatch(prefix.network(), prefix.mask());
    if (iter != v6Rib_.end()) {
      v6Rib_.erase(iter);
    }
  }

  void addV4DropRoute(const std::string& prefixStr) {
    auto prefix = makePrefixV4(prefixStr);
    auto route = std::make_shared<Route<folly::IPAddressV4>>(
        Route<folly::IPAddressV4>::makeThrift(prefix));
    route->update(
        ClientID::BGPD,
        RouteNextHopEntry(RouteForwardAction::DROP, AdminDistance::EBGP));
    route->setResolved(
        RouteNextHopEntry(RouteForwardAction::DROP, AdminDistance::EBGP));
    route->publish();
    v4Rib_.insert(prefix, route);
  }

  void addV6DropRoute(const std::string& prefixStr) {
    auto prefix = makePrefixV6(prefixStr);
    auto route = std::make_shared<Route<folly::IPAddressV6>>(
        Route<folly::IPAddressV6>::makeThrift(prefix));
    route->update(
        ClientID::BGPD,
        RouteNextHopEntry(RouteForwardAction::DROP, AdminDistance::EBGP));
    route->setResolved(
        RouteNextHopEntry(RouteForwardAction::DROP, AdminDistance::EBGP));
    route->publish();
    v6Rib_.insert(prefix, route);
  }

  void addV4ToCpuRoute(const std::string& prefixStr) {
    auto prefix = makePrefixV4(prefixStr);
    auto route = std::make_shared<Route<folly::IPAddressV4>>(
        Route<folly::IPAddressV4>::makeThrift(prefix));
    route->update(
        ClientID::BGPD,
        RouteNextHopEntry(RouteForwardAction::TO_CPU, AdminDistance::EBGP));
    route->setResolved(
        RouteNextHopEntry(RouteForwardAction::TO_CPU, AdminDistance::EBGP));
    route->publish();
    v4Rib_.insert(prefix, route);
  }

  void addV6ToCpuRoute(const std::string& prefixStr) {
    auto prefix = makePrefixV6(prefixStr);
    auto route = std::make_shared<Route<folly::IPAddressV6>>(
        Route<folly::IPAddressV6>::makeThrift(prefix));
    route->update(
        ClientID::BGPD,
        RouteNextHopEntry(RouteForwardAction::TO_CPU, AdminDistance::EBGP));
    route->setResolved(
        RouteNextHopEntry(RouteForwardAction::TO_CPU, AdminDistance::EBGP));
    route->publish();
    v6Rib_.insert(prefix, route);
  }

  std::shared_ptr<IdToNextHopMap> getIdToNextHopMap() {
    auto fibInfo = getFibInfo(state_);
    return fibInfo ? fibInfo->getIdToNextHopMap() : nullptr;
  }

  std::shared_ptr<IdToNextHopIdSetMap> getIdToNextHopIdSetMap() {
    auto fibInfo = getFibInfo(state_);
    return fibInfo ? fibInfo->getIdToNextHopIdSetMap() : nullptr;
  }

  // Verifies that the ID maps in switch state match the ID maps in the
  // NextHopIDManager
  void verifyIdMapsMatchIdManager() {
    auto stateIdToNextHopMap = getIdToNextHopMap();
    auto stateIdToNextHopIdSetMap = getIdToNextHopIdSetMap();
    const auto& managerIdToNextHop = nextHopIDManager_->getIdToNextHop();
    const auto& managerIdToNextHopIdSet =
        nextHopIDManager_->getIdToNextHopIdSet();

    // Verify sizes match
    if (stateIdToNextHopMap) {
      EXPECT_EQ(stateIdToNextHopMap->size(), managerIdToNextHop.size());
    } else {
      EXPECT_EQ(managerIdToNextHop.size(), 0);
    }

    if (stateIdToNextHopIdSetMap) {
      EXPECT_EQ(
          stateIdToNextHopIdSetMap->size(), managerIdToNextHopIdSet.size());
    } else {
      EXPECT_EQ(managerIdToNextHopIdSet.size(), 0);
    }

    // Verify each NextHop in manager exists in state with same value
    for (const auto& [id, nextHop] : managerIdToNextHop) {
      auto stateNextHop = stateIdToNextHopMap->getNextHopIf(id);
      ASSERT_NE(stateNextHop, nullptr)
          << "NextHop id " << id << " not in state";
      auto stateNextHopThrift = stateNextHop->toThrift();
      EXPECT_EQ(
          network::toIPAddress(*stateNextHopThrift.address()), nextHop.addr())
          << "NextHop address mismatch for id " << id;
    }

    // Verify each NextHopIdSet in manager exists in state with same value
    for (const auto& [setId, nextHopIdSet] : managerIdToNextHopIdSet) {
      auto stateNextHopIdSet =
          stateIdToNextHopIdSetMap->getNextHopIdSetIf(setId);
      ASSERT_NE(stateNextHopIdSet, nullptr)
          << "NextHopIdSet id " << setId << " not in state";

      // Convert manager's NextHopIdSet to a set for comparison
      std::set<NextHopId> managerSet;
      for (const auto& nhId : nextHopIdSet) {
        managerSet.insert(nhId);
      }

      // Convert state's NextHopIdSet to a set for comparison
      // Iterate directly; elements are wrapped, so unwrap with .toThrift()
      std::set<NextHopId> stateSet;
      for (const auto& elem : *stateNextHopIdSet) {
        stateSet.insert((*elem).toThrift());
      }

      EXPECT_EQ(stateSet, managerSet)
          << "NextHopIdSet mismatch for setId " << setId;
    }
  }

  // Adds standard test routes
  // - 2 routes with 3 shared nexthops (same ECMP group)
  // - 1 route with single nexthop
  // - 1 route with 5 nexthops
  // - 3 routes with partial overlap (3 nexthops each)
  // - 1 route with DROP action (empty nexthop set, no ID allocated)
  // - 1 route with TO_CPU action (empty nexthop set, no ID allocated)
  // Total: 28 unique nexthops, 12 unique nexthop sets
  // (DROP and TO_CPU routes have empty nexthop sets and don't get IDs)
  void addStandardTestRoutes(bool includeV6 = true) {
    // V4 routes
    // 1. 2 routes with 3 nexthops (same ones) - shared ECMP group
    std::vector<std::string> sharedV4Nexthops = {
        "1.1.1.1", "1.1.1.2", "1.1.1.3"};
    addV4Route("10.0.0.0/24", sharedV4Nexthops);
    addV4Route("10.0.1.0/24", sharedV4Nexthops);

    // 2. 1 route with single nexthop
    addV4Route("10.1.0.0/24", {"2.2.2.1"});

    // 3. 1 route with 5 nexthops
    addV4Route(
        "10.2.0.0/24", {"3.3.3.1", "3.3.3.2", "3.3.3.3", "3.3.3.4", "3.3.3.5"});

    // 4. 3 routes with partial overlap
    addV4Route("10.3.0.0/24", {"4.4.4.1", "4.4.4.2", "4.4.4.3"});
    addV4Route("10.3.1.0/24", {"4.4.4.2", "4.4.4.3", "4.4.4.4"});
    addV4Route("10.3.2.0/24", {"4.4.4.3", "4.4.4.4", "4.4.4.5"});

    // 5. 1 route with DROP action
    addV4DropRoute("10.4.0.0/24");

    // 6. 1 route with TO_CPU action
    addV4ToCpuRoute("10.5.0.0/24");

    if (!includeV6) {
      return;
    }

    // V6 routes (same patterns)
    // 1. 2 routes with 3 nexthops (same ones) - shared ECMP group
    std::vector<std::string> sharedV6Nexthops = {
        "2001:db8::1", "2001:db8::2", "2001:db8::3"};
    addV6Route("2001:db8:1::/64", sharedV6Nexthops);
    addV6Route("2001:db8:2::/64", sharedV6Nexthops);

    // 2. 1 route with single nexthop
    addV6Route("2001:db8:10::/64", {"2001:db8:a::1"});

    // 3. 1 route with 5 nexthops
    addV6Route(
        "2001:db8:20::/64",
        {"2001:db8:b::1",
         "2001:db8:b::2",
         "2001:db8:b::3",
         "2001:db8:b::4",
         "2001:db8:b::5"});

    // 4. 3 routes with partial overlap
    addV6Route(
        "2001:db8:30::/64",
        {"2001:db8:c::1", "2001:db8:c::2", "2001:db8:c::3"});
    addV6Route(
        "2001:db8:31::/64",
        {"2001:db8:c::2", "2001:db8:c::3", "2001:db8:c::4"});
    addV6Route(
        "2001:db8:32::/64",
        {"2001:db8:c::3", "2001:db8:c::4", "2001:db8:c::5"});

    // 5. 1 route with DROP action
    addV6DropRoute("2001:db8:40::/64");

    // 6. 1 route with TO_CPU action
    addV6ToCpuRoute("2001:db8:50::/64");
  }

  void verifyRouteNextHopsInternal(
      const RouteNextHopEntry& fwdInfo,
      const std::string& prefixStr,
      const std::shared_ptr<IdToNextHopMap>& idToNextHopMap,
      const std::shared_ptr<IdToNextHopIdSetMap>& idToNextHopIdSetMap) {
    auto resolvedSetId = fwdInfo.getResolvedNextHopSetID();
    const auto& nextHopSet = fwdInfo.getNextHopSet();

    // Routes with empty nexthop sets (TO_CPU/DROP) don't get IDs
    if (nextHopSet.empty()) {
      EXPECT_FALSE(resolvedSetId.has_value())
          << prefixStr
          << ": Empty nexthop set should not have resolvedNextHopSetID";
      return;
    }

    // Routes with nexthops should have a resolvedNextHopSetID
    ASSERT_TRUE(resolvedSetId.has_value())
        << prefixStr << ": Missing resolvedNextHopSetID";

    // Look up the NextHopIdSet from the map
    auto nextHopIdSetNode = idToNextHopIdSetMap->getNextHopIdSetIf(
        static_cast<NextHopSetId>(*resolvedSetId));
    ASSERT_NE(nextHopIdSetNode, nullptr)
        << prefixStr << ": NextHopIdSet for setId " << *resolvedSetId
        << " not found";

    // Collect the IP addresses from the NextHop map
    // Iterate directly; elements are wrapped, so unwrap with .toThrift()
    std::set<std::string> retrievedNexthops;
    for (const auto& elem : *nextHopIdSetNode) {
      auto nhId = (*elem).toThrift();
      auto nextHopNode = idToNextHopMap->getNextHopIf(nhId);
      ASSERT_NE(nextHopNode, nullptr)
          << prefixStr << ": NextHop for id " << nhId << " not found";

      // Extract IP address from the NextHop
      auto nextHopThrift = nextHopNode->toThrift();
      auto addr = network::toIPAddress(*nextHopThrift.address());
      retrievedNexthops.insert(addr.str());
    }

    // Compare with expected nexthops from the route's forward info
    std::set<std::string> expectedNexthops;
    for (const auto& nhop : fwdInfo.getNextHopSet()) {
      expectedNexthops.insert(nhop.addr().str());
    }

    EXPECT_EQ(retrievedNexthops, expectedNexthops)
        << "NextHops mismatch for route " << prefixStr;
  }

  // This verifies if the NextHop IDs present in the FIB resolves to the correct
  // set of NextHops in the route. This will resolve the ID to set of NextHops
  // and then compare it with the NextHopSet present in the route.
  void verifyIDMapsConsistency() {
    auto fibInfo = getFibInfo(state_);
    ASSERT_NE(fibInfo, nullptr) << "FibInfo is null";

    auto fibContainer = fibInfo->getFibContainerIf(vrf_);
    ASSERT_NE(fibContainer, nullptr) << "FibContainer is null";

    auto idToNextHopMap = getIdToNextHopMap();
    auto idToNextHopIdSetMap = getIdToNextHopIdSetMap();

    // Verify all V4 routes
    auto fibV4 = fibContainer->getFibV4();
    for (const auto& [_, route] : std::as_const(*fibV4)) {
      auto prefix = route->prefix();
      std::string prefixStr =
          folly::sformat("{}/{}", prefix.network().str(), prefix.mask());
      const auto& fwdInfo = route->getForwardInfo();
      verifyRouteNextHopsInternal(
          fwdInfo, prefixStr, idToNextHopMap, idToNextHopIdSetMap);
    }

    // Verify all V6 routes
    auto fibV6 = fibContainer->getFibV6();
    for (const auto& [_, route] : std::as_const(*fibV6)) {
      auto prefix = route->prefix();
      std::string prefixStr =
          folly::sformat("{}/{}", prefix.network().str(), prefix.mask());
      const auto& fwdInfo = route->getForwardInfo();
      verifyRouteNextHopsInternal(
          fwdInfo, prefixStr, idToNextHopMap, idToNextHopIdSetMap);
    }
  }

  RouterID vrf_;
  std::shared_ptr<SwitchState> state_;
  std::unique_ptr<NextHopIDManager> nextHopIDManager_;
  IPv4NetworkToRouteMap v4Rib_;
  IPv6NetworkToRouteMap v6Rib_;
  LabelToRouteMap labelRib_;
};

TEST_F(NextHopMapPopulationTest, RouteAdditionTestsThroughFIBUpdater) {
  addStandardTestRoutes();
  runFibUpdater();

  verifyIdMapsMatchIdManager();
  verifyIDMapsConsistency();
}

} // namespace facebook::fboss
