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

#include <glog/logging.h>
#include "fboss/agent/types.h"

#include <chrono>

namespace facebook::fboss {

/*
 * utility to normalize weights to a fixed total count
 */

// Modify ucmp weight distribution such that total weight is equal to
// a fixed value (normalizedPathCount). This is needed to support
// wide ucmp on TH3 where the total member count in ECMP structure
// programmed in hardware has be a power of 2. A high level description
// of normalization steps with an example walk through is included below.
//
// Consider a UCMP with following weight distribution
//      Number of nexthops      Weight
//          20                    8
//          10                    5
//          15                    7
//          10                    2
//
// 1. Compute a scale factor for weight so that weights can be adjusted
//    to a number close to normalized count.
//         factor = normalized count / total weight
//    In this example factor = 512/335 = 1.528
// 2. Scale weights by the factor computed
//    In the example, the scaled weights are
//      Number of nexthops      Weight
//          20                   12
//          10                    7
//          15                   10
//          10                    3
//    total scaled weight = 490
// 3. Compute underflow = normalized count - total weight.
//    In this example underflow = 512 - 490 = 22
// 4. Allocate the underflow slots to the highest weight next
//    hop sets that can accommodate the slots. This is done
//    with the intention of reducing the percent traffic load
//    difference between members of same weight group to minimum.
//    In this example, the largest 2 weights (12 and 10) has total
//    20 + 15 = 35 nexthops. Allocate 22 remaining slots among
//    these 2 weight groups by filling the lowest weight group first.
//    Weight 10 will get 15 slots (weight becomes 11 now) and
//    weight 12 will get 7 nexthops (split to 2 groups of weight 12 and 13)
//      Number of nexthops      Weight
//           7                   13
//          13                   12
//          10                    7
//          15                   11
//          10                    3
// Total count = 512

template <typename T>
void normalizeNextHopWeightsToMaxPaths(
    std::map<T, uint64_t>& nhAndWeights,
    uint64_t normalizedPathCount) {
  uint64_t totalWeight = 0;
  for (const auto egressAndWeight : nhAndWeights) {
    totalWeight += egressAndWeight.second;
  }
  // compute the scale factor
  double factor = normalizedPathCount / static_cast<double>(totalWeight);
  uint64_t scaledTotalWeight = 0;
  std::map<uint64_t, uint32_t> WeightMap;

  for (auto& egressidAndWeight : nhAndWeights) {
    uint64_t w = std::max(int(egressidAndWeight.second * factor), 1);
    egressidAndWeight.second = w;
    WeightMap[w]++;
    scaledTotalWeight += w;
  }
  if (scaledTotalWeight < normalizedPathCount) {
    int underflow = normalizedPathCount - scaledTotalWeight;

    std::map<int, int> underflowWeightAllocation;
    // distribute any remaining underflows to most weighted nhops
    int underflowAllocated = 0;
    // check from highest weight to lowest
    for (auto it = WeightMap.rbegin(); it != WeightMap.rend(); it++) {
      underflowAllocated += it->second;
      underflowWeightAllocation[it->first] = it->second;
      // stop when we find enough nexthops to absord underflow
      if (underflowAllocated >= underflow) {
        break;
      }
    }

    uint32_t overAllocation = underflowAllocated - underflow;
    // Shift uneven allocation to the highest weight groups
    // so that percentage error between members of same group is minimum.
    for (auto it = WeightMap.rbegin(); it != WeightMap.rend(); it++) {
      int delta = std::min(overAllocation, it->second);
      underflowWeightAllocation[it->first] -= delta;
      overAllocation -= delta;
      if (!overAllocation) {
        break;
      }
    }

    // distribute the underflow evenly across the members in weight groups
    for (auto& egressidAndWeight : nhAndWeights) {
      auto weight = egressidAndWeight.second;
      if (underflow && underflowWeightAllocation[weight]) {
        underflowWeightAllocation[egressidAndWeight.second]--;
        egressidAndWeight.second++;
        underflow--;
      }
    }
    // we should allocate all underflow slots
    CHECK_EQ(underflow, 0);
  }
}

} // namespace facebook::fboss
