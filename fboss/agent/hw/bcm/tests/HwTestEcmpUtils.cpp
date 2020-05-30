/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/test/HwTestEcmpUtils.h"

#include "fboss/agent/hw/bcm/BcmEcmpUtils.h"
#include "fboss/agent/hw/bcm/BcmSwitch.h"

namespace facebook::fboss::utility {

std::multiset<uint64_t> getEcmpMembersInHw(
    const facebook::fboss::HwSwitch* hw,
    const folly::CIDRNetwork& prefix,
    facebook::fboss::RouterID rid,
    int sizeInSw) {
  auto bcmSw = static_cast<const BcmSwitch*>(hw);
  auto ecmp = getEgressIdForRoute(bcmSw, prefix.first, prefix.second, rid);
  auto members = utility::getEcmpGroupInHw(bcmSw->getUnit(), ecmp, sizeInSw);
  return std::multiset<uint64_t>(members.begin(), members.end());
}
} // namespace facebook::fboss::utility
