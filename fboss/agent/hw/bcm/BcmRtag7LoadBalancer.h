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

#include <boost/container/flat_map.hpp>
#include "fboss/agent/types.h"

extern "C" {
#include <bcm/switch.h>
}

namespace facebook::fboss {

class BcmRtag7Module;
class BcmSwitch;
class LoadBalancer;

/* The BcmRtag7LoadBalancer singleton manages the RTAG7 load-balancing engine.
 *
 * It is responsible for allocating the resources of this engine so as to
 * achieve the load-balancing requirements of SwitchState's LoadBalancerTable
 * (or report that the configuration is not possible to implement).
 *
 * Presently, BcmRtag7LoaBalancer leverages the following facts to perform a
 * a trivial resource allocation:
 *
 * - Configuration constrains the maximum number of LoadBalancers that may exist
 *   in the system to two (ECMP and LAG).
 * - The RTAG7 engine exposes _two_ identical load-balancing modules that can be
 *   programmed independently of one another.
 *
 * RTAG7 resource allocation is then performed by dedicating one module to ECMP
 * and the other to LAG. In this way, all load-balancing configurations are
 * permissible.
 *
 * If a third load-balancing point is added in the future, this class would
 * have to be extended to share one of the two modules between two
 * load-balancers.
 */
class BcmRtag7LoadBalancer {
 public:
 public:
  explicit BcmRtag7LoadBalancer(const BcmSwitch* hw);
  ~BcmRtag7LoadBalancer();

  // Prior to the invocation of the following 3 functions, the global HW update
  // lock (BcmSwitch::lock_) should have been acquired.
  void addLoadBalancer(const std::shared_ptr<LoadBalancer>& loadBalancer);
  void programLoadBalancer(
      const std::shared_ptr<LoadBalancer>& oldLoadBalancer,
      const std::shared_ptr<LoadBalancer>& newLoadBalancer);
  void deleteLoadBalancer(const std::shared_ptr<LoadBalancer>& loadBalancer);

 private:
  // Forbidden copy constructor and assignment operator
  BcmRtag7LoadBalancer(const BcmRtag7LoadBalancer&) = delete;
  BcmRtag7LoadBalancer& operator=(const BcmRtag7LoadBalancer&) = delete;

  /*
   * The following table encodes the allocation scheme outlined above: each
   * module is dedicated to one LoadBalancer.
   */
  using LoadBalancerIDToBcmRtag7Module = boost::container::
      flat_map<LoadBalancerID, std::unique_ptr<BcmRtag7Module>>;
  LoadBalancerIDToBcmRtag7Module rtag7Modules_;

  const BcmSwitch* const hw_;
};

} // namespace facebook::fboss
