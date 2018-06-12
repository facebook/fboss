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

namespace facebook {
namespace fboss {

LoadBalancerMap::LoadBalancerMap() {}

LoadBalancerMap::~LoadBalancerMap() {}

std::shared_ptr<LoadBalancer> LoadBalancerMap::getLoadBalancerIf(
    LoadBalancerID id) const {
  return getNodeIf(id);
}

FBOSS_INSTANTIATE_NODE_MAP(LoadBalancerMap, LoadBalancerMapTraits);

} // namespace fboss
} // namespace facebook
