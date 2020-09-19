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
#include "fboss/agent/hw/sai/switch/SaiNextHopManager.h"
#include "fboss/agent/hw/sai/switch/SaiRouterInterfaceManager.h"
#include "fboss/agent/hw/sai/switch/SaiSwitchManager.h"
#include "fboss/agent/hw/sai/switch/SaiVirtualRouterManager.h"
#include "fboss/agent/hw/switch_asics/HwAsic.h"

#include "fboss/agent/platforms/sai/SaiPlatform.h"

#include <optional>

namespace facebook::fboss {

sai_object_id_t SaiRouteHandle::nextHopAdapterKey() const {
  return std::visit(
      [](auto& handle) { return handle->adapterKey(); }, nexthopHandle_);
}

std::shared_ptr<SaiNextHopGroupHandle> SaiRouteHandle::nextHopGroupHandle()
    const {
  auto* nextHopGroupHandle =
      std::get_if<std::shared_ptr<SaiNextHopGroupHandle>>(&nexthopHandle_);
  if (!nextHopGroupHandle) {
    return nullptr;
  }
  return *nextHopGroupHandle;
}

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
    SaiRouteTraits::CreateAttributes attributes{
        packetAction, cpuPortId, std::nullopt};
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
    const std::shared_ptr<Route<AddrT>>& oldRoute,
    const std::shared_ptr<Route<AddrT>>& newRoute) {
  SaiRouteTraits::RouteEntry entry = routeEntryFromSwRoute(routerId, newRoute);
  auto fwd = newRoute->getForwardInfo();
  sai_int32_t packetAction;
  std::optional<SaiRouteTraits::CreateAttributes> attributes;
  SaiRouteHandle::NextHopHandle nextHopHandle;
  std::optional<SaiRouteTraits::Attributes::Metadata> metadata;
  if (newRoute->getClassID()) {
    metadata = static_cast<sai_uint32_t>(newRoute->getClassID().value());
  } else if (oldRoute && oldRoute->getClassID()) {
    metadata = 0;
  }

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
    if (newRoute->isConnected()) {
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
          packetAction, std::move(routerInterfaceId), metadata};
    } else if (fwd.getNextHopSet().size() > 1) {
      /*
       * A Route which has more than one NextHops will create or reference an
       * existing SaiNextHopGroup corresponding to ECMP over those next hops.
       * When no route refers to a next hop set, it will be removed in SAI as
       * well.
       */
      auto nextHopGroupHandle =
          managerTable_->nextHopGroupManager().incRefOrAddNextHopGroup(
              fwd.normalizedNextHops());
      NextHopGroupSaiId nextHopGroupId{
          nextHopGroupHandle->nextHopGroup->adapterKey()};
      attributes = SaiRouteTraits::CreateAttributes{
          packetAction, std::move(nextHopGroupId), metadata};
      nextHopHandle = nextHopGroupHandle;
    } else {
      CHECK_EQ(fwd.getNextHopSet().size(), 1);
      /* A route which has oonly one next hop, create a subscriber for next hop
       * to make route point back and forth next hop or CPU
       */
      auto swNextHop =
          folly::poly_cast<ResolvedNextHop>(*(fwd.getNextHopSet().begin()));
      auto managedNextHop =
          managerTable_->nextHopManager().refOrEmplaceNextHop(swNextHop);

      SwitchSaiId switchId = managerTable_->switchManager().getSwitchSaiId();
      sai_object_id_t nextHopId{
          SaiApiTable::getInstance()->switchApi().getAttribute(
              switchId, SaiSwitchTraits::Attributes::CpuPort{})};
      ;
      /* claim the next hop first */
      if (auto* ipNextHop =
              std::get_if<std::shared_ptr<ManagedIpNextHop>>(&managedNextHop)) {
        auto managedRouteNextHop =
            std::make_shared<ManagedRouteNextHop<SaiIpNextHopTraits>>(
                managerTable_, platform_, entry, *ipNextHop);
        SaiObjectEventPublisher::getInstance()
            ->get<SaiIpNextHopTraits>()
            .subscribe(managedRouteNextHop);
        if (managedRouteNextHop->isReady()) {
          nextHopId =
              managedRouteNextHop->getPublisherObject().lock()->adapterKey();
        }
        routeHandle->nexthopHandle_ = managedRouteNextHop;
      } else if (
          auto* mplsNextHop = std::get_if<std::shared_ptr<ManagedMplsNextHop>>(
              &managedNextHop)) {
        auto managedRouteNextHop =
            std::make_shared<ManagedRouteNextHop<SaiMplsNextHopTraits>>(
                managerTable_, platform_, entry, *mplsNextHop);
        SaiObjectEventPublisher::getInstance()
            ->get<SaiMplsNextHopTraits>()
            .subscribe(managedRouteNextHop);
        if (managedRouteNextHop->isReady()) {
          nextHopId =
              managedRouteNextHop->getPublisherObject().lock()->adapterKey();
        }
        routeHandle->nexthopHandle_ = managedRouteNextHop;
      }
      attributes =
          SaiRouteTraits::CreateAttributes{packetAction, nextHopId, metadata};

      /* claim the route now */
      auto& store = SaiStore::getInstance()->get<SaiRouteTraits>();
      auto route = store.setObject(entry, attributes.value());
      routeHandle->route = route;
      return;
    }
  } else if (fwd.getAction() == TO_CPU) {
    packetAction = SAI_PACKET_ACTION_FORWARD;
    SwitchSaiId switchId = managerTable_->switchManager().getSwitchSaiId();
    PortSaiId cpuPortId{SaiApiTable::getInstance()->switchApi().getAttribute(
        switchId, SaiSwitchTraits::Attributes::CpuPort{})};
    attributes = SaiRouteTraits::CreateAttributes{
        packetAction, std::move(cpuPortId), metadata};
  } else if (fwd.getAction() == DROP) {
    packetAction = SAI_PACKET_ACTION_DROP;
    attributes = SaiRouteTraits::CreateAttributes{
        packetAction, SAI_NULL_OBJECT_ID, metadata};
  }
  auto& store = SaiStore::getInstance()->get<SaiRouteTraits>();
  auto route = store.setObject(entry, attributes.value());
  routeHandle->route = route;
  routeHandle->nexthopHandle_ = nextHopHandle;
}

template <typename AddrT>
void SaiRouteManager::changeRoute(
    const std::shared_ptr<Route<AddrT>>& oldSwRoute,
    const std::shared_ptr<Route<AddrT>>& newSwRoute,
    RouterID routerId) {
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
  addOrUpdateRoute(itr->second.get(), routerId, oldSwRoute, newSwRoute);
}

template <typename AddrT>
void SaiRouteManager::addRoute(
    const std::shared_ptr<Route<AddrT>>& swRoute,
    RouterID routerId) {
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
  addOrUpdateRoute(
      routeHandle.get(), routerId, std::shared_ptr<Route<AddrT>>{}, swRoute);
  handles_.emplace(entry, std::move(routeHandle));
}

template <typename AddrT>
void SaiRouteManager::removeRoute(
    const std::shared_ptr<Route<AddrT>>& swRoute,
    RouterID routerId) {
  SaiRouteTraits::RouteEntry entry = routeEntryFromSwRoute(routerId, swRoute);
  size_t count = handles_.erase(entry);
  if (!count) {
    throw FbossError(
        "Failed to remove non-existent route to ", swRoute->prefix().str());
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

template <typename NextHopTraitsT>
ManagedRouteNextHop<NextHopTraitsT>::ManagedRouteNextHop(
    SaiManagerTable* managerTable,
    const SaiPlatform* platform,
    SaiRouteTraits::AdapterHostKey routeKey,
    std::shared_ptr<ManagedNextHop<NextHopTraitsT>> managedNextHop)
    : detail::SaiObjectEventSubscriber<NextHopTraitsT>(
          managedNextHop->adapterHostKey()),
      managerTable_(managerTable),
      platform_(platform),
      routeKey_(std::move(routeKey)),
      managedNextHop_(managedNextHop) {}

template <typename NextHopTraitsT>
void ManagedRouteNextHop<NextHopTraitsT>::afterCreate(
    ManagedRouteNextHop<NextHopTraitsT>::PublisherObject nexthop) {
  this->setPublisherObject(nexthop);
  // set route to next hop
  auto route = SaiStore::getInstance()->get<SaiRouteTraits>().get(routeKey_);
  if (!route) {
    // route is not yet created.
    return;
  }
  auto attributes = route->attributes();
  sai_object_id_t nextHopId = nexthop->adapterKey();
  std::get<std::optional<SaiRouteTraits::Attributes::NextHopId>>(attributes) =
      nextHopId;
  route->setAttributes(attributes);
  if (platform_->getAsic()->isSupported(HwAsic::Feature::ROUTE_METADATA)) {
    updateMetadata();
  }
}

template <typename NextHopTraitsT>
void ManagedRouteNextHop<NextHopTraitsT>::beforeRemove() {
  // set route to CPU
  auto route = SaiStore::getInstance()->get<SaiRouteTraits>().get(routeKey_);
  auto attributes = route->attributes();

  SwitchSaiId switchId = managerTable_->switchManager().getSwitchSaiId();
  sai_object_id_t cpuPortId{
      SaiApiTable::getInstance()->switchApi().getAttribute(
          switchId, SaiSwitchTraits::Attributes::CpuPort{})};
  std::get<std::optional<SaiRouteTraits::Attributes::NextHopId>>(attributes) =
      cpuPortId;
  route->setAttributes(attributes);
  this->setPublisherObject(nullptr);
}

template <typename NextHopTraitsT>
sai_object_id_t ManagedRouteNextHop<NextHopTraitsT>::adapterKey() const {
  auto route = SaiStore::getInstance()->get<SaiRouteTraits>().get(routeKey_);
  CHECK(route);
  auto attributes = route->attributes();
  return GET_OPT_ATTR(Route, NextHopId, attributes);
}

template <typename NextHopTraitsT>
void ManagedRouteNextHop<NextHopTraitsT>::updateMetadata() const {
  auto route = SaiStore::getInstance()->get<SaiRouteTraits>().get(routeKey_);
  CHECK(route);

  auto expectedMetadata =
      std::get<std::optional<SaiRouteTraits::Attributes::Metadata>>(
          route->attributes());

  auto& api = SaiApiTable::getInstance()->routeApi();
  std::optional<SaiRouteTraits::Attributes::Metadata> actualMetadata =
      api.getAttribute(
          route->adapterKey(), SaiRouteTraits::Attributes::Metadata{});

  if (!expectedMetadata.has_value()) {
    SaiRouteTraits::Attributes::Metadata resetMetadata =
        SaiRouteTraits::Attributes::Metadata::defaultValue();
    api.setAttribute(route->adapterKey(), resetMetadata);
  } else if (expectedMetadata != actualMetadata) {
    api.setAttribute(route->adapterKey(), expectedMetadata.value());
  }
}

template class ManagedRouteNextHop<SaiIpNextHopTraits>;
template class ManagedRouteNextHop<SaiMplsNextHopTraits>;

template SaiRouteTraits::RouteEntry
SaiRouteManager::routeEntryFromSwRoute<folly::IPAddressV6>(
    RouterID routerId,
    const std::shared_ptr<Route<folly::IPAddressV6>>& swEntry) const;
template SaiRouteTraits::RouteEntry
SaiRouteManager::routeEntryFromSwRoute<folly::IPAddressV4>(
    RouterID routerId,
    const std::shared_ptr<Route<folly::IPAddressV4>>& swEntry) const;

template void SaiRouteManager::changeRoute<folly::IPAddressV6>(
    const std::shared_ptr<Route<folly::IPAddressV6>>& oldSwEntry,
    const std::shared_ptr<Route<folly::IPAddressV6>>& newSwEntry,
    RouterID routerId);
template void SaiRouteManager::changeRoute<folly::IPAddressV4>(
    const std::shared_ptr<Route<folly::IPAddressV4>>& oldSwEntry,
    const std::shared_ptr<Route<folly::IPAddressV4>>& newSwEntry,
    RouterID routerId);

template void SaiRouteManager::addRoute<folly::IPAddressV6>(
    const std::shared_ptr<Route<folly::IPAddressV6>>& swEntry,
    RouterID routerId);
template void SaiRouteManager::addRoute<folly::IPAddressV4>(
    const std::shared_ptr<Route<folly::IPAddressV4>>& swEntry,
    RouterID routerId);

template void SaiRouteManager::removeRoute<folly::IPAddressV6>(
    const std::shared_ptr<Route<folly::IPAddressV6>>& swEntry,
    RouterID routerId);
template void SaiRouteManager::removeRoute<folly::IPAddressV4>(
    const std::shared_ptr<Route<folly::IPAddressV4>>& swEntry,
    RouterID routerId);

} // namespace facebook::fboss
