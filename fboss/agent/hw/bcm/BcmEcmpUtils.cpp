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
  if (hw->getPlatform()->getAsic()->isSupported(HwAsic::Feature::HSDK)) {
    // @lint-ignore CLANGTIDY
    bcm_l3_ecmp_member_t pathsInHw[sizeInSw];
    bcm_l3_ecmp_get(
        hw->getUnit(), &existing, sizeInSw, pathsInHw, &pathsInHwCount);
    for (size_t i = 0; i < pathsInHwCount; ++i) {
      if (existing.ecmp_group_flags == BCM_L3_ECMP_MEMBER_WEIGHTED) {
        for (size_t j = 0; j < pathsInHw[i].weight; j++) {
          ecmpGroup.insert(pathsInHw[i].egress_if);
        }
      } else {
        ecmpGroup.insert(pathsInHw[i].egress_if);
      }
    }
  } else {
    // @lint-ignore CLANGTIDY
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
std::pair<bcm_if_t, int> toIntfId(T egress) {
  return std::make_pair(egress, 1);
}

template <>
std::pair<bcm_if_t, int> toIntfId(bcm_l3_ecmp_member_t egress) {
  return std::make_pair(egress.egress_if, egress.weight);
}

template <typename T>
int bcm_l3_ecmp_traverse_cb(
    int /*unit*/,
    bcm_l3_egress_ecmp_t* ecmp,
    int memberCount,
    T* memberArray,
    void* userData) {
  if (!userData) {
    return 0;
  }
  auto* outvec =
      reinterpret_cast<std::pair<std::vector<bcm_if_t>*, bool>*>(userData)
          ->first;
  auto getMemberIds =
      reinterpret_cast<std::pair<std::vector<bcm_if_t>*, bool>*>(userData)
          ->second;
  if (getMemberIds) {
    for (auto i = 0; i < memberCount; i++) {
      if (ecmp->ecmp_group_flags == BCM_L3_ECMP_MEMBER_WEIGHTED) {
        auto egress = toIntfId<T>(memberArray[i]);
        for (auto j = 0; j < egress.second; j++) {
          outvec->push_back(egress.first);
        }
      } else {
        outvec->push_back(toIntfId<T>(memberArray[i]).first);
      }
    }
  } else {
    outvec->push_back(ecmp->ecmp_intf);
  }
  return 0;
}

std::vector<bcm_if_t> getEcmpMembersInHw(const BcmSwitch* hw) {
  std::vector<bcm_if_t> ecmpMembers;
  auto userData = std::make_pair(&ecmpMembers, true /* get ecmp member ids*/);
  if (hw->getPlatform()->getAsic()->isSupported(HwAsic::Feature::HSDK)) {
    bcm_l3_ecmp_traverse(
        hw->getUnit(),
        bcm_l3_ecmp_traverse_cb<bcm_l3_ecmp_member_t>,
        &userData);
  } else {
    bcm_l3_egress_ecmp_traverse(
        hw->getUnit(), bcm_l3_ecmp_traverse_cb<bcm_if_t>, &userData);
  }
  return ecmpMembers;
}

std::vector<bcm_if_t> getEcmpsInHw(const BcmSwitch* hw) {
  std::vector<bcm_if_t> ecmps;
  auto userData = std::make_pair(&ecmps, false /* get ecmp ids*/);
  if (hw->getPlatform()->getAsic()->isSupported(HwAsic::Feature::HSDK)) {
    bcm_l3_ecmp_traverse(
        hw->getUnit(),
        bcm_l3_ecmp_traverse_cb<bcm_l3_ecmp_member_t>,
        &userData);
  } else {
    bcm_l3_egress_ecmp_traverse(
        hw->getUnit(), bcm_l3_ecmp_traverse_cb<bcm_if_t>, &userData);
  }
  return ecmps;
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
