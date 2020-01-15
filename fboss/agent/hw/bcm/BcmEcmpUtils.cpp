/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/hw/bcm/BcmEcmpUtils.h"

#include "fboss/agent/hw/bcm/BcmRoute.h"
#include "fboss/agent/hw/bcm/BcmSwitch.h"

#include <folly/IPAddress.h>

namespace facebook::fboss::utility {

std::multiset<bcm_if_t>
getEcmpGroupInHw(int unit, bcm_if_t ecmp, int sizeInSw) {
  std::multiset<bcm_if_t> ecmpGroup;
  bcm_l3_egress_ecmp_t existing;
  bcm_l3_egress_ecmp_t_init(&existing);
  existing.ecmp_intf = ecmp;
  bcm_if_t pathsInHw[sizeInSw];
  int pathsInHwCount;
  bcm_l3_egress_ecmp_get(unit, &existing, sizeInSw, pathsInHw, &pathsInHwCount);
  for (size_t i = 0; i < pathsInHwCount; ++i) {
    ecmpGroup.insert(pathsInHw[i]);
  }
  return ecmpGroup;
}

int getEcmpSizeInHw(int unit, bcm_if_t ecmp, int sizeInSw) {
  return getEcmpGroupInHw(unit, ecmp, sizeInSw).size();
}

bcm_if_t getEgressIdForRoute(
    const BcmSwitch* hw,
    const folly::IPAddress& ip,
    uint8_t mask,
    bcm_vrf_t vrf) {
  auto bcmRoute = hw->routeTable()->getBcmRoute(vrf, ip, mask);
  return bcmRoute->getEgressId();
}

} // namespace facebook::fboss::utility
