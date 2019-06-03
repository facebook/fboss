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

#include "fboss/agent/hw/sai/api/NeighborApi.h"
#include "fboss/agent/hw/sai/api/RouteApi.h"
#include "fboss/agent/hw/sai/api/SaiApiTable.h"
#include "fboss/agent/state/StateDelta.h"
#include "fboss/agent/types.h"

#include <memory>
#include <unordered_map>

namespace facebook {
namespace fboss {

class SaiManagerTable;
class SaiNextHopGroup;
class SaiPlatform;

class SaiRoute {
 public:
  SaiRoute(
      SaiApiTable* apiTable,
      SaiManagerTable* managerTable,
      const RouteApiParameters::EntryType& entry,
      const RouteApiParameters::Attributes& attributes,
      std::shared_ptr<SaiNextHopGroup> nextHopGroup);
  ~SaiRoute();
  SaiRoute(const SaiRoute& other) = delete;
  SaiRoute(SaiRoute&& other) = delete;
  SaiRoute& operator=(const SaiRoute& other) = delete;
  SaiRoute& operator=(SaiRoute&& other) = delete;
  bool operator==(const SaiRoute& other) const;
  bool operator!=(const SaiRoute& other) const;

  void setAttributes(const RouteApiParameters::Attributes& desiredAttributes);
  const RouteApiParameters::Attributes attributes() const {
    return attributes_;
  }

 private:
  SaiApiTable* apiTable_;
  SaiManagerTable* managerTable_;
  RouteApiParameters::EntryType entry_;
  RouteApiParameters::Attributes attributes_;
  std::shared_ptr<SaiNextHopGroup> nextHopGroup_;
};

class SaiRouteManager {
 public:
  SaiRouteManager(
      SaiApiTable* apiTable,
      SaiManagerTable* managerTable,
      const SaiPlatform* platform);
  // Helper function to create a SAI RouteEntry from an FBOSS SwitchState
  // Route (e.g., Route<IPAddressV6>)
  template <typename AddrT>
  RouteApiParameters::EntryType routeEntryFromSwRoute(
      RouterID routerId,
      const std::shared_ptr<Route<AddrT>>& swEntry) const;

  template <typename AddrT>
  void changeRoute(
      RouterID routerId,
      const std::shared_ptr<Route<AddrT>>& oldSwRoute,
      const std::shared_ptr<Route<AddrT>>& newSwRoute);

  std::vector<std::unique_ptr<SaiRoute>> makeInterfaceToMeRoutes(
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

  SaiRoute* getRoute(const RouteApiParameters::EntryType& entry);
  const SaiRoute* getRoute(const RouteApiParameters::EntryType& entry) const;

  void clear();

 private:
  SaiRoute* getRouteImpl(const RouteApiParameters::EntryType& entry) const;

  SaiApiTable* apiTable_;
  SaiManagerTable* managerTable_;
  const SaiPlatform* platform_;
  std::unordered_map<RouteApiParameters::EntryType, std::unique_ptr<SaiRoute>>
      routes_;
};

} // namespace fboss
} // namespace facebook
