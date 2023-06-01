/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/state/LoadBalancerMap.h"
#include "fboss/agent/Constants.h"
#include "fboss/agent/state/LoadBalancer.h"
#include "fboss/agent/state/NodeMap-defs.h"
#include "fboss/agent/state/SwitchState.h"

namespace facebook::fboss {

std::shared_ptr<LoadBalancer> LoadBalancerMap::getLoadBalancerIf(
    LoadBalancerID id) const {
  return getNodeIf(id);
}

void LoadBalancerMap::addLoadBalancer(
    std::shared_ptr<LoadBalancer> loadBalancer) {
  addNode(loadBalancer);
}
void LoadBalancerMap::updateLoadBalancer(
    std::shared_ptr<LoadBalancer> loadBalancer) {
  updateNode(loadBalancer);
}

MultiSwitchLoadBalancerMap* MultiSwitchLoadBalancerMap::modify(
    std::shared_ptr<SwitchState>* state) {
  if (!isPublished()) {
    CHECK(!(*state)->isPublished());
    return this;
  }

  SwitchState::modify(state);
  auto map = clone();
  for (auto mnitr = cbegin(); mnitr != cend(); ++mnitr) {
    (*map)[mnitr->first] = mnitr->second->clone();
  }
  auto* ptr = map.get();
  (*state)->resetLoadBalancers(std::move(map));
  return ptr;
}

template class ThriftMapNode<LoadBalancerMap, LoadBalancerMapTraits>;

} // namespace facebook::fboss
