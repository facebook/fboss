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

  const auto& ribNhopMap = nextHopIDManager_->getIdToNextHop();
  const auto& ribSetMap = nextHopIDManager_->getIdToNextHopIdSet();

  // fibsInfoMap is always populated for any switch with a switchId; if the
  // nextHopIDManager exists, that invariant must hold.
  auto fibsInfoMap = state->getFibsInfoMap();
  CHECK(!fibsInfoMap->empty());
  // FIB maps are shared across all switches, pick from first
  const auto& [_, fibInfo] = *fibsInfoMap->cbegin();
  auto fibNhopMap = fibInfo->getIdToNextHopMap();
  auto fibSetMap = fibInfo->getIdToNextHopIdSetMap();
  auto fibNameMap = fibInfo->getNameToNextHopSetId();

  bool nhopChanged = false;
  bool setChanged = false;
  bool namedChanged = false;

  // Diff nhop maps: clone FIB, add missing from RIB, remove stale from FIB
  auto updatedNhopMap =
      fibNhopMap ? fibNhopMap->clone() : std::make_shared<IdToNextHopMap>();
  for (const auto& [id, nextHopEntry] : ribNhopMap) {
    if (!fibNhopMap || !fibNhopMap->getNextHopIf(id)) {
      updatedNhopMap->addNextHop(id, nextHopEntry.nextHop.toThrift());
      nhopChanged = true;
    }
  }
  if (fibNhopMap) {
    for (const auto& [id, _] : std::as_const(*fibNhopMap)) {
      if (ribNhopMap.find(NextHopID(id)) == ribNhopMap.end()) {
        updatedNhopMap->removeNextHopIf(id);
        nhopChanged = true;
      }
    }
  }

  // Diff set maps: clone FIB, add missing from RIB, remove stale from FIB
  auto updatedSetMap =
      fibSetMap ? fibSetMap->clone() : std::make_shared<IdToNextHopIdSetMap>();
  for (const auto& [id, nextHopIdSet] : ribSetMap) {
    if (!fibSetMap || !fibSetMap->getNextHopIdSetIf(id)) {
      std::set<int64_t> nhopIds;
      for (auto nhopId : nextHopIdSet) {
        nhopIds.insert(static_cast<int64_t>(nhopId));
      }
      updatedSetMap->addNextHopIdSet(id, nhopIds);
      setChanged = true;
    }
  }
  if (fibSetMap) {
    for (const auto& [id, _] : std::as_const(*fibSetMap)) {
      if (ribSetMap.find(NextHopSetID(id)) == ribSetMap.end()) {
        updatedSetMap->removeNextHopIdSetIf(id);
        setChanged = true;
      }
    }
  }

  // Diff named next-hop group maps
  const auto& ribNameMap = nextHopIDManager_->getNameToNextHopSetID();
  auto updatedNameMap = fibNameMap;
  for (const auto& [name, setId] : ribNameMap) {
    auto it = fibNameMap.find(name);
    if (it == fibNameMap.end() ||
        it->second != static_cast<NextHopSetId>(setId)) {
      updatedNameMap[name] = static_cast<NextHopSetId>(setId);
      namedChanged = true;
    }
  }
  for (const auto& [name, _] : fibNameMap) {
    if (ribNameMap.find(name) == ribNameMap.end()) {
      updatedNameMap.erase(name);
      namedChanged = true;
    }
  }

  if (!nhopChanged && !setChanged && !namedChanged) {
    return state;
  }

  XLOG(DBG3) << "[NextHop ID Manager] updater diff: nhopChanged=" << nhopChanged
             << " setChanged=" << setChanged << " namedChanged=" << namedChanged
             << " ribNhops=" << ribNhopMap.size()
             << " ribSets=" << ribSetMap.size()
             << " ribNamed=" << ribNameMap.size();

  // Apply only the maps that actually changed
  auto nextState = state;
  for (const auto& [matcher, _] : std::as_const(*state->getFibsInfoMap())) {
    auto perSwitchFibInfo = nextState->getFibsInfoMap()->getNodeIf(matcher);
    if (!perSwitchFibInfo) {
      continue;
    }
    auto fibInfoPtr = perSwitchFibInfo->modify(&nextState);
    if (nhopChanged) {
      fibInfoPtr->setIdToNextHopMap(updatedNhopMap);
    }
    if (setChanged) {
      fibInfoPtr->setIdToNextHopIdSetMap(updatedSetMap);
    }
    if (namedChanged) {
      fibInfoPtr->setNameToNextHopSetId(updatedNameMap);
    }
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
