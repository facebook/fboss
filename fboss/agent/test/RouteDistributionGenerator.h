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

#include "fboss/agent/platforms/common/PlatformMode.h"
#include "fboss/agent/types.h"

#include <folly/IPAddress.h>
#include <map>
#include <memory>
#include <vector>

namespace facebook::fboss {
class SwitchState;
}

namespace facebook::fboss::utility {

using Masklen2NumPrefixes = std::map<uint8_t, uint32_t>;

/*
 * RouteDistributionGenerator takes a input state, distribution spec and
 * chunk size to generate a vector of vector<Routes> (chunk) satisfying that
 * distribution spec and chunk size.
 */
class RouteDistributionGenerator {
 public:
  struct Route {
    folly::CIDRNetwork prefix;
    std::vector<folly::IPAddress> nhops;
  };
  using RouteChunk = std::vector<Route>;
  using RouteChunks = std::vector<RouteChunk>;
  using SwitchStates = std::vector<std::shared_ptr<SwitchState>>;

  RouteDistributionGenerator(
      const std::shared_ptr<SwitchState>& startingState,
      const Masklen2NumPrefixes& v6DistributionSpec,
      const Masklen2NumPrefixes& v4DistributionSpec,
      unsigned int chunkSize,
      unsigned int ecmpWidth,
      RouterID routerId = RouterID(0));
  /*
   * Compute, cache and return route distribution
   */
  const RouteChunks& get() const;
  const SwitchStates& getSwitchStates() const;

  std::shared_ptr<SwitchState> startingState() const {
    return startingState_;
  }
  unsigned int ecmpWidth() const {
    return ecmpWidth_;
  }
  unsigned int chunkSize() const {
    return chunkSize_;
  }

  bool isSupported(PlatformMode /*mode*/) const {
    return true;
  }

 protected:
  std::optional<SwitchStates>& getGeneratedStates() const {
    return generatedStates_;
  }
  std::optional<SwitchStates>& getGeneratedStates() {
    return generatedStates_;
  }

 private:
  template <typename AddrT>
  const std::vector<folly::IPAddress>& getNhops() const;
  template <typename AddT>
  void genRouteDistribution(const Masklen2NumPrefixes& routeDistribution) const;

  const std::shared_ptr<SwitchState> startingState_;
  const Masklen2NumPrefixes v6DistributionSpec_;
  const Masklen2NumPrefixes v4DistributionSpec_;
  const unsigned int chunkSize_;
  const unsigned int ecmpWidth_;
  const RouterID routerId_{0};
  /*
   * Caches for genertated chunks and states. Mark mutable since
   * caching is just a optimization doesn't and doesn't reflect the
   * essential state of this class. So allow modifying from const
   * methods.
   */
  mutable std::optional<RouteChunks> generatedRouteChunks_;
  mutable std::optional<SwitchStates> generatedStates_;
};

} // namespace facebook::fboss::utility
