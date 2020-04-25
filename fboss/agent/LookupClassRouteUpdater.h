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

#include "fboss/agent/SwSwitch.h"
#include "fboss/agent/state/RouteNextHop.h"
#include "fboss/agent/state/StateDelta.h"

namespace facebook::fboss {

class LookupClassRouteUpdater {
 public:
  explicit LookupClassRouteUpdater(SwSwitch* sw) : sw_(sw) {}

  void processUpdates(const StateDelta& stateDelta);

  void initPort(
      const std::shared_ptr<SwitchState>& switchState,
      std::shared_ptr<Port> port);

  void neighborClassIDUpdated(
      const folly::IPAddress& ip,
      VlanID vlanID,
      std::optional<cfg::AclLookupClass> classID);

  void updateStateObserverLocalCache(
      const std::shared_ptr<SwitchState>& switchState);

 private:
  template <typename AddrT>
  void processUpdatesHelper(const StateDelta& stateDelta);

  template <typename RouteT>
  void processRouteAdded(
      const std::shared_ptr<SwitchState>& switchState,
      RouterID rid,
      const std::shared_ptr<RouteT>& addedRoute);
  template <typename RouteT>
  void processRouteRemoved(const std::shared_ptr<RouteT>& removedRoute);
  template <typename RouteT>
  void processRouteChanged(
      const std::shared_ptr<SwitchState>& switchState,
      RouterID rid,
      const std::shared_ptr<RouteT>& oldRoute,
      const std::shared_ptr<RouteT>& newRoute);

  void updateClassID(const std::map<
                     std::pair<RouterID, folly::CIDRNetwork>,
                     std::optional<cfg::AclLookupClass>>& ridAndCidrToClassID);

  template <typename AddrT>
  void updateClassIDHelper(
      RouterID rid,
      std::shared_ptr<SwitchState>& newState,
      const std::shared_ptr<RouteTable> routeTable,
      const RoutePrefix<AddrT>& routePrefix,
      std::optional<cfg::AclLookupClass> classID = std::nullopt);

  template <typename AddrT>
  void updateClassIDForPrefix(
      RouterID rid,
      const RoutePrefix<AddrT>& routePrefix,
      std::optional<cfg::AclLookupClass> classID);

  template <typename AddrT>
  void clearClassIDsForRoutes() const;

  SwSwitch* sw_;
};

} // namespace facebook::fboss
