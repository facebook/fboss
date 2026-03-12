/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/rib/ForwardingInformationBaseUpdater.h"

#include "fboss/agent/AgentFeatures.h"
#include "fboss/agent/FibHelpers.h"
#include "fboss/agent/SwitchIdScopeResolver.h"
#include "fboss/agent/rib/NetworkToRouteMap.h"
#include "fboss/agent/rib/NextHopIDManager.h"
#include "fboss/agent/rib/RoutingInformationBase.h"
#include "fboss/agent/state/FibInfo.h"
#include "fboss/agent/state/FibInfoMap.h"
#include "fboss/agent/state/ForwardingInformationBaseContainer.h"
#include "fboss/agent/state/ForwardingInformationBaseMap.h"
#include "fboss/agent/state/NodeBase-defs.h"
#include "fboss/agent/state/Route.h"
#include "fboss/agent/state/SwitchState.h"

#include <algorithm>

#include <folly/logging/xlog.h>

namespace facebook::fboss {

ForwardingInformationBaseUpdater::ForwardingInformationBaseUpdater(
    const SwitchIdScopeResolver* resolver,
    RouterID vrf,
    const IPv4NetworkToRouteMap& v4NetworkToRoute,
    const IPv6NetworkToRouteMap& v6NetworkToRoute,
    const LabelToRouteMap& labelToRoute,
    const NextHopIDManager* nextHopIDManager)
    : resolver_(resolver),
      vrf_(vrf),
      v4NetworkToRoute_(v4NetworkToRoute),
      v6NetworkToRoute_(v6NetworkToRoute),
      labelToRoute_(labelToRoute),
      nextHopIDManager_(nextHopIDManager) {}

std::shared_ptr<SwitchState> ForwardingInformationBaseUpdater::operator()(
    const std::shared_ptr<SwitchState>& state) {
  // A ForwardingInformationBaseContainer holds a
  // ForwardingInformationBaseV4 and a ForwardingInformationBaseV6 for a
  // particular VRF. Since FIBs for both address families will be updated,
  // we invoke modify() on the ForwardingInformationBaseContainer rather
  // than on its two children (namely ForwardingInformationBaseV4 and
  // ForwardingInformationBaseV6) in succession.
  // Unlike the coupled RIB implementation, we need only update the
  // SwitchState for a single VRF.
  std::shared_ptr<SwitchState> nextState(state);
  SCOPE_SUCCESS {
    std::optional<StateDelta> next(StateDelta(state, nextState));
    lastDelta_.swap(next);
  };

  auto previousFibContainer =
      nextState->getFibsInfoMap()->getFibContainerIf(vrf_);
  if (!previousFibContainer) {
    auto fibInfoMap = nextState->getFibsInfoMap()->modify(&nextState);

    previousFibContainer =
        std::make_shared<ForwardingInformationBaseContainer>(vrf_);

    auto scope = resolver_->scope(previousFibContainer);
    auto fibInfo = fibInfoMap->getFibInfo(scope);
    if (!fibInfo) {
      fibInfo = std::make_shared<FibInfo>();
      fibInfoMap->addNode(scope.matcherString(), fibInfo);
    }

    // Update the container through FibInfo
    fibInfo->updateFibContainer(previousFibContainer, &nextState);
  }
  CHECK(previousFibContainer);

  auto newFibV4 = createUpdatedFib(
      v4NetworkToRoute_, previousFibContainer->getFibV4(), nextState);

  auto newFibV6 = createUpdatedFib(
      v6NetworkToRoute_, previousFibContainer->getFibV6(), nextState);

  auto newLabelFib = createUpdatedLabelFib(
      labelToRoute_, state->getLabelForwardingInformationBase());

  if (!newFibV4 && !newFibV6 && !newLabelFib) {
    // return nextState in case we modified state above to insert new VRF
    return nextState;
  }
  auto nextFibContainer = previousFibContainer->modify(&nextState);

  if (nextHopIDManager_) {
    auto scope = resolver_->scope(previousFibContainer);
    auto fibInfo = nextState->getFibsInfoMap()->getFibInfo(scope);

    auto fibInfoPtr = fibInfo->modify(&nextState);
    CHECK(fibInfoPtr);
    auto id2Nhop = std::make_shared<IdToNextHopMap>();
    auto id2NhopSetIds = std::make_shared<IdToNextHopIdSetMap>();
    for (const auto& [id, nhop] : nextHopIDManager_->getIdToNextHop()) {
      id2Nhop->addNextHop(id, nhop.toThrift());
    }
    for (const auto& [id, nhopIdSet] :
         nextHopIDManager_->getIdToNextHopIdSet()) {
      auto toNhopIdsThrift = [](const auto& nhopIdSet) {
        std::set<int64_t> nhopIds;
        std::for_each(
            nhopIdSet.begin(), nhopIdSet.end(), [&nhopIds](const auto nhopId) {
              nhopIds.insert(static_cast<int64_t>(nhopId));
            });
        return nhopIds;
      };
      id2NhopSetIds->addNextHopIdSet(id, toNhopIdsThrift(nhopIdSet));
    }
    fibInfoPtr->setIdToNextHopMap(id2Nhop);
    fibInfoPtr->setIdToNextHopIdSetMap(id2NhopSetIds);
  }
  if (newFibV4) {
    nextFibContainer->ref<switch_state_tags::fibV4>() = std::move(newFibV4);
  }

  if (newFibV6) {
    nextFibContainer->ref<switch_state_tags::fibV6>() = std::move(newFibV6);
  }

  if (newLabelFib) {
    nextState->resetLabelForwardingInformationBase(newLabelFib);
  }
  // This will run on every unit test. We add this check to ensure that DCHECK
  // does not run when developers manually build and run agent-hw-tests in dev
  // mode.
  if (!FLAGS_verify_fib_nexthop_id_consistency) {
    DCHECK(verifyNextHopIdConsistency(nextState));
  }
  // This will run on tests wherever we set
  // FLAGS_verify_fib_nexthop_id_consistency We will only set this flag for
  // agent-hw-tests.
  else {
    CHECK(verifyNextHopIdConsistency(nextState));
  }

  return nextState;
}

template <typename AddressT>
std::shared_ptr<typename facebook::fboss::ForwardingInformationBase<AddressT>>
ForwardingInformationBaseUpdater::createUpdatedFib(
    const facebook::fboss::NetworkToRouteMap<AddressT>& rib,
    const std::shared_ptr<facebook::fboss::ForwardingInformationBase<AddressT>>&
        fib,
    std::shared_ptr<SwitchState>& state) {
  typename facebook::fboss::ForwardingInformationBase<
      AddressT>::Base::NodeContainer updatedFib;

  bool updated = false;
  for (const auto& entry : rib) {
    const auto& ribRoute = entry.value();

    if (!ribRoute->isResolved()) {
      // The recursive resolution algorithm considers a next-hop TO_CPU or
      // DROP to be resolved.
      continue;
    }

    // TODO(samank): optimize to linear time intersection algorithm
    facebook::fboss::RoutePrefix<AddressT> fibPrefix{
        ribRoute->prefix().network(), ribRoute->prefix().mask()};
    std::shared_ptr<facebook::fboss::Route<AddressT>> fibRoute =
        fib->getNodeIf(fibPrefix.str());

    if (fibRoute) {
      if (fibRoute == ribRoute || fibRoute->isSame(ribRoute.get())) {
        // Pointer or contents are same
      } else {
        // Route has changed
        fibRoute = ribRoute;
        updated = true;
      }
    } else {
      // New route
      fibRoute = ribRoute;
      updated = true;
    }
    CHECK(fibRoute->isPublished());
    updatedFib.emplace_hint(updatedFib.cend(), fibPrefix.str(), fibRoute);
  }
  // Check for deleted routes. Routes that were in the previous FIB
  // and have now been removed
  for (const auto& iter : std::as_const(*fib)) {
    const auto& fibEntry = iter.second;
    auto prefix = fibEntry->getID();
    if (updatedFib.find(prefix) == updatedFib.end()) {
      updated = true;
    }
  }

  DCHECK_EQ(
      updatedFib.size(),
      std::count_if(
          rib.begin(),
          rib.end(),
          [](const typename std::remove_reference_t<
              decltype(rib)>::ConstIterator::TreeNode& entry) {
            return entry.value()->isResolved();
          }));

  return updated ? std::make_shared<ForwardingInformationBase<AddressT>>(
                       std::move(updatedFib))
                 : nullptr;
}

std::shared_ptr<facebook::fboss::MultiLabelForwardingInformationBase>
ForwardingInformationBaseUpdater::createUpdatedLabelFib(
    const facebook::fboss::NetworkToRouteMap<LabelID>& rib,
    std::shared_ptr<facebook::fboss::MultiLabelForwardingInformationBase> fib) {
  if (!FLAGS_mpls_rib) {
    return nullptr;
  }

  bool updated = false;
  auto newFib = std::make_shared<MultiLabelForwardingInformationBase>();
  for (const auto& entry : rib) {
    const auto& label = entry.first;
    const auto& ribRoute = entry.second;
    if (!ribRoute->isResolved()) {
      // The recursive resolution algorithm considers a next-hop TO_CPU or
      // DROP to be resolved.
      continue;
    }
    auto fibRoute = fib->getNodeIf(label);
    if (fibRoute) {
      if (fibRoute == ribRoute || fibRoute->isSame(ribRoute.get())) {
        // Pointer or contents are same, reuse existing route
      } else {
        fibRoute = ribRoute;
        updated = true;
      }
    } else {
      fibRoute = ribRoute;
      updated = true;
    }
    if (!facebook::fboss::MultiLabelForwardingInformationBase::
            isValidNextHopSet(ribRoute->getForwardInfo().getNextHopSet())) {
      throw FbossError("invalid label next hop");
    }
    CHECK(fibRoute->isPublished());
    newFib->addNode(fibRoute, resolver_->scope(fibRoute));
  }
  // Check for deleted routes. Routes that were in the previous FIB
  // and have now been removed
  for (const auto& miter : std::as_const(*fib)) {
    for (const auto& iter : std::as_const(*miter.second)) {
      const auto& fibEntry = iter.second;
      const auto& label = fibEntry->getID();
      if (!newFib->getNodeIf(label)) {
        updated = true;
        break;
      }
    }
  }
  return updated ? newFib : nullptr;
}

bool ForwardingInformationBaseUpdater::verifyNextHopIdConsistency(
    const std::shared_ptr<SwitchState>& state) const {
  if (!nextHopIDManager_) {
    return true;
  }

  auto fibContainer = state->getFibsInfoMap()->getFibContainerIf(vrf_);
  if (!fibContainer) {
    return true;
  }
  XLOG(DBG2) << "Verifying FIB NextHop ID consistency";
  auto verifyNextHopIds = [&](const auto& route,
                              const std::optional<NextHopSetID>& setId,
                              const auto& expectedNhops,
                              const std::string& idType) -> bool {
    if (!setId) {
      return true;
    }
    XLOG(DBG3) << "Verifying " << idType << " for route " << route->str()
               << " with ID " << static_cast<int64_t>(*setId);
    auto reconstructedNhops =
        getNextHops(state, static_cast<NextHopSetId>(*setId));
    std::sort(reconstructedNhops.begin(), reconstructedNhops.end());

    std::vector<NextHop> expectedSorted(
        expectedNhops.begin(), expectedNhops.end());
    if (reconstructedNhops != expectedSorted) {
      XLOG(ERR) << "FIB NextHop ID consistency check failed for route "
                << route->str() << ": " << idType
                << " nexthops mismatch. Expected Inline Nexthops: "
                << RouteNextHopSet(expectedSorted.begin(), expectedSorted.end())
                << " Resolved NextHops from ID: "
                << RouteNextHopSet(
                       reconstructedNhops.begin(), reconstructedNhops.end());
      return false;
    }
    return true;
  };

  auto verifyRoutes = [&](const auto& fib) -> bool {
    for (const auto& [_, route] : std::as_const(*fib)) {
      if (!route->isResolved()) {
        continue;
      }
      const auto& fwdInfo = route->getForwardInfo();

      if (!verifyNextHopIds(
              route,
              fwdInfo.getResolvedNextHopSetID(),
              fwdInfo.getNextHopSet(),
              "resolvedNextHopSetID")) {
        return false;
      }
      if (fwdInfo.getNormalizedResolvedNextHopSetID() !=
          fwdInfo.getResolvedNextHopSetID()) {
        if (!verifyNextHopIds(
                route,
                fwdInfo.getNormalizedResolvedNextHopSetID(),
                fwdInfo.nonOverrideNormalizedNextHops(),
                "normalizedResolvedNextHopSetID")) {
          return false;
        }
      }
    }
    return true;
  };

  return verifyRoutes(fibContainer->getFibV4()) &&
      verifyRoutes(fibContainer->getFibV6());
}

} // namespace facebook::fboss
