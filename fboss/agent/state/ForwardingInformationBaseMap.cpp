/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/state/ForwardingInformationBaseMap.h"
#include "fboss/agent/state/NodeMap-defs.h"

#include "fboss/agent/state/SwitchState.h"

#include "fboss/agent/HwSwitchMatcher.h"

namespace facebook::fboss {

ForwardingInformationBaseMap::ForwardingInformationBaseMap() {}

ForwardingInformationBaseMap::~ForwardingInformationBaseMap() {}

std::shared_ptr<ForwardingInformationBaseContainer>
ForwardingInformationBaseMap::getFibContainerIf(RouterID vrf) const {
  return getNodeIf(vrf);
}

std::pair<uint64_t, uint64_t> ForwardingInformationBaseMap::getRouteCount()
    const {
  uint64_t v4Count = 0;
  uint64_t v6Count = 0;

  for (const auto& iter : std::as_const(*this)) {
    const auto& fibContainer = iter.second;
    v4Count += fibContainer->getFibV4()->size();
    v6Count += fibContainer->getFibV6()->size();
  }

  return std::make_pair(v4Count, v6Count);
}

std::shared_ptr<ForwardingInformationBaseContainer>
ForwardingInformationBaseMap::getFibContainer(RouterID vrf) const {
  std::shared_ptr<ForwardingInformationBaseContainer> fibContainer =
      getFibContainerIf(vrf);

  if (fibContainer) {
    return fibContainer;
  }

  throw FbossError("No ForwardingInformationBaseContainer found for VRF ", vrf);
}

void ForwardingInformationBaseMap::updateForwardingInformationBaseContainer(
    const std::shared_ptr<ForwardingInformationBaseContainer>& fibContainer) {
  if (getNodeIf(fibContainer->getID())) {
    updateNode(fibContainer);
  } else {
    addNode(fibContainer);
  }
}

MultiSwitchForwardingInformationBaseMap*
MultiSwitchForwardingInformationBaseMap::modify(
    std::shared_ptr<SwitchState>* state) {
  if (!isPublished()) {
    CHECK(!(*state)->isPublished());
    return this;
  }

  SwitchState::modify(state);
  auto newFibMap = clone();
  auto* rtn = newFibMap.get();
  (*state)->resetForwardingInformationBases(std::move(newFibMap));

  return rtn;
}

void MultiSwitchForwardingInformationBaseMap::
    updateForwardingInformationBaseContainer(
        const std::shared_ptr<ForwardingInformationBaseContainer>& fibContainer,
        const HwSwitchMatcher& matcher) {
  auto node = getNodeIf(fibContainer->getID());
  if (node) {
    updateNode(fibContainer, matcher);
  } else {
    addNode(fibContainer, matcher);
  }
}

std::pair<uint64_t, uint64_t>
MultiSwitchForwardingInformationBaseMap::getRouteCount() const {
  uint64_t v4Count = 0;
  uint64_t v6Count = 0;

  for (const auto& [_, fibs] : std::as_const(*this)) {
    auto [v4, v6] = fibs->getRouteCount();
    v4Count += v4;
    v6Count += v6;
  }
  return std::make_pair(v4Count, v6Count);
}

template class ThriftMapNode<
    ForwardingInformationBaseMap,
    ForwardingInformationBaseMapTraits>;

} // namespace facebook::fboss
