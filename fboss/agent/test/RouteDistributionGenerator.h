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

#include "fboss/agent/types.h"

#include <map>
#include <memory>
#include <vector>

namespace facebook {
namespace fboss {
class SwitchState;
}
} // namespace facebook

namespace facebook {
namespace fboss {
namespace utility {

using Masklen2NumPrefixes = std::map<uint8_t, uint32_t>;

/*
 * RouteDistributionGenerator takes a input state, distribtion spc and
 * chunk size to generate a sequence of switch states that can be used
 * to program the required route distribution.
 * Chunk size controls the number of switch state updates that will
 * generated, with each update containing <= chunk size number of new
 * routes.
 */
class RouteDistributionGenerator {
 public:
  using SwitchStates = std::vector<std::shared_ptr<SwitchState>>;
  RouteDistributionGenerator(
      const std::shared_ptr<SwitchState>& startingState,
      const Masklen2NumPrefixes& v6DistributionSpec,
      const Masklen2NumPrefixes& v4DistributionSpec,
      unsigned int chunkSize,
      unsigned int ecmpWidth,
      RouterID routerId = RouterID(0));
  /*
   * Compute, cache and return route distribition
   */
  SwitchStates get();

 private:
  template <typename AddT>
  void genRouteDistribution(
      const Masklen2NumPrefixes& routeDistribution,
      int* trailingUpdateSize);

  std::shared_ptr<SwitchState> startingState_;
  Masklen2NumPrefixes v6DistributionSpec_;
  Masklen2NumPrefixes v4DistributionSpec_;
  unsigned int chunkSize_;
  unsigned int ecmpWidth_;
  RouterID routerId_{0};
  folly::Optional<SwitchStates> generatedStates_;
};
} // namespace utility
} // namespace fboss
} // namespace facebook
