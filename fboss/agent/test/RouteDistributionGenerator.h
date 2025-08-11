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
    const facebook::fboss::RouterID& routerId,
    std::optional<typename IdT<AddrT>::type> offset = std::nullopt) {
  // Obtain a new prefix.
  auto prefix = offset.has_value() ? prefixGenerator.getNext(offset.value())
                                   : prefixGenerator.getNext();
  while (findRoute<AddrT>(routerId, {prefix.network(), prefix.mask()}, state)) {
    prefix = offset.has_value() ? prefixGenerator.getNext(offset.value())
                                : prefixGenerator.getNext();
  }
  return folly::CIDRNetwork{{prefix.network()}, prefix.mask()};
}

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
  template <typename AddrT>
  struct RouteDistribution {
    uint8_t masklen{};
    uint32_t numPrefixes{};
    std::optional<typename IdT<AddrT>::type> offset =
        std::nullopt; // offset used to generate prefix
  };
  using RouteChunk = std::vector<Route>;
  using RouteChunks = std::vector<RouteChunk>;
  using ThriftRouteChunk = std::vector<UnicastRoute>;
  using ThriftRouteChunks = std::vector<ThriftRouteChunk>;
  template <typename AddrT>
  using RouteDistributionSpec = std::vector<RouteDistribution<AddrT>>;

  RouteDistributionGenerator(
      const std::shared_ptr<SwitchState>& startingState,
      const RouteDistributionSpec<folly::IPAddressV6>& v6DistributionSpec,
      const RouteDistributionSpec<folly::IPAddressV4>& v4DistributionSpec,
      unsigned int chunkSize,
      unsigned int ecmpWidth,
      bool needL2EntryForNeighbor,
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

  bool needL2EntryForNeighbor() const {
    return needL2EntryForNeighbor_;
  }

 private:
  template <typename AddrT>
  const std::vector<UnresolvedNextHop>& getNhops() const;

  const std::shared_ptr<SwitchState> startingState_;
  const RouteDistributionSpec<folly::IPAddressV6> v6DistributionSpec_;
  const RouteDistributionSpec<folly::IPAddressV4> v4DistributionSpec_;
  const unsigned int chunkSize_;
  const unsigned int ecmpWidth_;
  const bool needL2EntryForNeighbor_;
  const RouterID routerId_{0};

 protected:
  template <typename AddT>
  void genRouteDistribution(
      const RouteDistributionSpec<AddT>& routeDistributionSpec) const;
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
