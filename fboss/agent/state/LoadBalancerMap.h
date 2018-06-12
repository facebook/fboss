/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#pragma once

#include "fboss/agent/state/NodeMap.h"
#include "fboss/agent/types.h"

namespace facebook {
namespace fboss {

class LoadBalancer;

using LoadBalancerMapTraits = NodeMapTraits<LoadBalancerID, LoadBalancer>;

/*
 * A container for the set of load-balancing data-plane entities.
 */
class LoadBalancerMap
    : public NodeMapT<LoadBalancerMap, LoadBalancerMapTraits> {
 public:
  LoadBalancerMap();
  ~LoadBalancerMap() override;

 private:
  // Inherit the constructors required for clone()
  using NodeMapT::NodeMapT;
  friend class CloneAllocator;
};

} // namespace fboss
} // namespace facebook
