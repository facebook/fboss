// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/test/utils/NextHopIdTestUtils.h"

#include "fboss/agent/state/FibInfo.h"
#include "fboss/agent/state/FibInfoMap.h"
#include "fboss/agent/state/ForwardingInformationBase.h"
#include "fboss/agent/state/ForwardingInformationBaseContainer.h"
#include "fboss/agent/state/NextHopIdMaps.h"
#include "fboss/agent/state/Route.h"

namespace facebook::fboss {

void allocateRouteNextHopIds(
    NextHopIDManager* idManager,
    RouteNextHopEntry& entry) {
  if (!idManager || entry.getAction() != RouteForwardAction::NEXTHOPS) {
    return;
  }
  auto swNextHops = entry.getNextHopSet();
  auto allocResult = idManager->getOrAllocRouteNextHopSetID(swNextHops);
  std::optional<NextHopSetID> resolvedId =
      allocResult.nextHopIdSetIter->second.id;
  entry.setResolvedNextHopSetID(resolvedId);

  auto normalizedNhops = entry.nonOverrideNormalizedNextHops();
  auto normAllocResult =
      idManager->getOrAllocRouteNextHopSetID(normalizedNhops);
  std::optional<NextHopSetID> normalizedId =
      normAllocResult.nextHopIdSetIter->second.id;
  entry.setNormalizedResolvedNextHopSetID(normalizedId);
}

void populateFibInfoIdMaps(
    const NextHopIDManager* idManager,
    std::shared_ptr<SwitchState>& state,
    const HwSwitchMatcher& scope) {
  if (!idManager) {
    return;
  }
  auto fibInfoMap = state->getFibsInfoMap()->modify(&state);
  auto fibInfo = fibInfoMap->getFibInfo(scope);
  if (!fibInfo) {
    fibInfoMap->addNode(scope.matcherString(), std::make_shared<FibInfo>());
    fibInfo = fibInfoMap->getFibInfo(scope);
  }
  auto* fibInfoPtr = fibInfo->modify(&state);

  auto id2Nhop = std::make_shared<IdToNextHopMap>();
  for (const auto& [id, nhopEntry] : idManager->getIdToNextHop()) {
    id2Nhop->addNextHop(id, nhopEntry.nextHop.toThrift());
  }
  auto id2NhopSetIds = std::make_shared<IdToNextHopIdSetMap>();
  for (const auto& [id, nhopIdSet] : idManager->getIdToNextHopIdSet()) {
    std::set<int64_t> nhopIds;
    for (const auto& nhopId : nhopIdSet) {
      nhopIds.insert(static_cast<int64_t>(nhopId));
    }
    id2NhopSetIds->addNextHopIdSet(id, nhopIds);
  }
  fibInfoPtr->setIdToNextHopMap(id2Nhop);
  fibInfoPtr->setIdToNextHopIdSetMap(id2NhopSetIds);
}

void assignNextHopIdsToAllRoutes(
    NextHopIDManager* idManager,
    std::shared_ptr<SwitchState>& state,
    const HwSwitchMatcher& scope) {
  if (!idManager) {
    return;
  }
  auto fibContainer = state->getFibsInfoMap()->getFibContainerIf(RouterID(0));
  if (!fibContainer) {
    return;
  }

  // Capture const FIBs before modifying state
  auto constFibV6 = fibContainer->getFibV6();
  auto constFibV4 = fibContainer->getFibV4();

  auto assignIds = [&](auto* mutableFib, const auto& constFib) {
    for (const auto& [_, route] : std::as_const(*constFib)) {
      if (!route->isResolved() ||
          route->getForwardInfo().getAction() !=
              RouteNextHopEntry::Action::NEXTHOPS) {
        continue;
      }
      if (route->getForwardInfo().getResolvedNextHopSetID().has_value()) {
        continue; // Already has IDs
      }
      RouteNextHopEntry updatedFwd;
      updatedFwd.fromThrift(route->getForwardInfo().toThrift());
      allocateRouteNextHopIds(idManager, updatedFwd);
      auto clonedRoute = route->clone();
      clonedRoute->setResolved(updatedFwd);
      clonedRoute->publish();
      mutableFib->updateNode(clonedRoute);
    }
  };

  // Get mutable FIBs through full state traversal
  assignIds(
      state->getFibsInfoMap()
          ->getFibContainer(RouterID(0))
          ->getFib<folly::IPAddressV6>()
          ->modify(RouterID(0), &state),
      constFibV6);
  assignIds(
      state->getFibsInfoMap()
          ->getFibContainer(RouterID(0))
          ->getFib<folly::IPAddressV4>()
          ->modify(RouterID(0), &state),
      constFibV4);

  populateFibInfoIdMaps(idManager, state, scope);
}

} // namespace facebook::fboss
