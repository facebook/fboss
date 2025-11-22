/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/state/FibInfoMap.h"
#include "fboss/agent/HwSwitchMatcher.h"
#include "fboss/agent/state/SwitchState.h"

namespace facebook::fboss {

MultiSwitchFibInfoMap* MultiSwitchFibInfoMap::modify(
    std::shared_ptr<SwitchState>* state) {
  if (!isPublished()) {
    CHECK(!(*state)->isPublished());
    return this;
  }

  SwitchState::modify(state);
  auto newFibInfoMap = clone();
  auto* rtn = newFibInfoMap.get();
  (*state)->resetFibsInfoMap(std::move(newFibInfoMap));

  return rtn;
}

void MultiSwitchFibInfoMap::updateFibInfo(
    const std::shared_ptr<FibInfo>& fibInfo,
    const HwSwitchMatcher& matcher) {
  auto matcherStr = matcher.matcherString();
  if (getNodeIf(matcherStr)) {
    updateNode(matcherStr, fibInfo);
  } else {
    addNode(matcherStr, fibInfo);
  }
}

std::pair<uint64_t, uint64_t> MultiSwitchFibInfoMap::getRouteCount() const {
  uint64_t v4Count{0}, v6Count{0};
  for (const auto& [_, fibInfo] : std::as_const(*this)) {
    auto [v4, v6] = fibInfo->getRouteCount();
    v4Count += v4;
    v6Count += v6;
  }
  return {v4Count, v6Count};
}

std::shared_ptr<ForwardingInformationBaseMap>
MultiSwitchFibInfoMap::getAllFibNodes() const {
  auto mergedMap = std::make_shared<ForwardingInformationBaseMap>();

  // Iterate through all switches
  for (const auto& [_, fibInfo] : std::as_const(*this)) {
    // Get the ForwardingInformationBaseMap for this switch
    auto fibsMap = fibInfo->getfibsMap();

    // Merge all FibContainers from this switch's map
    for (const auto& [routerId, fibContainer] : std::as_const(*fibsMap)) {
      mergedMap->updateForwardingInformationBaseContainer(fibContainer);
    }
  }

  return mergedMap;
}

} // namespace facebook::fboss
