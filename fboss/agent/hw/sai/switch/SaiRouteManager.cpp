/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/sai/switch/SaiRouteManager.h"
#include "fboss/agent/hw/sai/store/SaiStore.h"
#include "fboss/agent/hw/sai/switch/SaiManagerTable.h"
#include "fboss/agent/hw/sai/switch/SaiNextHopGroupManager.h"
#include "fboss/agent/hw/sai/switch/SaiRouterInterfaceManager.h"
#include "fboss/agent/hw/sai/switch/SaiSwitchManager.h"
#include "fboss/agent/hw/sai/switch/SaiVirtualRouterManager.h"

#include <optional>

namespace facebook {
namespace fboss {

SaiRouteManager::SaiRouteManager(
    SaiManagerTable* managerTable,
    const SaiPlatform* platform)
    : managerTable_(managerTable), platform_(platform) {}

template <typename AddrT>
SaiRouteTraits::RouteEntry SaiRouteManager::routeEntryFromSwRoute(
    RouterID routerId,
    const std::shared_ptr<Route<AddrT>>& swRoute) const {
  SwitchSaiId switchId = managerTable_->switchManager().getSwitchSaiId();
  folly::IPAddress prefixNetwork{swRoute->prefix().network};
  folly::CIDRNetwork prefix{prefixNetwork, swRoute->prefix().mask};
  SaiVirtualRouterHandle* virtualRouterHandle =
      managerTable_->virtualRouterManager().getVirtualRouterHandle(routerId);
  if (!virtualRouterHandle) {
    throw FbossError("No virtual router with id ", routerId);
  }
  return SaiRouteTraits::RouteEntry{
      switchId, virtualRouterHandle->virtualRouter->adapterKey(), prefix};
}

template <typename AddrT>
void SaiRouteManager::changeRoute(
    RouterID /* routerId */,
    const std::shared_ptr<Route<AddrT>>& /* oldSwRoute */,
    const std::shared_ptr<Route<AddrT>>& /* newSwRoute */) {
  // TODO: implement modifying an existing route
}

std::vector<std::shared_ptr<SaiRoute>> SaiRouteManager::makeInterfaceToMeRoutes(
    const std::shared_ptr<Interface>& swInterface) {
  std::vector<std::shared_ptr<SaiRoute>> toMeRoutes;
  // Compute information common to all addresses in the interface:
  // vr id
  RouterID routerId = swInterface->getRouterID();
  SaiVirtualRouterHandle* virtualRouterHandle =
      managerTable_->virtualRouterManager().getVirtualRouterHandle(routerId);
  if (!virtualRouterHandle) {
    throw FbossError("No virtual router with id ", routerId);
  }
  sai_object_id_t virtualRouterId =
      virtualRouterHandle->virtualRouter->adapterKey();
  ;
  // packet action
  sai_packet_action_t packetAction = SAI_PACKET_ACTION_FORWARD;
  SwitchSaiId switchId = managerTable_->switchManager().getSwitchSaiId();
  sai_object_id_t cpuPortId =
      SaiApiTable::getInstance()->switchApi().getAttribute2(
          switchId, SaiSwitchTraits::Attributes::CpuPort{});

  toMeRoutes.reserve(swInterface->getAddresses().size());
  // Compute per-address information
  for (const auto& address : swInterface->getAddresses()) {
    // empty next hop group -- this route will not manage the
    // lifetime of a next hop group
    std::shared_ptr<SaiNextHopGroupHandle> nextHopGroup;
    // destination
    folly::CIDRNetwork destination{address.first, address.first.bitCount()};
    SaiRouteTraits::RouteEntry entry{switchId, virtualRouterId, destination};
    SaiRouteTraits::CreateAttributes attributes{packetAction, cpuPortId};
    auto& store = SaiStore::getInstance()->get<SaiRouteTraits>();
    auto route = store.setObject(entry, attributes);
    toMeRoutes.emplace_back(route);
  }
  return toMeRoutes;
}

template <typename AddrT>
void SaiRouteManager::addRoute(
    RouterID routerId,
    const std::shared_ptr<Route<AddrT>>& swRoute) {
  SaiRouteTraits::RouteEntry entry = routeEntryFromSwRoute(routerId, swRoute);
  auto itr = handles_.find(entry);
  if (itr != handles_.end()) {
    throw FbossError(
        "Failure to add route. A route already exists to ",
        swRoute->prefix().str());
  }
  auto fwd = swRoute->getForwardInfo();
  sai_int32_t packetAction;
  std::optional<sai_object_id_t> nextHopIdOpt;
  std::shared_ptr<SaiNextHopGroupHandle> nextHopGroupHandle;
  if (fwd.getAction() == NEXTHOPS) {
    packetAction = SAI_PACKET_ACTION_FORWARD;
    /*
     * A Route which satisfies isConnected() is an interface subnet route.
     * It will have one NextHop with the ip configured for the interface
     * and with the configured InterfaceID.
     * We can use that InterfaceID to lookup the SAI id of the corresponding
     * router_interface which is the proper next hop for the subnet route.
     * (see sairoute.h for an explanation of router_interface_id as next hop)
     */
    if (swRoute->isConnected()) {
      CHECK_EQ(fwd.getNextHopSet().size(), 1);
      InterfaceID interfaceId{fwd.getNextHopSet().begin()->intf()};
      const SaiRouterInterfaceHandle* routerInterfaceHandle =
          managerTable_->routerInterfaceManager().getRouterInterfaceHandle(
              interfaceId);
      if (!routerInterfaceHandle) {
        throw FbossError(
            "cannot create subnet route without a sai_router_interface "
            "for InterfaceID: ",
            interfaceId);
      }
      nextHopIdOpt = routerInterfaceHandle->routerInterface->adapterKey();
    } else {
      /*
       * A Route which has NextHop(s) will create or reference an existing
       * SaiNextHopGroup corresponding to ECMP over those next hops. When no
       * route refers to a next hop set, it will be removed in SAI as well.
       */
      nextHopGroupHandle =
          managerTable_->nextHopGroupManager().incRefOrAddNextHopGroup(
              fwd.getNextHopSet());
      nextHopIdOpt = nextHopGroupHandle->nextHopGroup->adapterKey();
    }
  } else if (fwd.getAction() == TO_CPU) {
    packetAction = SAI_PACKET_ACTION_FORWARD;
    SwitchSaiId switchId = managerTable_->switchManager().getSwitchSaiId();
    sai_object_id_t cpuPortId =
        SaiApiTable::getInstance()->switchApi().getAttribute2(
            switchId, SaiSwitchTraits::Attributes::CpuPort{});
    nextHopIdOpt = cpuPortId;
  } else if (fwd.getAction() == DROP) {
    packetAction = SAI_PACKET_ACTION_DROP;
  }

  SaiRouteTraits::CreateAttributes attributes{packetAction, nextHopIdOpt};
  auto& store = SaiStore::getInstance()->get<SaiRouteTraits>();
  auto route = store.setObject(entry, attributes);
  auto routeHandle = std::make_unique<SaiRouteHandle>();
  routeHandle->route = route;
  routeHandle->nextHopGroupHandle = nextHopGroupHandle;
  handles_.emplace(entry, std::move(routeHandle));
}

template <typename AddrT>
void SaiRouteManager::removeRoute(
    RouterID routerId,
    const std::shared_ptr<Route<AddrT>>& swRoute) {
  SaiRouteTraits::RouteEntry entry = routeEntryFromSwRoute(routerId, swRoute);
  size_t count = handles_.erase(entry);
  if (!count) {
    throw FbossError(
        "Failed to remove non-existent route to ", swRoute->prefix().str());
  }
}

void SaiRouteManager::processRouteDelta(const StateDelta& delta) {
  for (const auto& routeDelta : delta.getRouteTablesDelta()) {
    RouterID routerId;
    if (routeDelta.getOld()) {
      routerId = routeDelta.getOld()->getID();
    } else {
      routerId = routeDelta.getNew()->getID();
    }
    auto processChanged = [this, routerId](
                              const auto& oldRoute, const auto& newRoute) {
      changeRoute(routerId, oldRoute, newRoute);
    };
    auto processAdded = [this, routerId](const auto& newRoute) {
      addRoute(routerId, newRoute);
    };
    auto processRemoved = [this, routerId](const auto& oldRoute) {
      removeRoute(routerId, oldRoute);
    };
    DeltaFunctions::forEachChanged(
        routeDelta.getRoutesV4Delta(),
        processChanged,
        processAdded,
        processRemoved);
    DeltaFunctions::forEachChanged(
        routeDelta.getRoutesV6Delta(),
        processChanged,
        processAdded,
        processRemoved);
  }
}

SaiRouteHandle* SaiRouteManager::getRouteHandle(
    const SaiRouteTraits::RouteEntry& entry) {
  return getRouteHandleImpl(entry);
}
const SaiRouteHandle* SaiRouteManager::getRouteHandle(
    const SaiRouteTraits::RouteEntry& entry) const {
  return getRouteHandleImpl(entry);
}
SaiRouteHandle* SaiRouteManager::getRouteHandleImpl(
    const SaiRouteTraits::RouteEntry& entry) const {
  auto itr = handles_.find(entry);
  if (itr == handles_.end()) {
    return nullptr;
  }
  if (!itr->second.get()) {
    XLOG(FATAL) << "invalid null route for destination: "
                << entry.destination().first << "/"
                << entry.destination().second;
  }
  return itr->second.get();
}

void SaiRouteManager::clear() {
  handles_.clear();
}

template SaiRouteTraits::RouteEntry
SaiRouteManager::routeEntryFromSwRoute<folly::IPAddressV6>(
    RouterID routerId,
    const std::shared_ptr<Route<folly::IPAddressV6>>& swEntry) const;
template SaiRouteTraits::RouteEntry
SaiRouteManager::routeEntryFromSwRoute<folly::IPAddressV4>(
    RouterID routerId,
    const std::shared_ptr<Route<folly::IPAddressV4>>& swEntry) const;

template void SaiRouteManager::changeRoute<folly::IPAddressV6>(
    RouterID routerId,
    const std::shared_ptr<Route<folly::IPAddressV6>>& oldSwEntry,
    const std::shared_ptr<Route<folly::IPAddressV6>>& newSwEntry);
template void SaiRouteManager::changeRoute<folly::IPAddressV4>(
    RouterID routerId,
    const std::shared_ptr<Route<folly::IPAddressV4>>& oldSwEntry,
    const std::shared_ptr<Route<folly::IPAddressV4>>& newSwEntry);

template void SaiRouteManager::addRoute<folly::IPAddressV6>(
    RouterID routerId,
    const std::shared_ptr<Route<folly::IPAddressV6>>& swEntry);
template void SaiRouteManager::addRoute<folly::IPAddressV4>(
    RouterID routerId,
    const std::shared_ptr<Route<folly::IPAddressV4>>& swEntry);

template void SaiRouteManager::removeRoute<folly::IPAddressV6>(
    RouterID routerId,
    const std::shared_ptr<Route<folly::IPAddressV6>>& swEntry);
template void SaiRouteManager::removeRoute<folly::IPAddressV4>(
    RouterID routerId,
    const std::shared_ptr<Route<folly::IPAddressV4>>& swEntry);

} // namespace fboss
} // namespace facebook
