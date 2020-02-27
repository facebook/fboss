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

#include "fboss/agent/hw/sai/api/NextHopGroupApi.h"
#include "fboss/agent/hw/sai/api/RouteApi.h"
#include "fboss/agent/hw/sai/store/SaiObject.h"
#include "fboss/agent/state/StateDelta.h"
#include "fboss/agent/types.h"

#include "folly/container/F14Map.h"

#include <memory>

namespace facebook::fboss {

class SaiManagerTable;
class SaiPlatform;
class SaiNextHopGroupHandle;

using SaiRoute = SaiObject<SaiRouteTraits>;

struct SaiRouteHandle {
  std::shared_ptr<SaiRoute> route;
  std::shared_ptr<SaiNextHopGroupHandle> nextHopGroupHandle;
};

class SaiRouteManager {
 public:
  SaiRouteManager(SaiManagerTable* managerTable, const SaiPlatform* platform);
  // Helper function to create a SAI RouteEntry from an FBOSS SwitchState
  // Route (e.g., Route<IPAddressV6>)
  template <typename AddrT>
  SaiRouteTraits::RouteEntry routeEntryFromSwRoute(
      RouterID routerId,
      const std::shared_ptr<Route<AddrT>>& swEntry) const;

  template <typename AddrT>
  void changeRoute(
      RouterID routerId,
      const std::shared_ptr<Route<AddrT>>& oldSwRoute,
      const std::shared_ptr<Route<AddrT>>& newSwRoute);

  std::vector<std::shared_ptr<SaiRoute>> makeInterfaceToMeRoutes(
      const std::shared_ptr<Interface>& swInterface);

  template <typename AddrT>
  void addRoute(
      RouterID routerId,
      const std::shared_ptr<Route<AddrT>>& swRoute);

  template <typename AddrT>
  void removeRoute(
      RouterID routerId,
      const std::shared_ptr<Route<AddrT>>& swRoute);

  void processRouteDelta(const StateDelta& delta);

  SaiRouteHandle* getRouteHandle(const SaiRouteTraits::RouteEntry& entry);
  const SaiRouteHandle* getRouteHandle(
      const SaiRouteTraits::RouteEntry& entry) const;

  void clear();

 private:
  SaiRouteHandle* getRouteHandleImpl(
      const SaiRouteTraits::RouteEntry& entry) const;
  template <typename AddrT>
  void addOrUpdateRoute(
      SaiRouteHandle* routeHandle,
      RouterID routerId,
      const std::shared_ptr<Route<AddrT>>& swRoute);

  template <typename AddrT>
  bool validRoute(const std::shared_ptr<Route<AddrT>>& swRoute);

  SaiManagerTable* managerTable_;
  const SaiPlatform* platform_;
  folly::F14FastMap<SaiRouteTraits::RouteEntry, std::unique_ptr<SaiRouteHandle>>
      handles_;
};

} // namespace facebook::fboss
