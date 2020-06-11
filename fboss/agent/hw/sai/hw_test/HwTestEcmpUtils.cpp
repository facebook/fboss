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
} // namespace facebook::fboss::utility
