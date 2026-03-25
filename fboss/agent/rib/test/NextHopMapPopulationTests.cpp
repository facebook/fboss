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

#include <folly/logging/xlog.h>
#include <gflags/gflags.h>

#include "fboss/agent/AddressUtil.h"
#include "fboss/agent/FibHelpers.h"
#include "fboss/agent/SwSwitchRouteUpdateWrapper.h"
#include "fboss/agent/if/gen-cpp2/ctrl_types.h"
#include "fboss/agent/if/gen-cpp2/mpls_types.h"
#include "fboss/agent/rib/NextHopIDManager.h"
#include "fboss/agent/state/FibInfo.h"
#include "fboss/agent/state/FibInfoMap.h"
#include "fboss/agent/state/LabelForwardingAction.h"
#include "fboss/agent/state/LabelForwardingInformationBase.h"
#include "fboss/agent/state/NextHopIdMaps.h"
#include "fboss/agent/state/RouteNextHopEntry.h"
#include "fboss/agent/state/RouteTypes.h"
#include "fboss/agent/test/HwTestHandle.h"
#include "fboss/agent/test/TestUtils.h"
#include "fboss/agent/types.h"

#include <folly/IPAddress.h>

DECLARE_bool(enable_nexthop_id_manager);

namespace facebook::fboss {

namespace {

constexpr AdminDistance DISTANCE = AdminDistance::EBGP;
const RouterID kRid0 = RouterID(0);
const ClientID kClientA = ClientID(1001);

HwSwitchMatcher scope() {
  return HwSwitchMatcher{std::unordered_set<SwitchID>{SwitchID(0)}};
}

} // namespace

class NextHopMapPopulationTest : public ::testing::Test {
 public:
  void SetUp() override {
    FLAGS_enable_nexthop_id_manager = true;
    auto config = initialConfig();
    handle_ = createTestHandle(&config);
    sw_ = handle_->getSw();
  }

  cfg::SwitchConfig initialConfig() const {
    cfg::SwitchConfig config;
    config.switchSettings()->switchIdToSwitchInfo() = {
        {0, createSwitchInfo(cfg::SwitchType::NPU)}};
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

 protected:
  RouteNextHopSet makeNextHops(
      const std::vector<std::string>& nexthopStrs,
      const std::vector<NextHopWeight>& weights = {}) {
    RouteNextHopSet nhops;
    for (size_t i = 0; i < nexthopStrs.size(); ++i) {
      NextHopWeight weight = weights.empty() ? ECMP_WEIGHT : weights[i];
      nhops.emplace(
          UnresolvedNextHop(folly::IPAddress(nexthopStrs[i]), weight));
    }
    return nhops;
  }

  // Add V4 route with nexthops
  void addV4Route(
      SwSwitchRouteUpdateWrapper& updater,
      const std::string& prefixStr,
      const std::vector<std::string>& nexthopStrs,
      const std::vector<NextHopWeight>& weights = {}) {
    auto prefix = makePrefixV4(prefixStr);
    auto nhops = makeNextHops(nexthopStrs, weights);
    updater.addRoute(
        kRid0,
        prefix.network(),
        prefix.mask(),
        kClientA,
        RouteNextHopEntry(nhops, DISTANCE));
  }

  // Add V6 route with nexthops
  void addV6Route(
      SwSwitchRouteUpdateWrapper& updater,
      const std::string& prefixStr,
      const std::vector<std::string>& nexthopStrs,
      const std::vector<NextHopWeight>& weights = {}) {
    auto prefix = makePrefixV6(prefixStr);
    auto nhops = makeNextHops(nexthopStrs, weights);
    updater.addRoute(
        kRid0,
        prefix.network(),
        prefix.mask(),
        kClientA,
        RouteNextHopEntry(nhops, DISTANCE));
  }

  void addV4DropRoute(
      SwSwitchRouteUpdateWrapper& updater,
      const std::string& prefixStr) {
    auto prefix = makePrefixV4(prefixStr);
    updater.addRoute(
        kRid0,
        prefix.network(),
        prefix.mask(),
        kClientA,
        RouteNextHopEntry(RouteForwardAction::DROP, DISTANCE));
  }

  void addV6DropRoute(
      SwSwitchRouteUpdateWrapper& updater,
      const std::string& prefixStr) {
    auto prefix = makePrefixV6(prefixStr);
    updater.addRoute(
        kRid0,
        prefix.network(),
        prefix.mask(),
        kClientA,
        RouteNextHopEntry(RouteForwardAction::DROP, DISTANCE));
  }

  void addV4ToCpuRoute(
      SwSwitchRouteUpdateWrapper& updater,
      const std::string& prefixStr) {
    auto prefix = makePrefixV4(prefixStr);
    updater.addRoute(
        kRid0,
        prefix.network(),
        prefix.mask(),
        kClientA,
        RouteNextHopEntry(RouteForwardAction::TO_CPU, DISTANCE));
  }

  void addV6ToCpuRoute(
      SwSwitchRouteUpdateWrapper& updater,
      const std::string& prefixStr) {
    auto prefix = makePrefixV6(prefixStr);
    updater.addRoute(
        kRid0,
        prefix.network(),
        prefix.mask(),
        kClientA,
        RouteNextHopEntry(RouteForwardAction::TO_CPU, DISTANCE));
  }

  // Add MPLS route with POP_AND_LOOKUP action.
  // POP_AND_LOOKUP routes have a null address (::) as nexthop since forwarding
  // is based on inner header lookup.
  void addMplsPopAndLookupRoute(
      SwSwitchRouteUpdateWrapper& updater,
      MplsLabel label) {
    MplsRoute mplsRoute;
    mplsRoute.topLabel() = label;
    NextHopThrift nexthop;
    nexthop.mplsAction() = MplsAction();
    *nexthop.mplsAction()->action() = MplsActionCode::POP_AND_LOOKUP;
    folly::IPAddress nullAddr{"::"};
    nexthop.address()->addr()->append(
        reinterpret_cast<const char*>(nullAddr.bytes()),
        folly::IPAddressV6::byteCount());
    mplsRoute.nextHops()->emplace_back(nexthop);
    updater.addRoute(kClientA, mplsRoute);
  }

  std::shared_ptr<MultiLabelForwardingInformationBase> getLabelFib() {
    auto state = sw_->getState();
    return state->getLabelForwardingInformationBase();
  }

  void delV4Route(
      SwSwitchRouteUpdateWrapper& updater,
      const std::string& prefixStr) {
    auto prefix = makePrefixV4(prefixStr);
    updater.delRoute(kRid0, prefix.network(), prefix.mask(), kClientA);
  }

  void delV6Route(
      SwSwitchRouteUpdateWrapper& updater,
      const std::string& prefixStr) {
    auto prefix = makePrefixV6(prefixStr);
    updater.delRoute(kRid0, prefix.network(), prefix.mask(), kClientA);
  }

  std::shared_ptr<FibInfo> getFibInfo() {
    auto state = sw_->getState();
    return state->getFibsInfoMap()->getFibInfo(scope());
  }

  std::shared_ptr<IdToNextHopMap> getIdToNextHopMap() {
    auto fibInfo = getFibInfo();
    return fibInfo ? fibInfo->getIdToNextHopMap() : nullptr;
  }

  std::shared_ptr<IdToNextHopIdSetMap> getIdToNextHopIdSetMap() {
    auto fibInfo = getFibInfo();
    return fibInfo ? fibInfo->getIdToNextHopIdSetMap() : nullptr;
  }

  const NextHopIDManager* getNextHopIDManager() {
    return sw_->getRib()->getNextHopIDManager();
  }

  // Verifies that the ID maps in switch state match the ID maps in the
  // NextHopIDManager
  void verifyIdMapsMatchIdManager() {
    auto stateIdToNextHopMap = getIdToNextHopMap();
    auto stateIdToNextHopIdSetMap = getIdToNextHopIdSetMap();
    auto* manager = getNextHopIDManager();
    const auto& managerIdToNextHop = manager->getIdToNextHop();
    const auto& managerIdToNextHopIdSet = manager->getIdToNextHopIdSet();

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

      // Use toThrift() to get the set without modifying published node
      auto stateSet = stateNextHopIdSet->toThrift();

      EXPECT_EQ(stateSet, managerSet)
          << "NextHopIdSet mismatch for setId " << setId;
    }
  }

  // This verifies if the NextHop IDs present in the FIB resolves to the correct
  // set of NextHops in the route. This will resolve the ID to set of NextHops
  // using the FibHelper getNexthops function and compare it with the NextHopSet
  // present in the route.
  void verifyIDMapsConsistency() {
    auto state = sw_->getState();
    auto fibInfo = getFibInfo();
    ASSERT_NE(fibInfo, nullptr) << "FibInfo is null";

    auto fibContainer = fibInfo->getFibContainerIf(kRid0);
    ASSERT_NE(fibContainer, nullptr) << "FibContainer is null";

    // Verify all V4 routes
    auto fibV4 = fibContainer->getFibV4();
    for (const auto& [_, route] : std::as_const(*fibV4)) {
      auto prefix = route->prefix();
      std::string prefixStr =
          folly::sformat("{}/{}", prefix.network().str(), prefix.mask());
      const auto& fwdInfo = route->getForwardInfo();
      auto resolvedSetId = fwdInfo.getResolvedNextHopSetID();
      const auto& expectedNextHopSet = fwdInfo.getNextHopSet();

      // Routes with empty nexthop sets (TO_CPU/DROP) don't get IDs
      if (expectedNextHopSet.empty()) {
        EXPECT_FALSE(resolvedSetId.has_value())
            << prefixStr
            << ": Empty nexthop set should not have resolvedNextHopSetID";
        continue;
      }

      // Routes with nexthops should have a resolvedNextHopSetID
      ASSERT_TRUE(resolvedSetId.has_value())
          << prefixStr << ": Missing resolvedNextHopSetID";

      // Use FibHelper getNexthops to resolve the ID back to NextHops
      auto resolvedNextHops =
          getNextHops(state, static_cast<NextHopSetId>(*resolvedSetId));

      // Sort and compare as vectors
      std::sort(resolvedNextHops.begin(), resolvedNextHops.end());
      std::vector<NextHop> expectedNextHops(
          expectedNextHopSet.begin(), expectedNextHopSet.end());

      EXPECT_EQ(resolvedNextHops, expectedNextHops)
          << "NextHops mismatch for route " << prefixStr;
    }

    // Verify all V6 routes
    auto fibV6 = fibContainer->getFibV6();
    for (const auto& [_, route] : std::as_const(*fibV6)) {
      auto prefix = route->prefix();
      std::string prefixStr =
          folly::sformat("{}/{}", prefix.network().str(), prefix.mask());
      const auto& fwdInfo = route->getForwardInfo();
      auto resolvedSetId = fwdInfo.getResolvedNextHopSetID();
      const auto& expectedNextHopSet = fwdInfo.getNextHopSet();

      // Routes with empty nexthop sets (TO_CPU/DROP) don't get IDs
      if (expectedNextHopSet.empty()) {
        EXPECT_FALSE(resolvedSetId.has_value())
            << prefixStr
            << ": Empty nexthop set should not have resolvedNextHopSetID";
        continue;
      }

      // Routes with nexthops should have a resolvedNextHopSetID
      ASSERT_TRUE(resolvedSetId.has_value())
          << prefixStr << ": Missing resolvedNextHopSetID";

      // Use FibHelper getNexthops to resolve the ID back to NextHops
      auto resolvedNextHops =
          getNextHops(state, static_cast<NextHopSetId>(*resolvedSetId));

      // Sort and compare as vectors
      std::sort(resolvedNextHops.begin(), resolvedNextHops.end());
      std::vector<NextHop> expectedNextHops(
          expectedNextHopSet.begin(), expectedNextHopSet.end());

      EXPECT_EQ(resolvedNextHops, expectedNextHops)
          << "NextHops mismatch for route " << prefixStr;
    }
  }

  void addStandardTestRoutes(
      SwSwitchRouteUpdateWrapper& updater,
      bool includeV6 = true) {
    // V4 routes
    // 1. 2 routes with 3 nexthops (same ones) - shared ECMP group
    std::vector<std::string> sharedV4Nexthops = {"1.1.1.10", "2.2.2.10"};
    addV4Route(updater, "10.0.0.0/24", sharedV4Nexthops);
    addV4Route(updater, "10.0.1.0/24", sharedV4Nexthops);

    // 2. 1 route with single nexthop
    addV4Route(updater, "10.1.0.0/24", {"2.2.2.20"});

    // 3. 1 route with 4 nexthops
    addV4Route(
        updater,
        "10.2.0.0/24",
        {"1.1.1.10", "2.2.2.10", "3.3.3.10", "4.4.4.10"});

    // 4. 3 routes with partial overlap
    addV4Route(updater, "10.3.0.0/24", {"1.1.1.10", "2.2.2.10", "3.3.3.10"});
    addV4Route(updater, "10.3.1.0/24", {"2.2.2.10", "3.3.3.10", "4.4.4.10"});
    addV4Route(updater, "10.3.2.0/24", {"3.3.3.10", "4.4.4.10", "1.1.1.10"});

    // 5. 1 route with DROP action
    addV4DropRoute(updater, "10.4.0.0/24");

    // 6. 1 route with TO_CPU action
    addV4ToCpuRoute(updater, "10.5.0.0/24");

    // 7. Weighted routes for normalized ID testing
    addV4Route(updater, "10.6.0.0/24", {"1.1.1.10", "2.2.2.10"}, {1, 1});
    addV4Route(updater, "10.6.1.0/24", {"1.1.1.10", "2.2.2.10"}, {10, 20});
    addV4Route(updater, "10.6.2.0/24", {"1.1.1.10", "2.2.2.10"}, {100, 200});

    if (!includeV6) {
      return;
    }

    // V6 routes (same patterns)
    // 1. 2 routes with shared nexthops
    std::vector<std::string> sharedV6Nexthops = {"1::10", "2::10"};
    addV6Route(updater, "2001:db8:1::/64", sharedV6Nexthops);
    addV6Route(updater, "2001:db8:2::/64", sharedV6Nexthops);

    // 2. 1 route with single nexthop
    addV6Route(updater, "2001:db8:10::/64", {"2::20"});

    // 3. 1 route with 4 nexthops
    addV6Route(
        updater, "2001:db8:20::/64", {"1::10", "2::10", "3::10", "4::10"});

    // 4. 3 routes with partial overlap
    addV6Route(updater, "2001:db8:30::/64", {"1::10", "2::10", "3::10"});
    addV6Route(updater, "2001:db8:31::/64", {"2::10", "3::10", "4::10"});
    addV6Route(updater, "2001:db8:32::/64", {"3::10", "4::10", "1::10"});

    // 5. 1 route with DROP action
    addV6DropRoute(updater, "2001:db8:40::/64");

    // 6. 1 route with TO_CPU action
    addV6ToCpuRoute(updater, "2001:db8:50::/64");
  }

  SwSwitch* sw_ = nullptr;
  std::unique_ptr<HwTestHandle> handle_;
};

TEST_F(NextHopMapPopulationTest, RouteAdditionTestsThroughRIBUpdater) {
  auto updater = sw_->getRouteUpdater();
  addStandardTestRoutes(updater);
  updater.program();

  verifyIdMapsMatchIdManager();
  verifyIDMapsConsistency();
}

TEST_F(NextHopMapPopulationTest, RouteDeletionTestsThroughRIBUpdater) {
  auto updater = sw_->getRouteUpdater();
  addStandardTestRoutes(updater);
  updater.program();
  verifyIdMapsMatchIdManager();
  verifyIDMapsConsistency();

  // Delete one route with same nexthop (shared ECMP) and check consistency
  updater = sw_->getRouteUpdater();
  delV4Route(updater, "10.0.0.0/24");
  updater.program();
  verifyIdMapsMatchIdManager();
  verifyIDMapsConsistency();

  // Delete the other shared route
  updater = sw_->getRouteUpdater();
  delV4Route(updater, "10.0.1.0/24");
  updater.program();
  verifyIdMapsMatchIdManager();
  verifyIDMapsConsistency();

  updater = sw_->getRouteUpdater();
  delV4Route(updater, "10.6.2.0/24");
  updater.program();
  verifyIdMapsMatchIdManager();
  verifyIDMapsConsistency();

  // Delete multiple routes in a single update
  updater = sw_->getRouteUpdater();
  delV4Route(updater, "10.3.0.0/24");
  delV4Route(updater, "10.1.0.0/24");
  delV4Route(updater, "10.2.0.0/24");
  updater.program();
  verifyIdMapsMatchIdManager();
  verifyIDMapsConsistency();

  // Delete DROP and TO_CPU routes
  updater = sw_->getRouteUpdater();
  delV4Route(updater, "10.4.0.0/24");
  delV4Route(updater, "10.5.0.0/24");
  updater.program();
  verifyIdMapsMatchIdManager();
  verifyIDMapsConsistency();

  // Delete all remaining routes including weighted routes
  updater = sw_->getRouteUpdater();
  delV4Route(updater, "10.3.1.0/24");
  delV4Route(updater, "10.3.2.0/24");
  delV4Route(updater, "10.6.0.0/24");
  delV4Route(updater, "10.6.1.0/24");
  delV6Route(updater, "2001:db8:1::/64");
  delV6Route(updater, "2001:db8:2::/64");
  delV6Route(updater, "2001:db8:10::/64");
  delV6Route(updater, "2001:db8:20::/64");
  delV6Route(updater, "2001:db8:30::/64");
  delV6Route(updater, "2001:db8:31::/64");
  delV6Route(updater, "2001:db8:32::/64");
  delV6Route(updater, "2001:db8:40::/64");
  delV6Route(updater, "2001:db8:50::/64");
  updater.program();

  verifyIdMapsMatchIdManager();
}

TEST_F(NextHopMapPopulationTest, RouteUpdateTestsThroughRIBUpdater) {
  auto updater = sw_->getRouteUpdater();
  addStandardTestRoutes(updater);
  updater.program();
  verifyIdMapsMatchIdManager();
  verifyIDMapsConsistency();

  // Update route from having 2 nexthops to 4 (ECMP expand)
  updater = sw_->getRouteUpdater();
  addV4Route(
      updater, "10.0.0.0/24", {"1.1.1.10", "2.2.2.10", "3.3.3.10", "4.4.4.10"});
  updater.program();
  verifyIdMapsMatchIdManager();
  verifyIDMapsConsistency();

  // Update route with 4 nexthops (10.2.0.0/24) to have 2 nexthops (ECMP shrink)
  updater = sw_->getRouteUpdater();
  addV4Route(updater, "10.2.0.0/24", {"1.1.1.10", "2.2.2.10"});
  updater.program();
  verifyIdMapsMatchIdManager();
  verifyIDMapsConsistency();

  // Update route to a completely new set of nexthops
  updater = sw_->getRouteUpdater();
  addV4Route(updater, "10.0.0.0/24", {"3.3.3.10", "4.4.4.10"});
  updater.program();
  verifyIdMapsMatchIdManager();
  verifyIDMapsConsistency();

  // Update route from having nexthops to TO_CPU action
  updater = sw_->getRouteUpdater();
  addV4ToCpuRoute(updater, "10.0.0.0/24");
  updater.program();
  verifyIdMapsMatchIdManager();
  verifyIDMapsConsistency();

  // Update route from TO_CPU action to having nexthops
  updater = sw_->getRouteUpdater();
  addV4Route(updater, "10.0.0.0/24", {"1.1.1.10", "2.2.2.10"});
  updater.program();
  verifyIdMapsMatchIdManager();
  verifyIDMapsConsistency();
}

TEST_F(NextHopMapPopulationTest, SerializationTests) {
  auto updater = sw_->getRouteUpdater();
  addStandardTestRoutes(updater);
  updater.program();

  verifyIdMapsMatchIdManager();
  verifyIDMapsConsistency();

  // Get original maps for comparison
  auto originalIdToNextHopMap = getIdToNextHopMap();
  auto originalIdToNextHopIdSetMap = getIdToNextHopIdSetMap();

  // Serialize the FibInfo
  auto fibInfo = getFibInfo();
  ASSERT_NE(fibInfo, nullptr);
  auto serialized = fibInfo->toThrift();

  // Deserialize into a new FibInfo
  auto deserialized = std::make_shared<FibInfo>(serialized);

  // Verify IdToNextHopMap is preserved
  auto deserializedNextHopMap = deserialized->getIdToNextHopMap();
  ASSERT_NE(deserializedNextHopMap, nullptr);
  EXPECT_EQ(deserializedNextHopMap->size(), originalIdToNextHopMap->size())
      << "IdToNextHopMap size should be preserved after deserialization";

  // Verify IdToNextHopIdSetMap is preserved
  auto deserializedNextHopIdSetMap = deserialized->getIdToNextHopIdSetMap();
  ASSERT_NE(deserializedNextHopIdSetMap, nullptr);
  EXPECT_EQ(
      deserializedNextHopIdSetMap->size(), originalIdToNextHopIdSetMap->size())
      << "IdToNextHopIdSetMap size should be preserved after deserialization";
}

// Test general addition of POP_AND_LOOKUP MPLS routes alongside normal
// weighted routes. This verifies that both types of routes are handled
// correctly
TEST_F(NextHopMapPopulationTest, PopAndLookupWithNormalRouteAddition) {
  // First, add standard IP routes
  auto updater = sw_->getRouteUpdater();
  addStandardTestRoutes(updater, false /* includeV6 */);

  // Now add MPLS POP_AND_LOOKUP routes
  addMplsPopAndLookupRoute(updater, 1001);
  addMplsPopAndLookupRoute(updater, 1002);
  addMplsPopAndLookupRoute(updater, 1003);
  updater.program();

  // Verify MPLS POP_AND_LOOKUP routes have resolvedNextHopSetID but NOT
  // normalizedResolvedNextHopSetID
  auto labelFib = getLabelFib();
  ASSERT_NE(labelFib, nullptr);

  auto labelEntry1 = labelFib->getNodeIf(1001);
  ASSERT_NE(labelEntry1, nullptr);
  const auto& labelFwdInfo1 = labelEntry1->getForwardInfo();
  EXPECT_TRUE(labelFwdInfo1.getResolvedNextHopSetID().has_value())
      << "POP_AND_LOOKUP MPLS route should have resolvedNextHopSetID";
  EXPECT_FALSE(labelFwdInfo1.getNormalizedResolvedNextHopSetID().has_value())
      << "POP_AND_LOOKUP MPLS route should NOT have "
      << "normalizedResolvedNextHopSetID";

  auto labelEntry2 = labelFib->getNodeIf(1002);
  ASSERT_NE(labelEntry2, nullptr);
  const auto& labelFwdInfo2 = labelEntry2->getForwardInfo();
  EXPECT_TRUE(labelFwdInfo2.getResolvedNextHopSetID().has_value())
      << "POP_AND_LOOKUP MPLS route should have resolvedNextHopSetID";
  EXPECT_FALSE(labelFwdInfo2.getNormalizedResolvedNextHopSetID().has_value())
      << "POP_AND_LOOKUP MPLS route should NOT have "
      << "normalizedResolvedNextHopSetID";

  verifyIdMapsMatchIdManager();
  verifyIDMapsConsistency();
}

} // namespace facebook::fboss
