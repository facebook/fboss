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

#include "fboss/agent/hw/sai/store/SaiObjectEventSubscriber.h"

#include <memory>
#include <mutex>

DECLARE_bool(disable_valid_route_check);

namespace facebook::fboss {

class SaiManagerTable;
class SaiPlatform;
class SaiNextHopGroupHandle;
class SaiStore;
class SaiRouteManager;
struct SaiCounterHandle;

using SaiRoute = SaiObject<SaiRouteTraits>;

template <typename T>
class ManagedNextHop;

template <typename NextHopTraitsT>
class ManagedRouteNextHop
    : public detail::SaiObjectEventSubscriber<NextHopTraitsT> {
 public:
  using PublisherObject = std::shared_ptr<const SaiObject<NextHopTraitsT>>;
  ManagedRouteNextHop(
      PortSaiId cpuPort,
      SaiRouteManager* routeManager,
      SaiRouteTraits::AdapterHostKey routeKey,
      std::shared_ptr<ManagedNextHop<NextHopTraitsT>> managedNextHop,
      bool routeMetadataSupported);
  void afterCreate(PublisherObject nexthop) override;
  void beforeRemove() override;
  void linkDown() override {}
  sai_object_id_t adapterKey() const;
  using detail::SaiObjectEventSubscriber<NextHopTraitsT>::isReady;
  typename NextHopTraitsT::AdapterHostKey adapterHostKey() const;

 private:
  void updateMetadata(
      SaiRouteTraits::Attributes::Metadata currentMetadata) const;

  PortSaiId cpuPort_;
  SaiRouteManager* routeManager_;
  typename SaiRouteTraits::AdapterHostKey routeKey_;
  std::shared_ptr<ManagedNextHop<NextHopTraitsT>> managedNextHop_;
  bool routeMetadataSupported_;
};

using ManagedRouteIpNextHop = ManagedRouteNextHop<SaiIpNextHopTraits>;
using ManagedRouteMplsNextHop = ManagedRouteNextHop<SaiMplsNextHopTraits>;

struct SaiRouteHandle {
  using NextHopHandle = std::variant<
      std::shared_ptr<SaiNextHopGroupHandle>,
      std::shared_ptr<ManagedRouteIpNextHop>,
      std::shared_ptr<ManagedRouteMplsNextHop>>;
  NextHopHandle nexthopHandle_;
  std::shared_ptr<SaiCounterHandle> counterHandle_;
  std::shared_ptr<SaiRoute> route;
  sai_object_id_t nextHopAdapterKey() const;
  std::shared_ptr<SaiNextHopGroupHandle> nextHopGroupHandle() const;
};

class SaiRouteManager {
 public:
  SaiRouteManager(
      SaiStore* saiStore,
      SaiManagerTable* managerTable,
      const SaiPlatform* platform);
  // Helper function to create a SAI RouteEntry from an FBOSS SwitchState
  // Route (e.g., Route<IPAddressV6>)
  template <typename AddrT>
  SaiRouteTraits::RouteEntry routeEntryFromSwRoute(
      RouterID routerId,
      const std::shared_ptr<Route<AddrT>>& swEntry) const;

  template <typename AddrT>
  void changeRoute(
      const std::shared_ptr<Route<AddrT>>& oldSwRoute,
      const std::shared_ptr<Route<AddrT>>& newSwRoute,
      RouterID routerId);

  std::vector<std::shared_ptr<SaiRoute>> makeInterfaceToMeRoutes(
      const std::shared_ptr<Interface>& swInterface);

  template <typename AddrT>
  void addRoute(
      const std::shared_ptr<Route<AddrT>>& swRoute,
      RouterID routerId);

  template <typename AddrT>
  void removeRoute(
      const std::shared_ptr<Route<AddrT>>& swRoute,
      RouterID routerId);

  SaiRouteHandle* getRouteHandle(const SaiRouteTraits::RouteEntry& entry);
  const SaiRouteHandle* getRouteHandle(
      const SaiRouteTraits::RouteEntry& entry) const;

  void clear();

  std::shared_ptr<SaiObject<SaiRouteTraits>> getRouteObject(
      SaiRouteTraits::AdapterHostKey routeKey);

 private:
  SaiRouteHandle* getRouteHandleImpl(
      const SaiRouteTraits::RouteEntry& entry) const;
  template <typename AddrT>
  void addOrUpdateRoute(
      SaiRouteHandle* routeHandle,
      RouterID routerId,
      const std::shared_ptr<Route<AddrT>>& oldRoute,
      const std::shared_ptr<Route<AddrT>>& newRoute);

  template <typename AddrT>
  bool validRoute(const std::shared_ptr<Route<AddrT>>& swRoute);

  template <
      typename NextHopTraitsT,
      typename ManagedNextHopT = ManagedNextHop<NextHopTraitsT>,
      typename ManagedRouteNextHopT = ManagedRouteNextHop<NextHopTraitsT>>
  std::shared_ptr<ManagedRouteNextHopT> refOrCreateManagedRouteNextHop(
      SaiRouteHandle* routeHandle,
      SaiRouteTraits::RouteEntry entry,
      std::shared_ptr<ManagedNextHopT> nexthop);

  template <typename AddrT>
  std::shared_ptr<SaiCounterHandle> getCounterHandleForRoute(
      const std::shared_ptr<Route<AddrT>>& newRoute,
      const std::shared_ptr<Route<AddrT>>& oldRoute,
      std::optional<SaiRouteTraits::Attributes::CounterID>& counterID);

  SaiStore* saiStore_;
  SaiManagerTable* managerTable_;
  const SaiPlatform* platform_;
  folly::F14FastMap<SaiRouteTraits::RouteEntry, std::unique_ptr<SaiRouteHandle>>
      handles_;
};

} // namespace facebook::fboss
