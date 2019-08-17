/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/test/RouteScaleGenerators.h"
namespace facebook {
namespace fboss {
namespace utility {

FSWRouteScaleGenerator::FSWRouteScaleGenerator(
    const std::shared_ptr<SwitchState>& startingState,
    unsigned int chunkSize,
    unsigned int ecmpWidth)
    : routeDistributionGen_(
          startingState,
          // v6 distribution
          {
              {48, 100},
              {52, 200},
              {56, 100},
              {64, 3550},
              {80, 300},
              {96, 200},
              {112, 100},
              {127, 100},
              {128, 3350},
          },
          // v4 distribution
          {
              {15, 200},
              {24, 2000},
              {26, 1000},
              {28, 200},
              {31, 100},
              {32, 4500},
          },
          chunkSize,
          ecmpWidth) {}

THAlpmRouteScaleGenerator::THAlpmRouteScaleGenerator(
    const std::shared_ptr<SwitchState>& startingState,
    unsigned int chunkSize,
    unsigned int ecmpWidth)
    : routeDistributionGen_(
          startingState,
          // v6 distribution
          {
              {48, 200},
              {52, 200},
              {56, 200},
              {64, 10000},
              {80, 200},
              {96, 200},
              {112, 200},
              {120, 200},
              {128, 10000},
          },
          // v4 distribution
          {
              {15, 400},
              {24, 400},
              {26, 400},
              {28, 400},
              {30, 400},
              {32, 10000},
          },
          chunkSize,
          ecmpWidth) {}

} // namespace utility
} // namespace fboss
} // namespace facebook
