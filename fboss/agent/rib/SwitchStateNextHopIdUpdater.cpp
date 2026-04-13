/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/rib/SwitchStateNextHopIdUpdater.h"

#include "fboss/agent/FibHelpers.h"
#include "fboss/agent/state/FibInfo.h"
#include "fboss/agent/state/FibInfoMap.h"
#include "fboss/agent/state/NextHopIdMaps.h"
#include "fboss/agent/state/SwitchState.h"

#include <folly/logging/xlog.h>

namespace facebook::fboss {

SwitchStateNextHopIdUpdater::SwitchStateNextHopIdUpdater(
    const NextHopIDManager* nextHopIDManager)
    : nextHopIDManager_(nextHopIDManager) {}

std::shared_ptr<SwitchState> SwitchStateNextHopIdUpdater::operator()(
    const std::shared_ptr<SwitchState>& state) {
  if (!nextHopIDManager_) {
    return state;
  }

  auto nextState = state;

  // Build id maps once to share across all FibInfo nodes
  auto id2Nhop = std::make_shared<IdToNextHopMap>();
  auto id2NhopSetIds = std::make_shared<IdToNextHopIdSetMap>();
  for (const auto& [id, nhop] : nextHopIDManager_->getIdToNextHop()) {
    id2Nhop->addNextHop(id, nhop.toThrift());
  }
  for (const auto& [id, nhopIdSet] : nextHopIDManager_->getIdToNextHopIdSet()) {
    std::set<int64_t> nhopIds;
    for (auto nhopId : nhopIdSet) {
      nhopIds.insert(static_cast<int64_t>(nhopId));
    }
    id2NhopSetIds->addNextHopIdSet(id, nhopIds);
  }

  // Update id maps in each FibInfo across all switches
  for (const auto& [matcher, _] : std::as_const(*state->getFibsInfoMap())) {
    auto fibInfo = nextState->getFibsInfoMap()->getNodeIf(matcher);
    if (!fibInfo) {
      continue;
    }
    auto fibInfoPtr = fibInfo->modify(&nextState);
    fibInfoPtr->setIdToNextHopMap(id2Nhop);
    fibInfoPtr->setIdToNextHopIdSetMap(id2NhopSetIds);
  }

  return nextState;
}

bool SwitchStateNextHopIdUpdater::verifyNextHopIdConsistency(
    const std::shared_ptr<SwitchState>& state) const {
  if (!nextHopIDManager_) {
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

      // Every resolved NEXTHOPS route must have IDs assigned
      if (fwdInfo.getAction() == RouteForwardAction::NEXTHOPS) {
        if (!fwdInfo.getResolvedNextHopSetID().has_value()) {
          XLOG(ERR) << "Resolved NEXTHOPS route " << route->str()
                    << " is missing resolvedNextHopSetID";
          return false;
        }
        if (!fwdInfo.getNormalizedResolvedNextHopSetID().has_value()) {
          XLOG(ERR) << "Resolved NEXTHOPS route " << route->str()
                    << " is missing normalizedResolvedNextHopSetID";
          return false;
        }
      }

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

  for (const auto& [matcherStr, fibInfo] :
       std::as_const(*state->getFibsInfoMap())) {
    auto fibsMap = fibInfo->getfibsMap();
    if (!fibsMap) {
      continue;
    }
    for (const auto& [vrfStr, fibContainer] : std::as_const(*fibsMap)) {
      if (!verifyRoutes(fibContainer->getFibV4()) ||
          !verifyRoutes(fibContainer->getFibV6())) {
        return false;
      }
    }
  }

  return true;
}

} // namespace facebook::fboss
