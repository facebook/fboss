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

#include <memory>

namespace facebook::fboss {

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

  std::shared_ptr<LoadBalancer> getLoadBalancerIf(LoadBalancerID id) const;

  void addLoadBalancer(std::shared_ptr<LoadBalancer> loadBalancer);

  folly::dynamic toFollyDynamic() const override;
  static std::shared_ptr<LoadBalancerMap> fromFollyDynamic(
      const folly::dynamic& serializedLoadBalancers);

 private:
  // Inherit the constructors required for clone()
  using NodeMapT::NodeMapT;
  friend class CloneAllocator;
};

} // namespace facebook::fboss
