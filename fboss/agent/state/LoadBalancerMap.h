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

#include "fboss/agent/FbossError.h"
#include "fboss/agent/state/NodeMap.h"
#include "fboss/agent/types.h"

#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/state/LoadBalancer.h"
#include "fboss/agent/state/Thrifty.h"

#include <memory>

namespace facebook::fboss {

using LoadBalancerMapTypeClass = apache::thrift::type_class::map<
    apache::thrift::type_class::enumeration,
    apache::thrift::type_class::structure>;
using LoadBalancerMapThriftType =
    std::map<cfg::LoadBalancerID, state::LoadBalancerFields>;
class LoadBalancerMap;
using LoadBalancerMapTraits = ThriftMapNodeTraits<
    LoadBalancerMap,
    LoadBalancerMapTypeClass,
    LoadBalancerMapThriftType,
    LoadBalancer>;

/*
 * A container for the set of load-balancing data-plane entities.
 */
class LoadBalancerMap
    : public ThriftMapNode<LoadBalancerMap, LoadBalancerMapTraits> {
 public:
  using Base = ThriftMapNode<LoadBalancerMap, LoadBalancerMapTraits>;

  using Base::Base;

  LoadBalancerMap() {}
  ~LoadBalancerMap() override {}
  std::shared_ptr<LoadBalancer> getLoadBalancerIf(LoadBalancerID id) const;

  void addLoadBalancer(std::shared_ptr<LoadBalancer> loadBalancer);
  void updateLoadBalancer(std::shared_ptr<LoadBalancer> loadBalancer);

 private:
  // Inherit the constructors required for clone()
  friend class CloneAllocator;
};

} // namespace facebook::fboss
