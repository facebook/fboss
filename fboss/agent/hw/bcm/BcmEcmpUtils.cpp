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
getEcmpGroupInHw(const BcmSwitch* hw, bcm_if_t ecmp, int sizeInSw) {
  std::multiset<bcm_if_t> ecmpGroup;
  bcm_l3_egress_ecmp_t existing;
  bcm_l3_egress_ecmp_t_init(&existing);
  existing.ecmp_intf = ecmp;
  int pathsInHwCount;
  if (hw->getPlatform()->getAsic()->isSupported(
          HwAsic::Feature::WEIGHTED_NEXTHOPGROUP_MEMBER)) {
#ifdef BCM_L3_ECMP_MEMBER_WEIGHTED
    // @lint-ignore HOWTOEVEN CArray
    bcm_l3_ecmp_member_t pathsInHw[sizeInSw];
    bcm_l3_ecmp_get(
        hw->getUnit(), &existing, sizeInSw, pathsInHw, &pathsInHwCount);
    for (size_t i = 0; i < pathsInHwCount; ++i) {
      ecmpGroup.insert(pathsInHw[i].egress_if);
    }
#endif
  } else {
    // @lint-ignore HOWTOEVEN CArray
    bcm_if_t pathsInHw[sizeInSw];
    bcm_l3_egress_ecmp_get(
        hw->getUnit(), &existing, sizeInSw, pathsInHw, &pathsInHwCount);
    for (size_t i = 0; i < pathsInHwCount; ++i) {
      ecmpGroup.insert(pathsInHw[i]);
    }
  }
  return ecmpGroup;
}

int getEcmpSizeInHw(const BcmSwitch* hw, bcm_if_t ecmp, int sizeInSw) {
  return getEcmpGroupInHw(hw, ecmp, sizeInSw).size();
}

template <typename T>
bcm_if_t toIntfId(T egress) {
  return egress;
}

#ifdef BCM_L3_ECMP_MEMBER_WEIGHTED
template <>
bcm_if_t toIntfId(bcm_l3_ecmp_member_t egress) {
  return egress.egress_if;
}
#endif

template <typename T>
int bcm_l3_ecmp_traverse_cb(
    int /*unit*/,
    bcm_l3_egress_ecmp_t* /*ecmp*/,
    int memberCount,
    T* memberArray,
    void* userData) {
  if (!userData) {
    return 0;
  }
  auto* outvec = reinterpret_cast<std::vector<bcm_if_t>*>(userData);
  for (auto i = 0; i < memberCount; i++) {
    outvec->push_back(toIntfId<T>(memberArray[i]));
  }
  return 0;
}

std::vector<bcm_if_t> getEcmpMembersInHw(const BcmSwitch* hw) {
  std::vector<bcm_if_t> ecmpMembers;
  if (hw->getPlatform()->getAsic()->isSupported(
          HwAsic::Feature::WEIGHTED_NEXTHOPGROUP_MEMBER)) {
#ifdef BCM_L3_ECMP_MEMBER_WEIGHTED
    bcm_l3_ecmp_traverse(
        hw->getUnit(),
        bcm_l3_ecmp_traverse_cb<bcm_l3_ecmp_member_t>,
        &ecmpMembers);
#endif
  } else {
    bcm_l3_egress_ecmp_traverse(
        hw->getUnit(), bcm_l3_ecmp_traverse_cb<bcm_if_t>, &ecmpMembers);
  }
  return ecmpMembers;
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
