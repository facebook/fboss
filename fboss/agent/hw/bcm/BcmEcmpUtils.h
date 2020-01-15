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

#include <set>

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

int getEcmpSizeInHw(int unit, bcm_if_t ecmp, int sizeInSw);
std::multiset<bcm_if_t> getEcmpGroupInHw(int unit, bcm_if_t ecmp, int sizeInSw);

bcm_if_t getEgressIdForRoute(
    const BcmSwitch* hw,
    const folly::IPAddress& ip,
    uint8_t mask,
    bcm_vrf_t vrf = 0);

} // namespace facebook::fboss::utility
