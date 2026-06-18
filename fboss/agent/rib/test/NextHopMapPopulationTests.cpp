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
#include "fboss/agent/SwitchIdScopeResolver.h"
#include "fboss/agent/if/gen-cpp2/ctrl_types.h"
#include "fboss/agent/if/gen-cpp2/mpls_types.h"
#include "fboss/agent/rib/FibUpdateHelpers.h"
#include "fboss/agent/rib/NextHopIDManager.h"
#include "fboss/agent/rib/RoutingInformationBase.h"
#include "fboss/agent/rib/SwitchStateNextHopIdUpdater.h"
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

  // Add MPLS route with SWAP action (regular MPLS forwarding). Nexthop must
  // be reachable via one of the configured interface subnets so the route
  // resolves and gets a non-empty fwd nexthop set.
  void addMplsSwapRoute(
      SwSwitchRouteUpdateWrapper& updater,
      MplsLabel label,
      MplsLabel swapLabel,
      const folly::IPAddress& nhopAddr) {
    MplsRoute mplsRoute;
    mplsRoute.topLabel() = label;
    NextHopThrift nexthop;
    nexthop.mplsAction() = MplsAction();
    *nexthop.mplsAction()->action() = MplsActionCode::SWAP;
    nexthop.mplsAction()->swapLabel() = swapLabel;
    nexthop.address() = facebook::network::toBinaryAddress(nhopAddr);
    mplsRoute.nextHops()->emplace_back(nexthop);
    updater.addRoute(kClientA, mplsRoute);
  }

  // SWAP MPLS with multiple nexthops (for ECMP scenarios).
  void addMplsSwapRouteMulti(
      SwSwitchRouteUpdateWrapper& updater,
      MplsLabel label,
      const std::vector<std::pair<folly::IPAddress, MplsLabel>>& nhops) {
    MplsRoute mplsRoute;
    mplsRoute.topLabel() = label;
    for (const auto& [nhopAddr, swapLabel] : nhops) {
      NextHopThrift nexthop;
      nexthop.mplsAction() = MplsAction();
      *nexthop.mplsAction()->action() = MplsActionCode::SWAP;
      nexthop.mplsAction()->swapLabel() = swapLabel;
      nexthop.address() = facebook::network::toBinaryAddress(nhopAddr);
      mplsRoute.nextHops()->emplace_back(nexthop);
    }
    updater.addRoute(kClientA, mplsRoute);
  }

  void delMplsRoute(
      SwSwitchRouteUpdateWrapper& updater,
      MplsLabel label,
      ClientID clientId = kClientA) {
    updater.delRoute(MplsLabel(label), clientId);
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

  // Verifies that the ID maps in switch state match the ID maps in the
  // NextHopIDManager
  void verifyIdMapsMatchIdManager() {
    auto stateIdToNextHopMap = getIdToNextHopMap();
    auto stateIdToNextHopIdSetMap = getIdToNextHopIdSetMap();
    auto manager = sw_->getRib()->getNextHopIDManagerCopy();
    ASSERT_NE(manager, nullptr);
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
    for (const auto& [id, nextHopEntry] : managerIdToNextHop) {
      auto stateNextHop = stateIdToNextHopMap->getNextHopIf(id);
      ASSERT_NE(stateNextHop, nullptr)
          << "NextHop id " << id << " not in state";
      auto stateNextHopThrift = stateNextHop->toThrift();
      EXPECT_EQ(
          network::toIPAddress(*stateNextHopThrift.address()),
          nextHopEntry.nextHop.addr())
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

  // Walks every RIB route (resolved and unresolved). For each route checks:
  //   - per-client entries: clientNextHopSetID present iff nexthops non-empty
  //   - forwarding info: resolvedNextHopSetID present iff fwd is resolved
  //   - normalizedResolvedNextHopSetID: resolvability checked when present
  // Defaults to sw_->getRib(); pass a standalone RIB (e.g. one built via
  // RoutingInformationBase::fromThrift) to verify it directly.
  void verifyIDMapsConsistency(const RoutingInformationBase* rib = nullptr) {
    if (!rib) {
      rib = sw_->getRib();
    }
    auto manager = rib->getNextHopIDManagerCopy();
    ASSERT_NE(manager, nullptr);
    auto ribThrift = rib->toThrift();
    auto vrfIt = ribThrift.find(static_cast<int32_t>(kRid0));
    ASSERT_NE(vrfIt, ribThrift.end()) << "VRF " << kRid0 << " not found in RIB";

    auto checkRoute = [&](const auto& routeFields,
                          const std::string& prefixStr) {
      // Per-client check: walk every per-client entry on this route.
      const auto& nhMulti = *routeFields.nexthopsmulti();
      for (const auto& [clientId, perClientThrift] :
           *nhMulti.client2NextHopEntry()) {
        RouteNextHopEntry perClientEntry(perClientThrift);
        const auto& nhopSet = perClientEntry.getNextHopSet();
        auto clientSetId = perClientEntry.getClientNextHopSetID();
        if (nhopSet.empty()) {
          EXPECT_FALSE(clientSetId.has_value())
              << prefixStr << " client=" << static_cast<int>(clientId)
              << ": empty nexthop set must not have clientNextHopSetID";
          continue;
        }
        ASSERT_TRUE(clientSetId.has_value())
            << prefixStr << " client=" << static_cast<int>(clientId)
            << ": missing clientNextHopSetID";
        auto resolved = manager->getNextHopsIf(*clientSetId);
        ASSERT_TRUE(resolved.has_value())
            << prefixStr << " client=" << static_cast<int>(clientId)
            << ": clientNextHopSetID " << *clientSetId
            << " not found in manager";
        EXPECT_EQ(*resolved, nhopSet)
            << "Per-client nexthops mismatch for " << prefixStr
            << " client=" << static_cast<int>(clientId);
      }

      // Forwarding-side check: only meaningful when the route is resolved.
      RouteNextHopEntry fwdEntry(*routeFields.fwd());
      const auto& fwdNhopSet = fwdEntry.getNextHopSet();
      auto resolvedSetId = fwdEntry.getResolvedNextHopSetID();
      auto normalizedSetId = fwdEntry.getNormalizedResolvedNextHopSetID();
      if (fwdNhopSet.empty()) {
        EXPECT_FALSE(resolvedSetId.has_value())
            << prefixStr
            << ": unresolved/DROP route must not have resolvedNextHopSetID";
        EXPECT_FALSE(normalizedSetId.has_value())
            << prefixStr
            << ": unresolved/DROP route must not have normalizedResolvedNextHopSetID";
        return;
      }
      ASSERT_TRUE(resolvedSetId.has_value())
          << prefixStr << ": resolved route missing resolvedNextHopSetID";
      auto resolvedNextHops = manager->getNextHopsIf(*resolvedSetId);
      ASSERT_TRUE(resolvedNextHops.has_value())
          << prefixStr << ": resolvedNextHopSetID " << *resolvedSetId
          << " not found in manager";
      EXPECT_EQ(*resolvedNextHops, fwdNhopSet)
          << "Forwarding nexthops mismatch for " << prefixStr;

      // Normalized ID is only stamped after ECMP normalization; verify
      // resolvability if present, but don't require its presence.
      if (normalizedSetId.has_value()) {
        auto normalizedNextHops = manager->getNextHopsIf(*normalizedSetId);
        EXPECT_TRUE(normalizedNextHops.has_value())
            << prefixStr << ": normalizedResolvedNextHopSetID "
            << *normalizedSetId << " not found in manager";
      }
    };

    for (const auto& [prefixStr, routeFields] :
         *vrfIt->second.v4NetworkToRoute()) {
      checkRoute(routeFields, prefixStr);
    }
    for (const auto& [prefixStr, routeFields] :
         *vrfIt->second.v6NetworkToRoute()) {
      checkRoute(routeFields, prefixStr);
    }
    // MPLS routes share RouteNextHopsMulti/RouteNextHopEntry shape with IP,
    // so the same checks apply.
    for (const auto& [label, routeFields] : *vrfIt->second.labelToRoute()) {
      checkRoute(routeFields, folly::to<std::string>("label=", label));
    }
  }

  // Multi-client variants of addV4Route/delV4Route (the default helpers
  // hardcode kClientA).
  void addV4RouteForClient(
      SwSwitchRouteUpdateWrapper& updater,
      ClientID clientId,
      const std::string& prefixStr,
      const std::vector<std::string>& nexthopStrs) {
    auto prefix = makePrefixV4(prefixStr);
    auto nhops = makeNextHops(nexthopStrs);
    updater.addRoute(
        kRid0,
        prefix.network(),
        prefix.mask(),
        clientId,
        RouteNextHopEntry(nhops, DISTANCE));
  }

  void addV4DropRouteForClient(
      SwSwitchRouteUpdateWrapper& updater,
      ClientID clientId,
      const std::string& prefixStr) {
    auto prefix = makePrefixV4(prefixStr);
    updater.addRoute(
        kRid0,
        prefix.network(),
        prefix.mask(),
        clientId,
        RouteNextHopEntry(RouteForwardAction::DROP, DISTANCE));
  }

  void addV6RouteForClient(
      SwSwitchRouteUpdateWrapper& updater,
      ClientID clientId,
      const std::string& prefixStr,
      const std::vector<std::string>& nexthopStrs) {
    auto prefix = makePrefixV6(prefixStr);
    auto nhops = makeNextHops(nexthopStrs);
    updater.addRoute(
        kRid0,
        prefix.network(),
        prefix.mask(),
        clientId,
        RouteNextHopEntry(nhops, DISTANCE));
  }

  void delV4RouteForClient(
      SwSwitchRouteUpdateWrapper& updater,
      ClientID clientId,
      const std::string& prefixStr) {
    auto prefix = makePrefixV4(prefixStr);
    updater.delRoute(kRid0, prefix.network(), prefix.mask(), clientId);
  }

  // Returns the clientNextHopSetID stored on (prefix, clientId), or nullopt.
  std::optional<NextHopSetID> getStoredClientId(
      const std::string& prefixStr,
      ClientID clientId) {
    auto state = sw_->getState();
    auto fibInfo = getFibInfo();
    if (!fibInfo) {
      return std::nullopt;
    }
    auto fibContainer = fibInfo->getFibContainerIf(kRid0);
    if (!fibContainer) {
      return std::nullopt;
    }
    auto prefix = makePrefixV4(prefixStr);
    auto fibV4 = fibContainer->getFibV4();
    for (const auto& [_, route] : std::as_const(*fibV4)) {
      if (route->prefix().network() == prefix.network() &&
          route->prefix().mask() == prefix.mask()) {
        auto entry = route->getEntryForClient(clientId);
        return entry ? entry->getClientNextHopSetID() : std::nullopt;
      }
    }
    return std::nullopt;
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

// Verifies per-client clientNextHopSetID allocation in
// RibRouteUpdater::addOrReplaceRoute(LabelID, ...). Both SWAP and
// POP_AND_LOOKUP MPLS routes must get clientNextHopSetID stamped on
// their per-client entry, mirroring the v4/v6 allocation path.
TEST_F(NextHopMapPopulationTest, AllocatesClientIdsForMplsRoutes) {
  FLAGS_mpls_rib = true;
  auto updater = sw_->getRouteUpdater();
  // SWAP MPLS route via interface subnet 1::/48.
  addMplsSwapRoute(updater, 100, 101, folly::IPAddress("1::10"));
  // POP_AND_LOOKUP MPLS route.
  addMplsPopAndLookupRoute(updater, 200);
  updater.program();

  auto ribThrift = sw_->getRib()->toThrift();
  ASSERT_FALSE(ribThrift.empty());
  const auto& labelToRoute = *ribThrift.begin()->second.labelToRoute();

  auto verifyClientIdsPresent = [](const auto& routeFields) {
    for (const auto& [_clientId, entry] :
         *routeFields.nexthopsmulti()->client2NextHopEntry()) {
      if (!entry.nexthops()->empty()) {
        EXPECT_TRUE(entry.clientNextHopSetID().has_value())
            << "MPLS per-client entry missing clientNextHopSetID";
      }
    }
  };

  auto swapIt = labelToRoute.find(100);
  ASSERT_NE(swapIt, labelToRoute.end());
  verifyClientIdsPresent(swapIt->second);

  auto popIt = labelToRoute.find(200);
  ASSERT_NE(popIt, labelToRoute.end());
  verifyClientIdsPresent(popIt->second);
}

// --- SwitchStateNextHopIdUpdater incremental diff tests ---
// These verify the updater correctly diffs RIB vs FIB maps and only
// applies the delta (adds missing entries, removes stale entries).

TEST_F(NextHopMapPopulationTest, UpdaterNoChangesReturnsSameState) {
  auto updater = sw_->getRouteUpdater();
  addStandardTestRoutes(updater);
  updater.program();
  verifyIdMapsMatchIdManager();
  verifyIDMapsConsistency();

  // Run the updater again with no RIB changes — should return same state
  auto stateBefore = sw_->getState();
  auto managerCopy = sw_->getRib()->getNextHopIDManagerCopy();
  SwitchStateNextHopIdUpdater ribUpdater(managerCopy.get());
  auto stateAfter = ribUpdater(stateBefore);
  EXPECT_EQ(stateAfter.get(), stateBefore.get());
}

TEST_F(NextHopMapPopulationTest, UpdaterIncrementalAddViaRouteUpdater) {
  // Start with standard routes (shared ECMP, single nhop, overlapping, etc.)
  auto updater = sw_->getRouteUpdater();
  addStandardTestRoutes(updater);
  updater.program();
  verifyIdMapsMatchIdManager();
  verifyIDMapsConsistency();

  // Incrementally add routes with both new and existing nexthops
  updater = sw_->getRouteUpdater();
  // These reuse existing nexthops — only new set IDs
  addV4Route(updater, "10.10.0.0/24", {"3.3.3.10"});
  addV4Route(updater, "10.10.1.0/24", {"1.1.1.10", "4.4.4.10"});
  addV6Route(updater, "2001:db8:100::/64", {"3::10", "4::10"});
  // These use brand new nexthops — new nhop IDs + set IDs
  addV4Route(updater, "10.10.4.0/24", {"1.1.1.30", "2.2.2.30"});
  addV6Route(updater, "2001:db8:101::/64", {"1::30"});
  // DROP/TO_CPU don't allocate IDs
  addV4DropRoute(updater, "10.10.2.0/24");
  addV4ToCpuRoute(updater, "10.10.3.0/24");
  updater.program();
  verifyIdMapsMatchIdManager();
  verifyIDMapsConsistency();
}

TEST_F(NextHopMapPopulationTest, UpdaterIncrementalRemoveViaRouteUpdater) {
  // Start with standard routes
  auto updater = sw_->getRouteUpdater();
  addStandardTestRoutes(updater);
  updater.program();
  verifyIdMapsMatchIdManager();
  verifyIDMapsConsistency();

  // Delete several routes: one from shared ECMP pair, overlapping groups, etc.
  updater = sw_->getRouteUpdater();
  delV4Route(updater, "10.0.0.0/24");
  delV4Route(updater, "10.3.0.0/24");
  delV4Route(updater, "10.3.1.0/24");
  delV6Route(updater, "2001:db8:1::/64");
  delV6Route(updater, "2001:db8:30::/64");
  updater.program();
  verifyIdMapsMatchIdManager();
  verifyIDMapsConsistency();

  // Delete remaining routes that used unique nexthops
  updater = sw_->getRouteUpdater();
  delV4Route(updater, "10.1.0.0/24");
  delV6Route(updater, "2001:db8:10::/64");
  updater.program();
  verifyIdMapsMatchIdManager();
  verifyIDMapsConsistency();
}

TEST_F(NextHopMapPopulationTest, UpdaterAddAndRemoveInSingleUpdate) {
  // Start with standard routes
  auto updater = sw_->getRouteUpdater();
  addStandardTestRoutes(updater);
  updater.program();
  verifyIdMapsMatchIdManager();
  verifyIDMapsConsistency();

  // Simultaneously delete some routes and add new ones
  updater = sw_->getRouteUpdater();
  // Delete shared ECMP pair and overlapping group
  delV4Route(updater, "10.0.0.0/24");
  delV4Route(updater, "10.0.1.0/24");
  delV4Route(updater, "10.3.0.0/24");
  delV6Route(updater, "2001:db8:1::/64");
  delV6Route(updater, "2001:db8:2::/64");
  // Add new routes with mix of existing and new nexthops
  addV4Route(updater, "10.20.0.0/24", {"1.1.1.10", "3.3.3.10"});
  addV4Route(updater, "10.20.1.0/24", {"4.4.4.10"});
  addV6Route(updater, "2001:db8:200::/64", {"1::10", "2::10", "3::10"});
  addV4DropRoute(updater, "10.20.2.0/24");
  updater.program();
  verifyIdMapsMatchIdManager();
  verifyIDMapsConsistency();
}

// ----- Per-client clientNextHopSetID lifecycle tests -----

// Covers: (1) initial alloc stamps an ID, (2) two clients with the same
// nexthop set share the ID via refcount, (3) re-programming the same set
// keeps the ID stable.
TEST_F(NextHopMapPopulationTest, ClientIdAllocationAndSharing) {
  const ClientID kClientB = ClientID(1002);

  // Step 1: kClientA programs a route -> ID gets stamped.
  auto updater = sw_->getRouteUpdater();
  addV4RouteForClient(
      updater, kClientA, "10.0.0.0/24", {"1.1.1.10", "2.2.2.10"});
  updater.program();
  auto idA = getStoredClientId("10.0.0.0/24", kClientA);
  ASSERT_TRUE(idA.has_value());

  // Step 2: kClientB programs the same nexthops on a different prefix ->
  // refcount-sharing produces the same ID.
  updater = sw_->getRouteUpdater();
  addV4RouteForClient(
      updater, kClientB, "10.1.0.0/24", {"1.1.1.10", "2.2.2.10"});
  updater.program();
  auto idB = getStoredClientId("10.1.0.0/24", kClientB);
  ASSERT_TRUE(idB.has_value());
  EXPECT_EQ(*idA, *idB);

  // Step 3: kClientA re-programs the same nexthops -> same-set short-circuit
  // keeps the original ID stable.
  updater = sw_->getRouteUpdater();
  addV4RouteForClient(
      updater, kClientA, "10.0.0.0/24", {"1.1.1.10", "2.2.2.10"});
  updater.program();
  auto idAReprogram = getStoredClientId("10.0.0.0/24", kClientA);
  ASSERT_TRUE(idAReprogram.has_value());
  EXPECT_EQ(*idA, *idAReprogram);

  verifyIDMapsConsistency();
}

// Covers: (1) re-programming with a different set switches to a new ID,
// (2) DROP / TO_CPU entries never get an ID.
TEST_F(NextHopMapPopulationTest, ClientIdSwitchOnDifferentSetAndNoneOnDrop) {
  // Step 1: program a route, then re-program with a different nexthop set.
  auto updater = sw_->getRouteUpdater();
  addV4RouteForClient(
      updater, kClientA, "10.0.0.0/24", {"1.1.1.10", "2.2.2.10"});
  updater.program();
  auto idBefore = getStoredClientId("10.0.0.0/24", kClientA);
  ASSERT_TRUE(idBefore.has_value());

  updater = sw_->getRouteUpdater();
  addV4RouteForClient(
      updater, kClientA, "10.0.0.0/24", {"3.3.3.10", "4.4.4.10"});
  updater.program();
  auto idAfter = getStoredClientId("10.0.0.0/24", kClientA);
  ASSERT_TRUE(idAfter.has_value());
  EXPECT_NE(*idBefore, *idAfter);

  // Step 2: a DROP route gets no clientNextHopSetID.
  updater = sw_->getRouteUpdater();
  addV4DropRouteForClient(updater, kClientA, "10.4.0.0/24");
  updater.program();
  EXPECT_FALSE(getStoredClientId("10.4.0.0/24", kClientA).has_value());

  verifyIDMapsConsistency();
}

// Two clients share a nexthop set; deleting one's route drops its refcount
// but the set survives because the other client still references it.
TEST_F(NextHopMapPopulationTest, DeleteOneClientReleasesItsIdOnly) {
  const ClientID kClientB = ClientID(1002);
  auto updater = sw_->getRouteUpdater();
  addV4RouteForClient(
      updater, kClientA, "10.0.0.0/24", {"1.1.1.10", "2.2.2.10"});
  addV4RouteForClient(
      updater, kClientB, "10.1.0.0/24", {"1.1.1.10", "2.2.2.10"});
  updater.program();

  auto sharedId = getStoredClientId("10.0.0.0/24", kClientA);
  ASSERT_TRUE(sharedId.has_value());
  ASSERT_EQ(*sharedId, *getStoredClientId("10.1.0.0/24", kClientB));

  updater = sw_->getRouteUpdater();
  delV4RouteForClient(updater, kClientA, "10.0.0.0/24");
  updater.program();

  auto idStillThere = getStoredClientId("10.1.0.0/24", kClientB);
  ASSERT_TRUE(idStillThere.has_value());
  EXPECT_EQ(*idStillThere, *sharedId);
  verifyIDMapsConsistency();
}

// Full mix: resolved + DROP + TO_CPU + unresolved + multi-client routes,
// all in a single state. Exercises the unified verifier end-to-end.
TEST_F(NextHopMapPopulationTest, MixedResolvedAndUnresolvedRoutes) {
  const ClientID kClientB = ClientID(1002);
  auto updater = sw_->getRouteUpdater();

  // Resolved + DROP + TO_CPU. Interfaces are on 1.1.1.0/24, 2.2.2.0/24,
  // 3.3.3.0/24, 4.4.4.0/24 (V4) and corresponding V6 subnets.
  addStandardTestRoutes(updater);

  // Unresolved V4 routes: nexthops not on any interface subnet.
  addV4Route(updater, "20.0.0.0/24", {"99.99.99.99"});
  addV4Route(updater, "20.1.0.0/24", {"99.99.99.99", "88.88.88.88"});

  // Unresolved V6 routes.
  addV6Route(updater, "2002:db8::/64", {"99::1"});
  addV6Route(updater, "2002:db8:1::/64", {"99::1", "88::1"});

  // Multi-client routes from kClientB: one resolved (shared nexthop set),
  // one unresolved.
  addV4RouteForClient(
      updater, kClientB, "30.0.0.0/24", {"1.1.1.10", "2.2.2.10"});
  addV4RouteForClient(updater, kClientB, "30.1.0.0/24", {"77.77.77.77"});

  updater.program();

  verifyIDMapsConsistency();
  verifyIdMapsMatchIdManager();
}

// fromThrift backfill stamps all three IDs on snapshot routes that lack them.
// Mirrors the production flag-OFF -> flag-ON warmboot scenario by programming
// routes with the manager off (so no IDs are stamped at program time), then
// reconstructing with the manager on (backfill should fill in all IDs).
TEST_F(NextHopMapPopulationTest, BackfillsMissingIdsFromWarmBoot) {
  // Tear down the fixture's flag-on handle and rebuild with the flag off so
  // routes are programmed without any IDs.
  handle_.reset();
  FLAGS_enable_nexthop_id_manager = false;
  auto config = initialConfig();
  handle_ = createTestHandle(&config);
  sw_ = handle_->getSw();

  auto updater = sw_->getRouteUpdater();
  addStandardTestRoutes(updater);
  updater.program();

  auto ribThrift = sw_->getRib()->toThrift();

  // Reconstruct under flag-on -- backfill runs inside fromThrift.
  FLAGS_enable_nexthop_id_manager = true;
  auto reconstructedRib = RoutingInformationBase::fromThrift(
      ribThrift,
      nullptr,
      std::make_shared<MultiLabelForwardingInformationBase>(),
      std::make_shared<MultiSwitchMySidMap>());

  // Every NEXTHOPS route must have all three IDs populated.
  auto reThrift = reconstructedRib->toThrift();
  size_t verifiedClientEntries = 0;
  size_t verifiedFwd = 0;
  auto verifyRoute = [&](const auto& routeFields) {
    if (!routeFields.fwd()->nexthops()->empty()) {
      EXPECT_TRUE(routeFields.fwd()->resolvedNextHopSetID().has_value());
      EXPECT_TRUE(
          routeFields.fwd()->normalizedResolvedNextHopSetID().has_value());
      ++verifiedFwd;
    }
    for (const auto& [_clientId, entry] :
         *routeFields.nexthopsmulti()->client2NextHopEntry()) {
      if (!entry.nexthops()->empty()) {
        EXPECT_TRUE(entry.clientNextHopSetID().has_value());
        ++verifiedClientEntries;
      }
    }
  };
  for (const auto& [_, routeTable] : reThrift) {
    for (const auto& [__, v4Route] : *routeTable.v4NetworkToRoute()) {
      verifyRoute(v4Route);
    }
    for (const auto& [__, v6Route] : *routeTable.v6NetworkToRoute()) {
      verifyRoute(v6Route);
    }
  }
  EXPECT_GT(verifiedClientEntries, 0u);
  EXPECT_GT(verifiedFwd, 0u);

  auto postManager = reconstructedRib->getNextHopIDManagerCopy();
  ASSERT_NE(postManager, nullptr);
  EXPECT_FALSE(postManager->getIdToNextHop().empty());
  EXPECT_FALSE(postManager->getIdToNextHopIdSet().empty());
}

// MPLS variant of the warmboot backfill test. Verifies:
//   - SWAP MPLS routes get all three IDs stamped on backfill
//     (resolvedNextHopSetID + normalizedResolvedNextHopSetID + per-client).
//   - POP_AND_LOOKUP MPLS routes get resolvedNextHopSetID + per-client only,
//     NOT normalizedResolvedNextHopSetID (POP_AND_LOOKUP forwards via inner-
//     header lookup, so there are no nexthops to normalize). Matches the
//     RouteUpdater allocation-time asymmetry at RouteUpdater.cpp's
//     `if (!labelPopandLookup)` guard.
TEST_F(NextHopMapPopulationTest, BackfillsMissingMplsIdsFromWarmBoot) {
  // Rebuild with manager-off so MPLS routes are programmed without IDs.
  // FLAGS_mpls_rib needed so RIB carries labelToRoute the backfill can walk.
  handle_.reset();
  FLAGS_enable_nexthop_id_manager = false;
  FLAGS_mpls_rib = true;
  auto config = initialConfig();
  handle_ = createTestHandle(&config);
  sw_ = handle_->getSw();

  auto updater = sw_->getRouteUpdater();
  // SWAP MPLS route resolving via interface subnet 1::/48.
  addMplsSwapRoute(updater, 100, 101, folly::IPAddress("1::10"));
  // POP_AND_LOOKUP MPLS route.
  addMplsPopAndLookupRoute(updater, 200);
  updater.program();

  auto ribThrift = sw_->getRib()->toThrift();

  // Reconstruct under flag-on — backfill walks labelToRoute too.
  FLAGS_enable_nexthop_id_manager = true;
  auto reconstructedRib = RoutingInformationBase::fromThrift(
      ribThrift,
      nullptr,
      std::make_shared<MultiLabelForwardingInformationBase>(),
      std::make_shared<MultiSwitchMySidMap>());
  auto reThrift = reconstructedRib->toThrift();
  ASSERT_FALSE(reThrift.empty());
  const auto& labelToRoute = *reThrift.begin()->second.labelToRoute();

  auto verifyClientIdsPresent = [](const auto& routeFields) {
    for (const auto& [_clientId, entry] :
         *routeFields.nexthopsmulti()->client2NextHopEntry()) {
      if (!entry.nexthops()->empty()) {
        EXPECT_TRUE(entry.clientNextHopSetID().has_value())
            << "MPLS per-client entry missing clientNextHopSetID";
      }
    }
  };

  // SWAP route at label 100: resolved + normalized + per-client all set.
  auto swapIt = labelToRoute.find(100);
  ASSERT_NE(swapIt, labelToRoute.end());
  EXPECT_TRUE(swapIt->second.fwd()->resolvedNextHopSetID().has_value())
      << "SWAP MPLS route missing resolvedNextHopSetID";
  EXPECT_TRUE(
      swapIt->second.fwd()->normalizedResolvedNextHopSetID().has_value())
      << "SWAP MPLS route missing normalizedResolvedNextHopSetID";
  verifyClientIdsPresent(swapIt->second);

  // POP_AND_LOOKUP at label 200: resolved + per-client only, NO normalized.
  auto popIt = labelToRoute.find(200);
  ASSERT_NE(popIt, labelToRoute.end());
  EXPECT_TRUE(popIt->second.fwd()->resolvedNextHopSetID().has_value())
      << "POP_AND_LOOKUP MPLS route missing resolvedNextHopSetID";
  EXPECT_FALSE(
      popIt->second.fwd()->normalizedResolvedNextHopSetID().has_value())
      << "POP_AND_LOOKUP MPLS route should NOT have "
      << "normalizedResolvedNextHopSetID";
  verifyClientIdsPresent(popIt->second);
}

// syncFib's bulk-delete path (removeAllUnclaimedRoutesFromClientImpl) must
// release the resolved + normalized nexthop set IDs when a reclaimed route
// is erased entirely. Covers both v4 and v6 template instantiations in one
// syncFib. Without the release the manager refcounts leak on every BGP
// reconvergence.
TEST_F(NextHopMapPopulationTest, SyncFibReleasesFwdSideIdsForReclaimedRoutes) {
  // Two v4 + two v6 routes with distinct nexthop sets so each owns a unique
  // resolved/normalized setID — no refcount sharing to mask a leak.
  auto updater = sw_->getRouteUpdater();
  addV4Route(updater, "10.0.0.0/24", {"1.1.1.10"});
  addV4Route(updater, "10.1.0.0/24", {"2.2.2.10"});
  addV6Route(updater, "2001:db8::/64", {"1::10"});
  addV6Route(updater, "2001:db8:1::/64", {"2::10"});
  updater.program();

  // Capture the setIDs of the routes that will be reclaimed
  // (10.0.0.0/24 and 2001:db8::/64).
  NextHopSetID v4ResolvedId, v4NormalizedId, v6ResolvedId, v6NormalizedId;
  {
    auto ribThrift = sw_->getRib()->toThrift();
    const auto& v4Route =
        ribThrift.begin()->second.v4NetworkToRoute()->at("10.0.0.0/24");
    auto v4ResolvedOpt = v4Route.fwd()->resolvedNextHopSetID();
    auto v4NormalizedOpt = v4Route.fwd()->normalizedResolvedNextHopSetID();
    ASSERT_TRUE(v4ResolvedOpt.has_value());
    ASSERT_TRUE(v4NormalizedOpt.has_value());
    v4ResolvedId = NextHopSetID(*v4ResolvedOpt);
    v4NormalizedId = NextHopSetID(*v4NormalizedOpt);

    const auto& v6Route =
        ribThrift.begin()->second.v6NetworkToRoute()->at("2001:db8::/64");
    auto v6ResolvedOpt = v6Route.fwd()->resolvedNextHopSetID();
    auto v6NormalizedOpt = v6Route.fwd()->normalizedResolvedNextHopSetID();
    ASSERT_TRUE(v6ResolvedOpt.has_value());
    ASSERT_TRUE(v6NormalizedOpt.has_value());
    v6ResolvedId = NextHopSetID(*v6ResolvedOpt);
    v6NormalizedId = NextHopSetID(*v6NormalizedOpt);
  }

  auto preManager = sw_->getRib()->getNextHopIDManagerCopy();
  ASSERT_NE(preManager, nullptr);
  ASSERT_TRUE(preManager->getIdToNextHopIdSet().contains(v4ResolvedId));
  ASSERT_TRUE(preManager->getIdToNextHopIdSet().contains(v4NormalizedId));
  ASSERT_TRUE(preManager->getIdToNextHopIdSet().contains(v6ResolvedId));
  ASSERT_TRUE(preManager->getIdToNextHopIdSet().contains(v6NormalizedId));

  // syncFib with only the surviving prefix in each family. The reclaimed
  // routes (10.0.0.0/24, 2001:db8::/64) go through
  // removeAllUnclaimedRoutesFromClientImpl<v4> and <v6> respectively. Each
  // was the only client on its prefix, so the route is erased entirely —
  // its resolved + normalized IDs must be released.
  updater = sw_->getRouteUpdater();
  addV4Route(updater, "10.1.0.0/24", {"2.2.2.10"});
  addV6Route(updater, "2001:db8:1::/64", {"2::10"});
  RouteUpdateWrapper::SyncFibInfo syncInfo;
  syncInfo.ridAndClients.insert({kRid0, kClientA});
  syncInfo.type = RouteUpdateWrapper::SyncFibInfo::SyncFibType::IP_ONLY;
  updater.program(syncInfo);

  auto postManager = sw_->getRib()->getNextHopIDManagerCopy();
  ASSERT_NE(postManager, nullptr);
  EXPECT_FALSE(postManager->getIdToNextHopIdSet().contains(v4ResolvedId))
      << "v4 resolvedNextHopSetID leaked in syncFib reclaim path";
  EXPECT_FALSE(postManager->getIdToNextHopIdSet().contains(v4NormalizedId))
      << "v4 normalizedResolvedNextHopSetID leaked in syncFib reclaim path";
  EXPECT_FALSE(postManager->getIdToNextHopIdSet().contains(v6ResolvedId))
      << "v6 resolvedNextHopSetID leaked in syncFib reclaim path";
  EXPECT_FALSE(postManager->getIdToNextHopIdSet().contains(v6NormalizedId))
      << "v6 normalizedResolvedNextHopSetID leaked in syncFib reclaim path";
}

// Regression: a route going resolved -> truly unresolvable must clear its fwd
// info, not just flip the flag, else the freed resolvedNextHopSetID stays
// stamped and double-frees on the next re-resolve/remove. An A->B->C->A nexthop
// cycle forces genuine unresolvability (no nexthops/drop/to-cpu) despite the
// fixture's default route.
TEST_F(NextHopMapPopulationTest, ResolvedToUnresolvableClearsFwdSideIds) {
  auto fwdResolvedId =
      [&](const std::string& prefixStr) -> std::optional<int64_t> {
    auto ribThrift = sw_->getRib()->toThrift();
    const auto& v4 = *ribThrift.begin()->second.v4NetworkToRoute();
    auto it = v4.find(prefixStr);
    if (it == v4.end()) {
      return std::nullopt;
    }
    return it->second.fwd()->resolvedNextHopSetID().to_optional();
  };
  auto fwdNormalizedId =
      [&](const std::string& prefixStr) -> std::optional<int64_t> {
    auto ribThrift = sw_->getRib()->toThrift();
    const auto& v4 = *ribThrift.begin()->second.v4NetworkToRoute();
    auto it = v4.find(prefixStr);
    if (it == v4.end()) {
      return std::nullopt;
    }
    return it->second.fwd()->normalizedResolvedNextHopSetID().to_optional();
  };

  // Step 1 (unresolved -> resolved): three routes resolve via connected
  // nexthops; each gets a distinct fwd-side resolved + normalized setID the
  // manager holds.
  auto updater = sw_->getRouteUpdater();
  addV4Route(updater, "10.0.0.0/8", {"1.1.1.10"});
  addV4Route(updater, "20.0.0.0/8", {"2.2.2.10"});
  addV4Route(updater, "30.0.0.0/8", {"3.3.3.10"});
  updater.program();
  auto idA = fwdResolvedId("10.0.0.0/8");
  auto idB = fwdResolvedId("20.0.0.0/8");
  auto idC = fwdResolvedId("30.0.0.0/8");
  ASSERT_TRUE(idA.has_value());
  ASSERT_TRUE(idB.has_value());
  ASSERT_TRUE(idC.has_value());
  ASSERT_TRUE(fwdNormalizedId("10.0.0.0/8").has_value());
  ASSERT_TRUE(fwdNormalizedId("20.0.0.0/8").has_value());
  ASSERT_TRUE(fwdNormalizedId("30.0.0.0/8").has_value());
  verifyIDMapsConsistency();
  verifyIdMapsMatchIdManager();

  // Step 2 (resolved -> truly unresolvable): rewire into an A->B->C->A cycle.
  // No route resolves, so each hits the unresolvable path. The fwd-side IDs
  // must be cleared off the route and freed from the manager. Without the fix
  // the stale freed IDs stay stamped and these EXPECT_FALSEs fail.
  updater = sw_->getRouteUpdater();
  addV4Route(updater, "10.0.0.0/8", {"20.1.1.10"});
  addV4Route(updater, "20.0.0.0/8", {"30.1.1.10"});
  addV4Route(updater, "30.0.0.0/8", {"10.1.1.10"});
  updater.program();
  EXPECT_FALSE(fwdResolvedId("10.0.0.0/8").has_value());
  EXPECT_FALSE(fwdResolvedId("20.0.0.0/8").has_value());
  EXPECT_FALSE(fwdResolvedId("30.0.0.0/8").has_value());
  EXPECT_FALSE(fwdNormalizedId("10.0.0.0/8").has_value());
  EXPECT_FALSE(fwdNormalizedId("20.0.0.0/8").has_value());
  EXPECT_FALSE(fwdNormalizedId("30.0.0.0/8").has_value());
  verifyIDMapsConsistency();
  verifyIdMapsMatchIdManager();

  // Step 3 (unresolvable -> resolved again): rewire back to connected
  // nexthops. The routes re-resolve with fresh fwd-side IDs; without the fix
  // updating from the dangling old ID double-frees here.
  updater = sw_->getRouteUpdater();
  addV4Route(updater, "10.0.0.0/8", {"1.1.1.10"});
  addV4Route(updater, "20.0.0.0/8", {"2.2.2.10"});
  addV4Route(updater, "30.0.0.0/8", {"3.3.3.10"});
  updater.program();
  EXPECT_TRUE(fwdResolvedId("10.0.0.0/8").has_value());
  EXPECT_TRUE(fwdResolvedId("20.0.0.0/8").has_value());
  EXPECT_TRUE(fwdResolvedId("30.0.0.0/8").has_value());
  EXPECT_TRUE(fwdNormalizedId("10.0.0.0/8").has_value());
  EXPECT_TRUE(fwdNormalizedId("20.0.0.0/8").has_value());
  EXPECT_TRUE(fwdNormalizedId("30.0.0.0/8").has_value());
  verifyIDMapsConsistency();
  verifyIdMapsMatchIdManager();

  // Step 4 (remove): the routes remove cleanly with no double-free.
  updater = sw_->getRouteUpdater();
  delV4Route(updater, "10.0.0.0/8");
  delV4Route(updater, "20.0.0.0/8");
  delV4Route(updater, "30.0.0.0/8");
  updater.program();
  verifyIDMapsConsistency();
  verifyIdMapsMatchIdManager();
}

} // namespace facebook::fboss
