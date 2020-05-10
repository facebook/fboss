/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/test/HwTestRouteUtils.h"

#include "fboss/agent/FbossError.h"
#include "fboss/agent/hw/sai/api/RouteApi.h"
#include "fboss/agent/hw/sai/api/SaiApiTable.h"
#include "fboss/agent/hw/sai/switch/SaiManagerTable.h"
#include "fboss/agent/hw/sai/switch/SaiSwitch.h"
#include "fboss/agent/hw/sai/switch/SaiVirtualRouterManager.h"
#include "fboss/agent/types.h"

namespace facebook::fboss::utility {

std::optional<cfg::AclLookupClass> getHwRouteClassID(
    const HwSwitch* hw,
    RouterID rid,
    const folly::CIDRNetwork& prefix) {
  const auto saiSwitch = static_cast<const SaiSwitch*>(hw);
  auto virtualRouterHandle =
      saiSwitch->managerTable()->virtualRouterManager().getVirtualRouterHandle(
          rid);
  if (!virtualRouterHandle) {
    throw FbossError("No virtual router with id 0");
  }
  SaiRouteTraits::RouteEntry r(
      saiSwitch->getSwitchId(),
      virtualRouterHandle->virtualRouter->adapterKey(),
      prefix);
  auto metadata = SaiApiTable::getInstance()->routeApi().getAttribute(
      r, SaiRouteTraits::Attributes::Metadata());
  return metadata == 0 ? std::nullopt
                       : std::optional(cfg::AclLookupClass(metadata));
}

} // namespace facebook::fboss::utility
