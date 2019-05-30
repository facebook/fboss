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
#include "fboss/agent/hw/sai/switch/SaiManagerTable.h"
#include "fboss/agent/hw/sai/switch/SaiNextHopGroupManager.h"
#include "fboss/agent/hw/sai/switch/SaiRouterInterfaceManager.h"
#include "fboss/agent/hw/sai/switch/SaiSwitchManager.h"
#include "fboss/agent/hw/sai/switch/SaiVirtualRouterManager.h"

namespace facebook {
namespace fboss {

SaiRoute::SaiRoute(
    SaiApiTable* apiTable,
    SaiManagerTable* managerTable,
    const RouteApiParameters::EntryType& entry,
    const RouteApiParameters::Attributes& attributes,
    std::shared_ptr<SaiNextHopGroup> nextHopGroup)
    : apiTable_(apiTable),
      managerTable_(managerTable),
      entry_(entry),
      attributes_(attributes),
      nextHopGroup_(nextHopGroup) {
  auto& routeApi = apiTable_->routeApi();
  routeApi.create(entry_, attributes.attrs());
}

SaiRoute::~SaiRoute() {
  apiTable_->routeApi().remove(entry_);
}

bool SaiRoute::operator==(const SaiRoute& other) const {
  return attributes_ == other.attributes_;
}

bool SaiRoute::operator!=(const SaiRoute& other) const {
  return !(*this == other);
}

void SaiRoute::setAttributes(
    const RouteApiParameters::Attributes& desiredAttributes) {
  auto& routeApi = apiTable_->routeApi();
  if (desiredAttributes.packetAction != attributes_.packetAction) {
    routeApi.setAttribute(
        RouteApiParameters::Attributes::PacketAction{
            desiredAttributes.packetAction},
        entry_);
  }
  if ((desiredAttributes.nextHopId != attributes_.nextHopId) &&
      desiredAttributes.nextHopId) {
    routeApi.setAttribute(
        RouteApiParameters::Attributes::NextHopId{
            desiredAttributes.nextHopId.value()},
        entry_);
  }
  attributes_ = desiredAttributes;
}

SaiRouteManager::SaiRouteManager(
    SaiApiTable* apiTable,
    SaiManagerTable* managerTable,
    const SaiPlatform* platform)
    : apiTable_(apiTable), managerTable_(managerTable), platform_(platform) {}

template <typename AddrT>
RouteApiParameters::EntryType SaiRouteManager::routeEntryFromSwRoute(
    RouterID routerId,
    const std::shared_ptr<Route<AddrT>>& swRoute) const {
  auto switchId = managerTable_->switchManager().getSwitchSaiId(SwitchID(0));
  folly::IPAddress prefixNetwork{swRoute->prefix().network};
  folly::CIDRNetwork prefix{prefixNetwork, swRoute->prefix().mask};
  SaiVirtualRouter* virtualRouter =
      managerTable_->virtualRouterManager().getVirtualRouter(routerId);
  if (!virtualRouter) {
    throw FbossError("No virtual router with id ", routerId);
  }
  return RouteApiParameters::EntryType{switchId, virtualRouter->id(), prefix};
}

template <typename AddrT>
void SaiRouteManager::changeRoute(
    RouterID /* routerId */,
    const std::shared_ptr<Route<AddrT>>& /* oldSwRoute */,
    const std::shared_ptr<Route<AddrT>>& /* newSwRoute */) {
  // TODO: implement modifying an existing route
}

template <typename AddrT>
void SaiRouteManager::addRoute(
    RouterID routerId,
    const std::shared_ptr<Route<AddrT>>& swRoute) {
  RouteApiParameters::EntryType entry =
      routeEntryFromSwRoute(routerId, swRoute);
  auto itr = routes_.find(entry);
  if (itr != routes_.end()) {
    throw FbossError(
        "Failure to add route. A route already exists to ",
        swRoute->prefix().str());
  }
  auto fwd = swRoute->getForwardInfo();
  sai_int32_t packetAction = getSaiPacketAction(fwd.getAction());
  folly::Optional<sai_object_id_t> nextHopIdOpt;
  std::shared_ptr<SaiNextHopGroup> nextHopGroup;
  if (packetAction == SAI_PACKET_ACTION_FORWARD) {
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
      SaiRouterInterface* saiInterface =
          managerTable_->routerInterfaceManager().getRouterInterface(
              interfaceId);
      if (!saiInterface) {
        throw FbossError(
            "cannot create subnet route without a sai_router_interface "
            "for InterfaceID: ",
            interfaceId);
      }
      nextHopIdOpt = saiInterface->id();
    } else {
      nextHopGroup =
          managerTable_->nextHopGroupManager().incRefOrAddNextHopGroup(
              fwd.getNextHopSet());
      nextHopIdOpt = nextHopGroup->id();
    }
  }
  RouteApiParameters::Attributes attributes{{packetAction, nextHopIdOpt}};
  auto route = std::make_unique<SaiRoute>(
      apiTable_, managerTable_, entry, attributes, nextHopGroup);
  routes_.emplace(std::make_pair(entry, std::move(route)));
}

template <typename AddrT>
void SaiRouteManager::removeRoute(
    RouterID routerId,
    const std::shared_ptr<Route<AddrT>>& swRoute) {
  RouteApiParameters::EntryType entry =
      routeEntryFromSwRoute(routerId, swRoute);
  size_t count = routes_.erase(entry);
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

SaiRoute* SaiRouteManager::getRoute(
    const RouteApiParameters::EntryType& entry) {
  return getRouteImpl(entry);
}
const SaiRoute* SaiRouteManager::getRoute(
    const RouteApiParameters::EntryType& entry) const {
  return getRouteImpl(entry);
}
SaiRoute* SaiRouteManager::getRouteImpl(
    const RouteApiParameters::EntryType& entry) const {
  auto itr = routes_.find(entry);
  if (itr == routes_.end()) {
    return nullptr;
  }
  if (!itr->second.get()) {
    XLOG(FATAL) << "invalid null route for destination: "
                << entry.destination().first << "/"
                << entry.destination().second;
  }
  return itr->second.get();
}

sai_int32_t SaiRouteManager::getSaiPacketAction(
    const RouteForwardAction& action) const {
  switch (action) {
    case DROP:
      return SAI_PACKET_ACTION_DROP;
    case TO_CPU:
      return SAI_PACKET_ACTION_TRAP;
    case NEXTHOPS:
      return SAI_PACKET_ACTION_FORWARD;
  }
  folly::assume_unreachable();
}

void SaiRouteManager::clear() {
  routes_.clear();
}

template RouteApiParameters::EntryType
SaiRouteManager::routeEntryFromSwRoute<folly::IPAddressV6>(
    RouterID routerId,
    const std::shared_ptr<Route<folly::IPAddressV6>>& swEntry) const;
template RouteApiParameters::EntryType
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
