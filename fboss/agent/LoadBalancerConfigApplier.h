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

#include <memory>
#include <vector>

#include <boost/container/flat_set.hpp>

#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/state/LoadBalancer.h"
#include "fboss/agent/state/LoadBalancerMap.h"
#include "fboss/agent/types.h"

class Platform;

namespace facebook {
namespace fboss {

/* LoadBalancerConfigApplier parses and validates a list of LoadBalancer Thrift
 * config structures and constructs a LoadBalancerMap SwitchState node
 * corresponding to this config.
 *
 * To retain the convention used in ApplyThriftConfig.cpp, some of the method
 * signatures below are awkward.
 */
class LoadBalancerConfigApplier {
 public:
  LoadBalancerConfigApplier(
      const std::shared_ptr<LoadBalancerMap>& originalLoadBalancers,
      const std::vector<cfg::LoadBalancer>& loadBalancersConfig,
      const Platform* platform);
  ~LoadBalancerConfigApplier();

  // Returns a LoadBalancerMap SwitchState object derived from
  // loadBalancersConfig_ or null if the resulting LoadBalancerMap
  // is equivalent to originalLoadBalancers_.
  std::shared_ptr<LoadBalancerMap> updateLoadBalancers();

 private:
  // Forbidden copy constructor and assignment operator
  LoadBalancerConfigApplier(const LoadBalancerConfigApplier&) = delete;
  LoadBalancerConfigApplier& operator=(const LoadBalancerConfigApplier&) =
      delete;

  LoadBalancerID parseLoadBalancerID(
      const cfg::LoadBalancer& loadBalancerID) const;
  std::tuple<
      LoadBalancer::IPv4Fields,
      LoadBalancer::IPv6Fields,
      LoadBalancer::TransportFields>
  parseFields(const cfg::LoadBalancer& cfg) const;

  // The newly created LoadBalancer is returned via std::shared_ptr out of
  // convenience (see callsite in updateLoadBalancer()); the caller is the
  // sole owner of the newly constructed LoadBalancer.
  std::shared_ptr<LoadBalancer> createLoadBalancer(
      const cfg::LoadBalancer& cfg) const;
  void appendToLoadBalancerContainer(
      LoadBalancerMap::NodeContainer* loadBalancerContainer,
      std::shared_ptr<LoadBalancer> loadBalancer) const;

  const std::shared_ptr<LoadBalancerMap>& originalLoadBalancers_;
  const std::vector<cfg::LoadBalancer>& loadBalancersConfig_;
  const Platform* platform_;
};

} // namespace fboss
} // namespace facebook
