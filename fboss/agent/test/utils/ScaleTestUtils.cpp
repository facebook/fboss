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
#include "fboss/agent/test/utils/AsicUtils.h"

namespace facebook::fboss::utility {

uint32_t getMaxDlbEcmpGroups(const std::vector<const HwAsic*>& asics) {
  auto asic = checkSameAndGetAsic(asics);
  auto maxDlbGroups = asic->getMaxDlbEcmpGroups();
  CHECK(maxDlbGroups.has_value());
  return maxDlbGroups.value();
}

uint32_t getMaxEcmpGroups(const std::vector<const HwAsic*>& asics) {
  auto asic = checkSameAndGetAsic(asics);
  auto maxEcmpGroups = asic->getMaxEcmpGroups();
  CHECK(maxEcmpGroups.has_value());
  return maxEcmpGroups.value();
}
uint32_t getMaxEcmpMembers(const std::vector<const HwAsic*>& asics) {
  auto asic = checkSameAndGetAsic(asics);
  auto maxEcmpMembers = asic->getMaxEcmpMembers();
  CHECK(maxEcmpMembers.has_value());
  return maxEcmpMembers.value();
}
uint32_t getMaxUcmpMembers(const std::vector<const HwAsic*>& asics) {
  auto asic = checkSameAndGetAsic(asics);
  auto maxUcmpMembers = asic->getMaxEcmpMembers();
  CHECK(maxUcmpMembers.has_value());
  if (asic->getAsicType() == cfg::AsicType::ASIC_TYPE_TOMAHAWK4) {
    return maxUcmpMembers.value() / 4;
  }
  return maxUcmpMembers.value();
}
constexpr auto oddUcmpWeight = 3;
constexpr auto evenUcmpWeight = 2;

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

// Create weightsOutputs where sum of weights {2,3,2,3,2,3} = maxEcmpMembers
// and return the corresponding ecmp members which are subset of inputs
std::vector<std::vector<PortDescriptor>> getUcmpMembersAndWeight(
    const std::vector<std::vector<PortDescriptor>>& inputs,
    std::vector<std::vector<NextHopWeight>>& weightsOutput,
    const int maxEcmpMembers) {
  int runningWeight = 0;
  std::vector<std::vector<PortDescriptor>> output;
  for (int i = 0; i < inputs.size(); i++) {
    std::vector<NextHopWeight> weightsTemp;
    std::vector<PortDescriptor> outputTemp;
    for (int j = 0; j < inputs[i].size(); j++) {
      // Assign weights 3 and 2 to ECMP members.
      int currWeight = (j % 2) ? oddUcmpWeight : evenUcmpWeight;
      if (runningWeight + currWeight > maxEcmpMembers) {
        currWeight = 1;
      }
      runningWeight += currWeight;
      weightsTemp.push_back(currWeight);
      outputTemp.push_back(inputs[i][j]);
      if (runningWeight == maxEcmpMembers) {
        weightsOutput.push_back(std::move(weightsTemp));
        output.push_back(std::move(outputTemp));
        return output;
      }
    }
    weightsOutput.push_back(std::move(weightsTemp));
    output.push_back(std::move(outputTemp));
  }
  return output;
}

// Create weightsOutputs {2,3,2,3,2,3} for all the inputs
// Currently used for TH4
void assignUcmpWeights(
    const std::vector<std::vector<PortDescriptor>>& inputs,
    std::vector<std::vector<NextHopWeight>>& weightsOutput) {
  for (int i = 0; i < inputs.size(); i++) {
    std::vector<NextHopWeight> temp;
    for (int j = 0; j < inputs[i].size(); j++) {
      int num = (j % 2) ? oddUcmpWeight : evenUcmpWeight;
      temp.push_back(num);
    }
    weightsOutput.push_back(std::move(temp));
  }
}
} // namespace facebook::fboss::utility
