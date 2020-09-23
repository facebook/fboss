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

#include "fboss/agent/FbossError.h"
#include "fboss/agent/hw/sai/api/NextHopGroupApi.h"
#include "fboss/agent/hw/sai/api/RouteApi.h"
#include "fboss/agent/hw/sai/switch/SaiNextHopGroupManager.h"
#include "fboss/agent/hw/sai/switch/SaiRouteManager.h"
#include "fboss/agent/hw/sai/switch/SaiSwitch.h"
#include "fboss/agent/hw/sai/switch/SaiVirtualRouterManager.h"
#include "fboss/agent/hw/switch_asics/HwAsic.h"
#include "fboss/agent/types.h"

namespace facebook::fboss::utility {

std::multiset<uint64_t> getEcmpMembersInHw(
    const facebook::fboss::HwSwitch* hw,
    const folly::CIDRNetwork& prefix,
    facebook::fboss::RouterID rid,
    int sizeInSw) {
  const auto saiSwitch = static_cast<const SaiSwitch*>(hw);
  const auto& virtualRouterHandle =
      saiSwitch->managerTable()->virtualRouterManager().getVirtualRouterHandle(
          rid);
  if (!virtualRouterHandle) {
    throw FbossError("No virtual router with for rid: ", rid);
  }
  SaiRouteTraits::RouteEntry r(
      saiSwitch->getSwitchId(),
      virtualRouterHandle->virtualRouter->adapterKey(),
      prefix);
  const auto& routeHandle =
      saiSwitch->managerTable()->routeManager().getRouteHandle(r);
  if (!routeHandle) {
    throw FbossError(
        "No route found for: ",
        prefix.first,
        "/",
        static_cast<int>(prefix.second));
  }
  auto handle = routeHandle->nextHopGroupHandle();
  if (!handle) {
    throw FbossError(
        "No next hop group found for: ",
        prefix.first,
        "/",
        static_cast<int>(prefix.second));
  }
  auto memberList = SaiApiTable::getInstance()->nextHopGroupApi().getAttribute(
      handle->nextHopGroup->adapterKey(),
      SaiNextHopGroupTraits::Attributes::NextHopMemberList());
  return std::multiset<uint64_t>(memberList.begin(), memberList.end());
}

uint64_t getEcmpMemberWeight(
    const facebook::fboss::HwSwitch* hw,
    const std::multiset<uint64_t>& pathsInHw,
    uint64_t pathInHw) {
  auto asic = hw->getPlatform()->getAsic();
  if (asic->isSupported(HwAsic::Feature::WEIGHTED_NEXTHOPGROUP_MEMBER)) {
    return SaiApiTable::getInstance()->nextHopGroupApi().getAttribute(
        static_cast<NextHopGroupMemberSaiId>(pathInHw),
        SaiNextHopGroupMemberTraits::Attributes::Weight{});
  }
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
    const facebook::fboss::HwSwitch* hw,
    const std::multiset<uint64_t>& pathsInHw) {
  std::set<uint64_t> uniquePaths(pathsInHw.begin(), pathsInHw.end());
  uint64_t totalWeight = 0;
  for (auto pathInHw : uniquePaths) {
    totalWeight += getEcmpMemberWeight(hw, pathsInHw, pathInHw);
  }
  return totalWeight;
}
} // namespace facebook::fboss::utility
