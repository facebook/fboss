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

#include "fboss/agent/LookupClassNextHopsAndRoutesManager.h"

#include "fboss/agent/state/Port.h"
#include "fboss/agent/state/RouteNextHop.h"
#include "fboss/agent/state/SwitchState.h"

namespace facebook::fboss {

class LookupClassNextHopsAndRoutesManager {
 public:
  template <typename RouteT>
  std::optional<cfg::AclLookupClass> processRouteAdded(
      const std::shared_ptr<SwitchState>& switchState,
      RouterID rid,
      const std::shared_ptr<RouteT>& addedRoute);

  template <typename RouteT>
  void processRouteRemoved(
      const std::shared_ptr<SwitchState>& switchState,
      RouterID rid,
      const std::shared_ptr<RouteT>& removedRoute);

  template <typename RouteT>
  std::optional<cfg::AclLookupClass> processRouteChanged(
      const std::shared_ptr<SwitchState>& switchState,
      RouterID rid,
      const std::shared_ptr<RouteT>& oldRoute,
      const std::shared_ptr<RouteT>& newRoute);

  std::map<
      std::pair<RouterID, folly::CIDRNetwork>,
      std::optional<cfg::AclLookupClass>>
  neighborClassIDUpdated(
      const folly::IPAddress& ip,
      VlanID vlanID,
      std::optional<cfg::AclLookupClass> classID);

  void initPort(
      const std::shared_ptr<SwitchState>& switchState,
      std::shared_ptr<Port> port);

  void updateStateObserverLocalCache(
      const std::shared_ptr<RouteTable> routeTable);
};

} // namespace facebook::fboss
