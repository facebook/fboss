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

#include "fboss/agent/FibHelpers.h"
#include "fboss/agent/if/gen-cpp2/ctrl_types.h"
#include "fboss/agent/state/RouteNextHop.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/test/ResourceLibUtil.h"
#include "fboss/agent/types.h"
#include "fboss/lib/if/gen-cpp2/fboss_common_types.h"

#include <folly/IPAddress.h>
#include <map>
#include <memory>
#include <vector>

namespace facebook::fboss {
class SwitchState;
}

namespace facebook::fboss::utility {

template <typename AddrT>
folly::CIDRNetwork getNewPrefix(
    PrefixGenerator<AddrT>& prefixGenerator,
    const std::shared_ptr<facebook::fboss::SwitchState>& state,
    facebook::fboss::RouterID routerId) {
  // Obtain a new prefix.
  auto prefix = prefixGenerator.getNext();
  while (findRoute<AddrT>(routerId, {prefix.network(), prefix.mask()}, state)) {
    prefix = prefixGenerator.getNext();
  }
  return folly::CIDRNetwork{{prefix.network()}, prefix.mask()};
}

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
    std::vector<UnresolvedNextHop> nhops;
  };
  using RouteChunk = std::vector<Route>;
  using RouteChunks = std::vector<RouteChunk>;
  using ThriftRouteChunk = std::vector<UnicastRoute>;
  using ThriftRouteChunks = std::vector<ThriftRouteChunk>;

  RouteDistributionGenerator(
      const std::shared_ptr<SwitchState>& startingState,
      const Masklen2NumPrefixes& v6DistributionSpec,
      const Masklen2NumPrefixes& v4DistributionSpec,
      unsigned int chunkSize,
      unsigned int ecmpWidth,
      RouterID routerId = RouterID(0));

  virtual ~RouteDistributionGenerator() = default;
  virtual std::shared_ptr<SwitchState> resolveNextHops(
      std::shared_ptr<SwitchState> in) const;
  /*
   * Compute, cache and return route distribution
   */
  const RouteChunks& get() const;
  const ThriftRouteChunks& getThriftRoutes(
      std::optional<RouteCounterID> counterID = std::nullopt) const;

  /*
   * All routes as a single chunk
   */
  RouteChunk allRoutes() const;
  ThriftRouteChunk allThriftRoutes() const;

  std::shared_ptr<SwitchState> startingState() const {
    return startingState_;
  }
  unsigned int ecmpWidth() const {
    return ecmpWidth_;
  }
  unsigned int chunkSize() const {
    return chunkSize_;
  }

  bool isSupported(PlatformType /*type*/) const {
    return true;
  }

 protected:
  const RouterID getRouterID() const {
    return routerId_;
  }

 private:
  template <typename AddrT>
  const std::vector<UnresolvedNextHop>& getNhops() const;

  const std::shared_ptr<SwitchState> startingState_;
  const Masklen2NumPrefixes v6DistributionSpec_;
  const Masklen2NumPrefixes v4DistributionSpec_;
  const unsigned int chunkSize_;
  const unsigned int ecmpWidth_;
  const RouterID routerId_{0};

 protected:
  template <typename AddT>
  void genRouteDistribution(const Masklen2NumPrefixes& routeDistribution) const;
  virtual void genRoutes() const;
  /*
   * Caches for generated chunks. Mark mutable since caching is just a
   * optimization doesn't and doesn't reflect the
   * essential state of this class. So allow modifying from const
   * methods.
   */
  mutable std::optional<RouteChunks> generatedRouteChunks_;
  mutable std::optional<ThriftRouteChunks> generatedThriftRoutes_;
};

} // namespace facebook::fboss::utility
