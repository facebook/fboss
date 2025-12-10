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

template struct ThriftStructNode<FibInfo, state::FibInfoFields>;

} // namespace facebook::fboss
