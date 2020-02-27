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

namespace facebook::fboss {

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
  VirtualRouterSaiId virtualRouterId{
      virtualRouterHandle->virtualRouter->adapterKey()};
  ;
  // packet action
  sai_packet_action_t packetAction = SAI_PACKET_ACTION_FORWARD;
  SwitchSaiId switchId = managerTable_->switchManager().getSwitchSaiId();
  PortSaiId cpuPortId{SaiApiTable::getInstance()->switchApi().getAttribute(
      switchId, SaiSwitchTraits::Attributes::CpuPort{})};

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
bool SaiRouteManager::validRoute(const std::shared_ptr<Route<AddrT>>& swRoute) {
  /*
   * For each subnet on an L3 Interface configured on the switch, FBOSS will
   * generate two routes: a subnet route to the prefix of the subnet,
   * necessary for routing packets to hosts in the subnet and a ToMe route
   * for the IP address of the switch itself in the subnet.
   *
   * If an interface is configured with a /32 or /128 subnet, then SwSwitch
   * will still produce a subnet route (important for recursive resolution
   * in the RIB) but there is no meaningful subnet route from the SAI
   * perspective: the only IP in the subnet is the switch itself.
   *
   * Packets that would hit this route should be punted via the ToMe route
   * programmed alongside the router interface (via a makeInterfaceToMeRoutes
   * call in RouterInterfaceManager). If we attempt to program the
   * subnet route, it will have the same destination and we will overwrite the
   * correct route. For that reason, such routes are not valid in
   * SaiRouteManager, and must be skipped.
   */
  // N.B., for now, this code looks a bit silly (could just do direct return)
  // but we use this style to suggest the possibility of future extension
  // with other conditions for invalid routes.
  if (swRoute->isConnected() && swRoute->isHostRoute()) {
    return false;
  }
  return true;
}

template <typename AddrT>
void SaiRouteManager::addOrUpdateRoute(
    SaiRouteHandle* routeHandle,
    RouterID routerId,
    const std::shared_ptr<Route<AddrT>>& swRoute) {
  SaiRouteTraits::RouteEntry entry = routeEntryFromSwRoute(routerId, swRoute);
  auto fwd = swRoute->getForwardInfo();
  sai_int32_t packetAction;
  std::optional<SaiRouteTraits::CreateAttributes> attributes;
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
      RouterInterfaceSaiId routerInterfaceId{
          routerInterfaceHandle->routerInterface->adapterKey()};
      attributes = SaiRouteTraits::CreateAttributes{
          packetAction, std::move(routerInterfaceId)};
    } else {
      /*
       * A Route which has NextHop(s) will create or reference an existing
       * SaiNextHopGroup corresponding to ECMP over those next hops. When no
       * route refers to a next hop set, it will be removed in SAI as well.
       */
      nextHopGroupHandle =
          managerTable_->nextHopGroupManager().incRefOrAddNextHopGroup(
              fwd.getNextHopSet());
      NextHopGroupSaiId nextHopGroupId{
          nextHopGroupHandle->nextHopGroup->adapterKey()};
      attributes = SaiRouteTraits::CreateAttributes{packetAction,
                                                    std::move(nextHopGroupId)};
    }
  } else if (fwd.getAction() == TO_CPU) {
    packetAction = SAI_PACKET_ACTION_FORWARD;
    SwitchSaiId switchId = managerTable_->switchManager().getSwitchSaiId();
    PortSaiId cpuPortId{SaiApiTable::getInstance()->switchApi().getAttribute(
        switchId, SaiSwitchTraits::Attributes::CpuPort{})};
    attributes =
        SaiRouteTraits::CreateAttributes{packetAction, std::move(cpuPortId)};
  } else if (fwd.getAction() == DROP) {
    packetAction = SAI_PACKET_ACTION_DROP;
    attributes = SaiRouteTraits::CreateAttributes{packetAction, std::nullopt};
  }
  auto& store = SaiStore::getInstance()->get<SaiRouteTraits>();
  auto route = store.setObject(entry, attributes.value());
  routeHandle->route = route;
  routeHandle->nextHopGroupHandle = nextHopGroupHandle;
}

template <typename AddrT>
void SaiRouteManager::changeRoute(
    RouterID routerId,
    const std::shared_ptr<Route<AddrT>>& /* oldSwRoute */,
    const std::shared_ptr<Route<AddrT>>& newSwRoute) {
  SaiRouteTraits::RouteEntry entry =
      routeEntryFromSwRoute(routerId, newSwRoute);
  auto itr = handles_.find(entry);
  if (itr == handles_.end()) {
    throw FbossError(
        "Failure to update route. Route does not exist ",
        newSwRoute->prefix().str());
  }
  if (!validRoute(newSwRoute)) {
    return;
  }
  addOrUpdateRoute(itr->second.get(), routerId, newSwRoute);
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
  if (!validRoute(swRoute)) {
    return;
  }
  auto routeHandle = std::make_unique<SaiRouteHandle>();
  addOrUpdateRoute(routeHandle.get(), routerId, swRoute);
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
                << static_cast<int>(entry.destination().second);
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

} // namespace facebook::fboss
