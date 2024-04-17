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

#include "fboss/agent/Utils.h"

#include <cstdint>
#include <set>
#include <vector>

extern "C" {
#include <bcm/l3.h>
}

namespace folly {
class IPAddress;
}

namespace facebook::fboss {
class BcmSwitch;

} // namespace facebook::fboss

namespace facebook::fboss::utility {

int getEcmpSizeInHw(const BcmSwitch* hw, bcm_if_t ecmp, int sizeInSw);
int getFlowletSizeWithScalingFactor(
    const BcmSwitch* hw,
    const int flowSetTableSize,
    const int numPaths,
    const int maxPaths);
std::multiset<bcm_if_t>
getEcmpGroupInHw(const BcmSwitch* hw, bcm_if_t ecmp, int sizeInSw);

std::vector<bcm_if_t> getEcmpMembersInHw(const BcmSwitch* hw);
std::vector<bcm_if_t> getEcmpsInHw(const BcmSwitch* hw);

bcm_if_t getEgressIdForRoute(
    const BcmSwitch* hw,
    const folly::IPAddress& ip,
    uint8_t mask,
    bcm_vrf_t vrf = 0);

bool isNativeUcmpEnabled(const BcmSwitch* hw, bcm_if_t ecmp);

void setEcmpDynamicMemberUp(const BcmSwitch* hw);

uint32 getFlowletDynamicMode(const cfg::SwitchingMode& switchingMode);

} // namespace facebook::fboss::utility
