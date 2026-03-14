/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/state/FibInfo.h"
#include "fboss/agent/state/ForwardingInformationBaseMap.h"
#include "fboss/agent/state/SwitchState.h"

namespace facebook::fboss {

FibInfo* FibInfo::modify(std::shared_ptr<SwitchState>* state) {
  if (!isPublished()) {
    CHECK(!(*state)->isPublished());
    return this;
  }

  auto fibInfoMap = (*state)->getFibsInfoMap()->modify(state);

  // find this FibInfo in the map
  for (const auto& [matcherStr, fibInfo] : std::as_const(*fibInfoMap)) {
    if (fibInfo.get() == this) {
      auto newFibInfo = clone();
      fibInfoMap->updateNode(matcherStr, newFibInfo);
      return newFibInfo.get();
    }
  }

  throw FbossError("FibInfo not found in FibInfoMap");
}

std::pair<uint64_t, uint64_t> FibInfo::getRouteCount() const {
  auto fibsMap = getfibsMap();
  if (!fibsMap) {
    return {0, 0};
  }
  return fibsMap->getRouteCount();
}

void FibInfo::updateFibContainer(
    const std::shared_ptr<ForwardingInformationBaseContainer>& fibContainer,
    std::shared_ptr<SwitchState>* state) {
  auto modifiedFibInfo = modify(state);

  auto fibsMap = modifiedFibInfo->getfibsMap()->clone();
  fibsMap->updateForwardingInformationBaseContainer(fibContainer);

  modifiedFibInfo->ref<switch_state_tags::fibsMap>() = fibsMap;
}

std::vector<NextHop> FibInfo::resolveNextHopSetFromId(NextHopSetId id) const {
  auto idToNextHopIdSetMap = getIdToNextHopIdSetMap();
  if (!idToNextHopIdSetMap) {
    throw FbossError("IdToNextHopIdSetMap is not initialized");
  }

  auto nextHopIdSetNode = idToNextHopIdSetMap->getNextHopIdSetIf(id);
  if (!nextHopIdSetNode) {
    throw FbossError("NextHopSetId ", id, " not found");
  }

  auto idToNextHopMap = getIdToNextHopMap();
  if (!idToNextHopMap) {
    throw FbossError("IdToNextHopMap is not initialized");
  }

  std::vector<NextHop> nextHops;
  nextHops.reserve(nextHopIdSetNode->size());
  for (const auto& elem : std::as_const(*nextHopIdSetNode)) {
    auto nextHopId = (*elem).toThrift();
    auto nextHopNode = idToNextHopMap->getNextHopIf(nextHopId);
    if (!nextHopNode) {
      throw FbossError("NextHopId ", nextHopId, " not found in IdToNextHopMap");
    }
    nextHops.push_back(
        util::fromThrift(
            nextHopNode->toThrift(), true /* allowV6NonLinkLocal */));
  }

  return nextHops;
}

std::optional<NextHopSetId> FibInfo::getNextHopSetIdIf(
    const std::string& name) const {
  auto nameToIdMap = safe_cref<switch_state_tags::nameToNextHopSetId>();
  if (!nameToIdMap) {
    return std::nullopt;
  }
  auto iter = nameToIdMap->find(name);
  if (iter == nameToIdMap->end()) {
    return std::nullopt;
  }
  return iter->second->toThrift();
}

NextHopSetId FibInfo::getNextHopSetId(const std::string& name) const {
  auto idOpt = getNextHopSetIdIf(name);
  if (!idOpt) {
    throw FbossError("Named next-hop group '", name, "' not found");
  }
  return *idOpt;
}

void FibInfo::setNextHopSetIdForName(const std::string& name, NextHopSetId id) {
  ref<switch_state_tags::nameToNextHopSetId>()->emplace(name, id);
}

void FibInfo::removeNextHopSetForName(const std::string& name) {
  ref<switch_state_tags::nameToNextHopSetId>()->remove(name);
}

std::vector<NextHop> FibInfo::resolveNextHopSetFromName(
    const std::string& name) const {
  auto idOpt = getNextHopSetIdIf(name);
  if (!idOpt) {
    throw FbossError("Named next-hop group '", name, "' not found");
  }
  return resolveNextHopSetFromId(*idOpt);
}

template struct ThriftStructNode<FibInfo, state::FibInfoFields>;

} // namespace facebook::fboss
