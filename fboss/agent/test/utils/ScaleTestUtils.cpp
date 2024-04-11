/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/test/utils/ScaleTestUtils.h"
#include "fboss/agent/hw/switch_asics/HwAsic.h"

namespace facebook::fboss::utility {

uint32_t getMaxEcmpGroups(const HwAsic* asic) {
  auto maxEcmpGroups = asic->getMaxEcmpGroups();
  CHECK(maxEcmpGroups.has_value());
  return maxEcmpGroups.value();
}
uint32_t getMaxEcmpMembers(const HwAsic* asic) {
  auto maxEcmpMembers = asic->getMaxEcmpMembers();
  CHECK(maxEcmpMembers.has_value());
  return maxEcmpMembers.value();
}

// Generate all possible combinations of k selections of the input
// vector.
// taken from fbcode/axon/common/coro_util.h
std::vector<std::vector<PortDescriptor>> genCombinations(
    const std::vector<PortDescriptor>& inputs,
    size_t k) {
  size_t n = inputs.size();
  std::vector<std::vector<PortDescriptor>> output;
  std::vector<bool> picked(n);
  std::fill(picked.begin(), picked.begin() + k, true);
  do {
    std::vector<PortDescriptor> currentCombination;
    for (size_t idx = 0; idx < n; idx++) {
      if (picked[idx]) {
        currentCombination.push_back(inputs[idx]);
      }
    }
    output.push_back(currentCombination);
  } while (std::prev_permutation(picked.begin(), picked.end()));

  return output;
}

// Generate all possible combinations of k selections of the input starting from
// minGroupSize to inputs.size()
std::vector<std::vector<PortDescriptor>> generateEcmpGroupScale(
    const std::vector<PortDescriptor>& inputs,
    const int maxEcmpGroups) {
  const int minGroupSize = 2;
  int groupsGenerated = 0;
  std::vector<std::vector<PortDescriptor>> currCombination;
  std::vector<std::vector<PortDescriptor>> allCombinations;
  for (int i = minGroupSize; i <= inputs.size(); i++) {
    currCombination = genCombinations(inputs, i);
    if ((groupsGenerated + currCombination.size()) >= maxEcmpGroups) {
      int remainingGrp = maxEcmpGroups - groupsGenerated;
      allCombinations.insert(
          allCombinations.end(),
          currCombination.begin(),
          currCombination.begin() + remainingGrp);
      break;
    }
    groupsGenerated += currCombination.size();
    allCombinations.insert(
        allCombinations.end(), currCombination.begin(), currCombination.end());
  }
  EXPECT_EQ(allCombinations.size(), maxEcmpGroups);
  return allCombinations;
}

// Generate all possible combinations of k selections of the input starting from
// inputs.size() to minGroupSize
std::vector<std::vector<PortDescriptor>> generateEcmpMemberScale(
    const std::vector<PortDescriptor>& inputs,
    const int maxEcmpMembers) {
  const int minGroupSize = 2;
  int membersGenerated = 0;
  std::vector<std::vector<PortDescriptor>> currCombination;
  std::vector<std::vector<PortDescriptor>> allCombinations;
  for (int i = inputs.size(); i >= minGroupSize; i--) {
    currCombination = genCombinations(inputs, i);
    // Check if after adding currCombination we would hit maxEcmpMembers
    if ((membersGenerated + currCombination.size() * i) >= maxEcmpMembers) {
      int remainingMem = maxEcmpMembers - membersGenerated;
      int remainingGrp = remainingMem / i;
      allCombinations.insert(
          allCombinations.end(),
          currCombination.begin(),
          currCombination.begin() + remainingGrp);
      if (remainingMem % i > 0) {
        allCombinations.push_back(std::vector<PortDescriptor>(
            inputs.begin(), inputs.begin() + (remainingMem % i)));
      }
      membersGenerated += remainingMem;
      break;
    }
    allCombinations.insert(
        allCombinations.end(), currCombination.begin(), currCombination.end());
    membersGenerated += currCombination.size() * i;
  }
  EXPECT_EQ(membersGenerated, maxEcmpMembers);
  return allCombinations;
}

} // namespace facebook::fboss::utility
