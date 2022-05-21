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
#include "fboss/agent/state/LoadBalancer.h"
#include "fboss/agent/state/NodeMap-defs.h"

namespace facebook::fboss {

LoadBalancerMap::LoadBalancerMap() {}

LoadBalancerMap::~LoadBalancerMap() {}

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

folly::dynamic LoadBalancerMap::toFollyDynamic() const {
  folly::dynamic serializedLoadBalancers = folly::dynamic::array;

  for (const auto& loadBalancer : *this) {
    auto serializedLoadBalancer = loadBalancer->toFollyDynamic();
    serializedLoadBalancers.push_back(serializedLoadBalancer);
  }

  return serializedLoadBalancers;
}

std::shared_ptr<LoadBalancerMap> LoadBalancerMap::fromFollyDynamic(
    const folly::dynamic& serializedLoadBalancers) {
  if (serializedLoadBalancers.isObject()) {
    return NodeMapT<LoadBalancerMap, LoadBalancerMapTraits>::fromFollyDynamic(
        serializedLoadBalancers);
  }
  // old way to save load balancer map was to save it as an array instead of map
  // until the new way of saving it as a map is in prod. support both ways.
  auto deserializedLoadBalancers = std::make_shared<LoadBalancerMap>();

  for (const auto& serializedLoadBalancer : serializedLoadBalancers) {
    auto deserializedLoadBalancer =
        LoadBalancer::fromFollyDynamic(serializedLoadBalancer);
    deserializedLoadBalancers->addLoadBalancer(deserializedLoadBalancer);
  }

  return deserializedLoadBalancers;
}

FBOSS_INSTANTIATE_NODE_MAP(LoadBalancerMap, LoadBalancerMapTraits);

} // namespace facebook::fboss
