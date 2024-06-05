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

#include <gtest/gtest.h>
#include "fboss/agent/SwSwitch.h"
#include "fboss/agent/types.h"

DECLARE_int32(ecmp_resource_percentage);

namespace facebook::fboss {
class HwAsic;
class SwSwitch;

namespace utility {

std::vector<std::vector<PortDescriptor>> generateEcmpGroupScale(
    const std::vector<PortDescriptor>& inputs,
    const int maxEcmpGroups);

std::vector<std::vector<PortDescriptor>> generateEcmpMemberScale(
    const std::vector<PortDescriptor>& inputs,
    const int maxEcmpMembers);

std::vector<std::vector<PortDescriptor>> getUcmpMembersAndWeight(
    const std::vector<std::vector<PortDescriptor>>& inputs,
    std::vector<std::vector<NextHopWeight>>& weightsOutput,
    const int maxEcmpMembers);

void assignUcmpWeights(
    const std::vector<std::vector<PortDescriptor>>& inputs,
    std::vector<std::vector<NextHopWeight>>& weightsOutput);

uint32_t getMaxDlbEcmpGroups(const std::vector<const HwAsic*>& asics);
uint32_t getMaxEcmpGroups(const std::vector<const HwAsic*>& asics);
uint32_t getMaxEcmpMembers(const std::vector<const HwAsic*>& asics);
uint32_t getMaxUcmpMembers(const std::vector<const HwAsic*>& asics);
} // namespace utility
} // namespace facebook::fboss
