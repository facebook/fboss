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

#include <string>
#include <type_traits> // To use 'std::integral_constant'.

#include <boost/container/flat_map.hpp>
#include <boost/iterator/filter_iterator.hpp>

#include <folly/IPAddress.h>
#include <folly/IPAddressV4.h>
#include <folly/IPAddressV6.h>
#include <folly/MacAddress.h>
#include <folly/Range.h>
#include <folly/lang/Bits.h>

#include "fboss/agent/types.h"

#include <chrono>

namespace facebook::fboss {

/*
 * utility to normalize weights to a fixed total count
 */
template <typename T>
void normalizeNextHopWeightsToMaxPaths(
    std::map<T, uint64_t>& nhAndWeights,
    uint64_t normalizedPathCount) {
  uint64_t totalWeight = 0;
  for (const auto egressAndWeight : nhAndWeights) {
    totalWeight += egressAndWeight.second;
  }
  // compute the scale factor
  int factor = normalizedPathCount / totalWeight;
  uint64_t scaledTotalWeight = 0;
  std::map<uint64_t, uint32_t> WeightMap;

  for (auto& egressidAndWeight : nhAndWeights) {
    uint64_t w = std::max((int)egressidAndWeight.second * factor, 1);
    egressidAndWeight.second = w;
    WeightMap[w]++;
    scaledTotalWeight += w;
  }
  if (scaledTotalWeight < normalizedPathCount) {
    // compute the underflow weight and it's distribution
    int underflow = normalizedPathCount - scaledTotalWeight;

    std::map<int, int> underflowWeightAllocation;
    double underflowFactor = underflow / static_cast<double>(scaledTotalWeight);
    for (const auto& weightAndCount : WeightMap) {
      underflowWeightAllocation[weightAndCount.first] =
          weightAndCount.first * weightAndCount.second * underflowFactor;
    }

    // distribute the underflow evenly across the members in weight groups
    for (auto& egressidAndWeight : nhAndWeights) {
      auto weight = egressidAndWeight.second;
      if (underflow && underflowWeightAllocation[weight]) {
        // Each member gets total for weight group/number of members slots
        auto adjust = underflowWeightAllocation[weight] / WeightMap[weight]--;
        underflowWeightAllocation[weight] -= adjust;
        egressidAndWeight.second += adjust;
        underflow -= adjust;
      }
    }

    // distribute any remaining underflows to least weight nhops
    while (underflow) {
      auto minNhop = std::min_element(
          nhAndWeights.begin(),
          nhAndWeights.end(),
          [](const auto& nhop1, const auto& nhop2) {
            return nhop1.second < nhop2.second;
          });
      minNhop->second++;
      underflow--;
    }
  }
}

} // namespace facebook::fboss
