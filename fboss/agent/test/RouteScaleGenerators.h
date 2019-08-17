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

#include "fboss/agent/test/RouteDistributionGenerator.h"

namespace facebook {
namespace fboss {
class SwitchState;
}
} // namespace facebook

namespace facebook {
namespace fboss {
namespace utility {

constexpr unsigned int kDefaultChunkSize = 4000;
constexpr unsigned int kDefaulEcmpWidth = 4;

/*
 * Each of the following generators take a input state and chunk size
 * to generate a sequence of switch states that can be used to program
 * the required route distribution.
 */
class FSWRouteScaleGenerator {
 public:
  explicit FSWRouteScaleGenerator(
      const std::shared_ptr<SwitchState>& startingState,
      unsigned int chunkSize = kDefaultChunkSize,
      unsigned int ecmpWidth = kDefaulEcmpWidth);
  std::vector<std::shared_ptr<SwitchState>> get() {
    return routeDistributionGen_.get();
  }

 private:
  RouteDistributionGenerator routeDistributionGen_;
};

class THAlpmRouteScaleGenerator {
 public:
  explicit THAlpmRouteScaleGenerator(
      const std::shared_ptr<SwitchState>& startingState,
      unsigned int chunkSize = kDefaultChunkSize,
      unsigned int ecmpWidth = kDefaulEcmpWidth);
  std::vector<std::shared_ptr<SwitchState>> get() {
    return routeDistributionGen_.get();
  }

 private:
  RouteDistributionGenerator routeDistributionGen_;
};

} // namespace utility
} // namespace fboss
} // namespace facebook
