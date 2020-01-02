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
#include <opennsl/l3.h>
}

namespace folly {
class IPAddress;
}
namespace facebook::fboss {

class BcmSwitch;

} // namespace facebook::fboss
namespace facebook::fboss {

namespace utility {

int getEcmpSizeInHw(int unit, opennsl_if_t ecmp, int sizeInSw);
std::multiset<opennsl_if_t>
getEcmpGroupInHw(int unit, opennsl_if_t ecmp, int sizeInSw);

opennsl_if_t getEgressIdForRoute(
    const BcmSwitch* hw,
    const folly::IPAddress& ip,
    uint8_t mask,
    opennsl_vrf_t vrf = 0);
} // namespace utility

} // namespace facebook::fboss
