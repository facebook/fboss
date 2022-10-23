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
class LoadBalancerMap
    : public NodeMapT<LoadBalancerMap, LoadBalancerMapTraits> {
 public:
  using BaseT = NodeMapT<LoadBalancerMap, LoadBalancerMapTraits>;
  LoadBalancerMap();
  ~LoadBalancerMap() override;

  std::shared_ptr<LoadBalancer> getLoadBalancerIf(LoadBalancerID id) const;

  void addLoadBalancer(std::shared_ptr<LoadBalancer> loadBalancer);
  void updateLoadBalancer(std::shared_ptr<LoadBalancer> loadBalancer);

  static std::shared_ptr<LoadBalancerMap> fromFollyDynamicLegacy(
      const folly::dynamic& serializedLoadBalancers);

  static std::shared_ptr<LoadBalancerMap> fromThrift(
      const std::map<LoadBalancerID, state::LoadBalancerFields>& lbs) {
    auto map = std::make_shared<LoadBalancerMap>();
    for (auto& entry : lbs) {
      map->addNode(LoadBalancer::fromThrift(entry.second));
    }
    return map;
  }

  std::map<LoadBalancerID, state::LoadBalancerFields> toThrift() const {
    std::map<LoadBalancerID, state::LoadBalancerFields> lbs{};
    for (auto entry : getAllNodes()) {
      lbs.emplace(entry.first, entry.second->toThrift());
    }
    return lbs;
  }

  bool operator==(const LoadBalancerMap& that) const {
    if (size() != that.size()) {
      return false;
    }
    auto iter = begin();
    while (iter != end()) {
      auto lb = *iter;
      auto other = that.getLoadBalancerIf(lb->getID());
      if (!other || *lb != *other) {
        return false;
      }
      iter++;
    }
    return true;
  }

  bool operator!=(const LoadBalancerMap& other) const {
    return !(*this == other);
  }

 private:
  // Inherit the constructors required for clone()
  using BaseT::BaseT;
  friend class CloneAllocator;
};

} // namespace facebook::fboss
