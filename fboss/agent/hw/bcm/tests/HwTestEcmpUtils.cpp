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

uint64_t getEcmpMemberWeight(
    const facebook::fboss::HwSwitch* /*hw*/,
    const std::multiset<uint64_t>& pathsInHw,
    uint64_t pathInHw) {
  std::set<uint64_t> uniquePaths(pathsInHw.begin(), pathsInHw.end());
  // This check assumes that egress ids grow as you add more egresses
  // That assumption could prove incorrect, in which case we would
  // need to map ips to egresses, somehow.
  auto iter = uniquePaths.find(pathInHw);
  if (iter == uniquePaths.end()) {
    throw FbossError("path not found");
  }
  return pathsInHw.count(pathInHw);
}

uint64_t getTotalEcmpMemberWeight(
    const facebook::fboss::HwSwitch* /*hw*/,
    const std::multiset<uint64_t>& pathsInHw) {
  return pathsInHw.size();
}
} // namespace facebook::fboss::utility
