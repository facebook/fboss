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

using LoadBalancerMapTraits = NodeMapTraits<LoadBalancerID, LoadBalancer>;

struct LoadBalancerMapThriftTraits
    : public ThriftyNodeMapTraits<LoadBalancerID, state::LoadBalancerFields> {
  static inline const std::string& getThriftKeyName() {
    static const std::string _key = "id";
    return _key;
  }

  static KeyType parseKey(const folly::dynamic& key) {
    auto keyInt = key.asInt();
    switch (keyInt) {
      case static_cast<int>(LoadBalancerID::ECMP):
      case static_cast<int>(LoadBalancerID::AGGREGATE_PORT):
        break;
      default:
        throw FbossError("invalid load balancer key:", keyInt);
    }
    return folly::to<LoadBalancerID>(keyInt);
  }
};

/*
 * A container for the set of load-balancing data-plane entities.
 */
class LoadBalancerMap : public ThriftyNodeMapT<
                            LoadBalancerMap,
                            LoadBalancerMapTraits,
                            LoadBalancerMapThriftTraits> {
 public:
  LoadBalancerMap();
  ~LoadBalancerMap() override;

  std::shared_ptr<LoadBalancer> getLoadBalancerIf(LoadBalancerID id) const;

  void addLoadBalancer(std::shared_ptr<LoadBalancer> loadBalancer);
  void updateLoadBalancer(std::shared_ptr<LoadBalancer> loadBalancer);

  static std::shared_ptr<LoadBalancerMap> fromFollyDynamicLegacy(
      const folly::dynamic& serializedLoadBalancers);

 private:
  // Inherit the constructors required for clone()
  using ThriftyNodeMapT::ThriftyNodeMapT;
  friend class CloneAllocator;
};

} // namespace facebook::fboss
