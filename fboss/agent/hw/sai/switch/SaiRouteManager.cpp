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
#include "fboss/agent/hw/sai/switch/SaiCounterManager.h"
#include "fboss/agent/hw/sai/switch/SaiManagerTable.h"
#include "fboss/agent/hw/sai/switch/SaiNextHopGroupManager.h"
#include "fboss/agent/hw/sai/switch/SaiNextHopManager.h"
#include "fboss/agent/hw/sai/switch/SaiRouterInterfaceManager.h"
#include "fboss/agent/hw/sai/switch/SaiSwitchManager.h"
#include "fboss/agent/hw/sai/switch/SaiVirtualRouterManager.h"
#include "fboss/agent/hw/switch_asics/HwAsic.h"

#include "fboss/agent/platforms/sai/SaiPlatform.h"

#include <optional>

DEFINE_bool(
    disable_valid_route_check,
    false,
    "Disable valid route check when creating or changing routes in SAI switches");

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
    SaiStore* saiStore,
    SaiManagerTable* managerTable,
    const SaiPlatform* platform)
    : saiStore_(saiStore), managerTable_(managerTable), platform_(platform) {}

template <typename AddrT>
SaiRouteTraits::RouteEntry SaiRouteManager::routeEntryFromSwRoute(
    RouterID routerId,
    const std::shared_ptr<Route<AddrT>>& swRoute) const {
  SwitchSaiId switchId = managerTable_->switchManager().getSwitchSaiId();
  folly::IPAddress prefixNetwork{swRoute->prefix().network()};
  folly::CIDRNetwork prefix{prefixNetwork, swRoute->prefix().mask()};
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
  PortSaiId cpuPortId = managerTable_->switchManager().getCpuPort();

  toMeRoutes.reserve(swInterface->getAddresses()->size());
  // Compute per-address information
  for (auto iter : std::as_const(*swInterface->getAddresses())) {
    folly::CIDRNetwork address(
        folly::IPAddress(iter.first), iter.second->cref());
    // empty next hop group -- this route will not manage the
    // lifetime of a next hop group
    std::shared_ptr<SaiNextHopGroupHandle> nextHopGroup;
    // destination
    folly::CIDRNetwork destination{address.first, address.first.bitCount()};
    SaiRouteTraits::RouteEntry entry{switchId, virtualRouterId, destination};
#if SAI_API_VERSION >= SAI_VERSION(1, 10, 0)
    SaiRouteTraits::CreateAttributes attributes{
        packetAction, cpuPortId, std::nullopt, std::nullopt};
#else
    SaiRouteTraits::CreateAttributes attributes{
        packetAction, cpuPortId, std::nullopt};
#endif
    auto& store = saiStore_->get<SaiRouteTraits>();
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
  if (!FLAGS_disable_valid_route_check && swRoute->isConnected() &&
      swRoute->isHostRoute()) {
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
  const auto& fwd = newRoute->getForwardInfo();
  sai_int32_t packetAction;
  std::optional<SaiRouteTraits::CreateAttributes> attributes;
  SaiRouteHandle::NextHopHandle nextHopHandle;
  std::optional<SaiRouteTraits::Attributes::Metadata> metadata;
  std::optional<SaiRouteTraits::Attributes::CounterID> counterID;
  std::shared_ptr<SaiCounterHandle> counterHandle;
  if (newRoute->getClassID()) {
    metadata = static_cast<sai_uint32_t>(newRoute->getClassID().value());
  } else if (oldRoute && oldRoute->getClassID()) {
    metadata = 0;
  }
  counterHandle = getCounterHandleForRoute(newRoute, oldRoute, counterID);

  if (fwd.getAction() == RouteForwardAction::NEXTHOPS) {
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
          routerInterfaceHandle->adapterKey()};
#if SAI_API_VERSION >= SAI_VERSION(1, 10, 0)
      attributes = SaiRouteTraits::CreateAttributes{
          packetAction, routerInterfaceId, metadata, std::nullopt};
#else
      attributes = SaiRouteTraits::CreateAttributes{
          packetAction, routerInterfaceId, metadata};
#endif

      XLOG(DBG3) << "Connected route: " << newRoute->str()
                 << " routerInterfaceId: " << routerInterfaceId;
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
#if SAI_API_VERSION >= SAI_VERSION(1, 10, 0)
      attributes = SaiRouteTraits::CreateAttributes{
          packetAction, nextHopGroupId, metadata, counterID};
#else
      attributes = SaiRouteTraits::CreateAttributes{
          packetAction, nextHopGroupId, metadata};
#endif
      nextHopHandle = nextHopGroupHandle;

      XLOG(DBG3) << "Route nhops > 1: " << newRoute->str()
                 << " nextHopGroupId: " << nextHopGroupId;
    } else {
      CHECK_EQ(fwd.getNextHopSet().size(), 1);
      /* A route which has oonly one next hop, create a subscriber for next hop
       * to make route point back and forth next hop or CPU
       */
      auto swNextHop =
          folly::poly_cast<ResolvedNextHop>(*(fwd.getNextHopSet().begin()));
      auto managedSaiNextHop =
          managerTable_->nextHopManager().addManagedSaiNextHop(swNextHop);
      sai_object_id_t nextHopId{};

      /* claim the next hop first */
      std::visit(
          [&](auto& managedNextHop) {
            using SharedPtrType = std::decay_t<decltype(managedNextHop)>;
            using NextHopTraits =
                typename SharedPtrType::element_type::ObjectTraits;

            auto managedRouteNextHop =
                refOrCreateManagedRouteNextHop<NextHopTraits>(
                    routeHandle, entry, managedNextHop);

            nextHopId = managedRouteNextHop->adapterKey();
            nextHopHandle = managedRouteNextHop;
          },
          managedSaiNextHop);

#if SAI_API_VERSION >= SAI_VERSION(1, 10, 0)
      attributes = SaiRouteTraits::CreateAttributes{
          packetAction, nextHopId, metadata, counterID};
#else
      attributes =
          SaiRouteTraits::CreateAttributes{packetAction, nextHopId, metadata};
#endif

      XLOG(DBG3) << "Route nhops == 1: " << newRoute->str()
                 << " nextHopId: " << nextHopId;
    }
  } else if (fwd.getAction() == RouteForwardAction::TO_CPU) {
    packetAction = SAI_PACKET_ACTION_FORWARD;
    PortSaiId cpuPortId = managerTable_->switchManager().getCpuPort();
#if SAI_API_VERSION >= SAI_VERSION(1, 10, 0)
    attributes = SaiRouteTraits::CreateAttributes{
        packetAction, cpuPortId, metadata, std::nullopt};
#else
    attributes =
        SaiRouteTraits::CreateAttributes{packetAction, cpuPortId, metadata};
#endif

    XLOG(DBG3) << "Route action TO CPU: " << newRoute->str()
               << " cpuPortId: " << cpuPortId;
  } else if (fwd.getAction() == RouteForwardAction::DROP) {
    packetAction = SAI_PACKET_ACTION_DROP;
#if SAI_API_VERSION >= SAI_VERSION(1, 10, 0)
    attributes = SaiRouteTraits::CreateAttributes{
        packetAction, SAI_NULL_OBJECT_ID, metadata, std::nullopt};
#else
    attributes = SaiRouteTraits::CreateAttributes{
        packetAction, SAI_NULL_OBJECT_ID, metadata};
#endif

    XLOG(DBG3) << "Route action DROP: " << newRoute->str();
  }
  auto& store = saiStore_->get<SaiRouteTraits>();
  auto route = store.setObject(entry, attributes.value());
  routeHandle->route = route;
  routeHandle->nexthopHandle_ = nextHopHandle;
  routeHandle->counterHandle_ = counterHandle;
}

template <typename AddrT>
void SaiRouteManager::changeRoute(
    const std::shared_ptr<Route<AddrT>>& oldSwRoute,
    const std::shared_ptr<Route<AddrT>>& newSwRoute,
    RouterID routerId) {
  SaiRouteTraits::RouteEntry entry =
      routeEntryFromSwRoute(routerId, newSwRoute);

  if (!validRoute(newSwRoute)) {
    XLOG(DBG3) << "Not a valid route, don't change:: old: " << oldSwRoute->str()
               << " new: " << newSwRoute->str();
    return;
  }

  auto itr = handles_.find(entry);
  if (itr == handles_.end()) {
    throw FbossError(
        "Failure to update route. Route does not exist ",
        newSwRoute->prefix().str());
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
    XLOG(DBG3) << "Not a valid route, don't add: " << swRoute->str();
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
  XLOG(DBG3) << "Remove route: " << swRoute->str();
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

std::shared_ptr<SaiObject<SaiRouteTraits>> SaiRouteManager::getRouteObject(
    SaiRouteTraits::AdapterHostKey routeKey) {
  return saiStore_->get<SaiRouteTraits>().get(routeKey);
}

template <typename AddrT>
std::shared_ptr<SaiCounterHandle> SaiRouteManager::getCounterHandleForRoute(
    const std::shared_ptr<Route<AddrT>>& newRoute,
    const std::shared_ptr<Route<AddrT>>& oldRoute,
    std::optional<SaiRouteTraits::Attributes::CounterID>& counterID) {
  std::shared_ptr<SaiCounterHandle> counterHandle;
  // Counters are supported only with IPv6 prefixes
  if constexpr (std::is_same_v<AddrT, folly::IPAddressV6>) {
    const auto& fwd = newRoute->getForwardInfo();
    if (fwd.getCounterID().has_value()) {
      counterHandle = managerTable_->counterManager().incRefOrAddRouteCounter(
          fwd.getCounterID().value());
      if (counterHandle) {
        counterID.emplace(counterHandle->adapterKey());
      }
    } else if (
        oldRoute && oldRoute->getForwardInfo().getCounterID().has_value()) {
      counterID = 0;
    }
  }
  return counterHandle;
}

template <
    typename NextHopTraitsT,
    typename ManagedNextHopT,
    typename ManagedRouteNextHopT>
std::shared_ptr<ManagedRouteNextHopT>
SaiRouteManager::refOrCreateManagedRouteNextHop(
    SaiRouteHandle* routeHandle,
    SaiRouteTraits::RouteEntry entry,
    std::shared_ptr<ManagedNextHopT> nexthop) {
  auto routeNexthopHandle = routeHandle->nexthopHandle_;
  using ManagedNextHopSharedPtr = std::shared_ptr<ManagedRouteNextHopT>;
  if (std::holds_alternative<ManagedNextHopSharedPtr>(routeNexthopHandle)) {
    auto existingManagedRouteNextHop =
        std::get<ManagedNextHopSharedPtr>(routeNexthopHandle);
    CHECK(existingManagedRouteNextHop) << "null managed route next hop";
    if (existingManagedRouteNextHop->adapterHostKey() ==
        nexthop->adapterHostKey()) {
      return existingManagedRouteNextHop;
    }
  }
  auto routeMetadataSupported =
      platform_->getAsic()->isSupported(HwAsic::Feature::ROUTE_METADATA);
  PortSaiId cpuPort = managerTable_->switchManager().getCpuPort();
  auto managedRouteNextHop = std::make_shared<ManagedRouteNextHopT>(
      cpuPort, this, entry, nexthop, routeMetadataSupported);
  SaiObjectEventPublisher::getInstance()->get<NextHopTraitsT>().subscribe(
      managedRouteNextHop);
  return managedRouteNextHop;
}

template <typename NextHopTraitsT>
ManagedRouteNextHop<NextHopTraitsT>::ManagedRouteNextHop(
    PortSaiId cpuPort,
    SaiRouteManager* routeManager,
    SaiRouteTraits::AdapterHostKey routeKey,
    std::shared_ptr<ManagedNextHop<NextHopTraitsT>> managedNextHop,
    bool routeMetadataSupported)
    : detail::SaiObjectEventSubscriber<NextHopTraitsT>(
          managedNextHop->adapterHostKey()),
      cpuPort_(cpuPort),
      routeManager_(routeManager),
      routeKey_(std::move(routeKey)),
      managedNextHop_(managedNextHop),
      routeMetadataSupported_(routeMetadataSupported) {}

template <typename NextHopTraitsT>
void ManagedRouteNextHop<NextHopTraitsT>::afterCreate(
    ManagedRouteNextHop<NextHopTraitsT>::PublisherObject nexthop) {
  CHECK(adapterKey() != nexthop->adapterKey())
      << "attempting to reset to the same next hop";
  this->setPublisherObject(nexthop);
  // set route to next hop
  auto route = routeManager_->getRouteObject(routeKey_);
  if (!route) {
    XLOG(DBG2) << "ManagedRouteNextHop afterCreate , route not yet created: "
               << routeKey_.toString();
    // route is not yet created.
    return;
  }
  auto& api = SaiApiTable::getInstance()->routeApi();
  SaiRouteTraits::Attributes::Metadata currentMetadata = routeMetadataSupported_
      ? api.getAttribute(
            route->adapterKey(), SaiRouteTraits::Attributes::Metadata{})
      : 0;
  auto attributes = route->attributes();
  sai_object_id_t nextHopId = nexthop->adapterKey();
  auto& currentNextHop =
      std::get<std::optional<SaiRouteTraits::Attributes::NextHopId>>(
          attributes);
  currentNextHop = nextHopId;
  route->setAttributes(attributes);
  updateMetadata(currentMetadata);
  XLOG(DBG2) << "ManagedRouteNextHop afterCreate: " << routeKey_.toString()
             << " assign nextHopId: " << nextHopId;
}

template <typename NextHopTraitsT>
void ManagedRouteNextHop<NextHopTraitsT>::beforeRemove() {
  XLOG(DBG2) << "ManagedRouteNextHop beforeRemove, set route to CPU: "
             << routeKey_.toString();

  // set route to CPU
  auto route = routeManager_->getRouteObject(routeKey_);
  auto attributes = route->attributes();

  std::get<std::optional<SaiRouteTraits::Attributes::NextHopId>>(attributes) =
      static_cast<sai_object_id_t>(cpuPort_);
  route->setAttributes(attributes);
  this->setPublisherObject(nullptr);
}

template <typename NextHopTraitsT>
sai_object_id_t ManagedRouteNextHop<NextHopTraitsT>::adapterKey() const {
  if (!this->isReady()) {
    return static_cast<sai_object_id_t>(cpuPort_);
  }
  return this->getPublisherObject().lock()->adapterKey();
}

template <typename NextHopTraitsT>
void ManagedRouteNextHop<NextHopTraitsT>::updateMetadata(
    SaiRouteTraits::Attributes::Metadata currentMetadata) const {
  auto route = routeManager_->getRouteObject(routeKey_);
  CHECK(route);

  auto expectedMetadata =
      std::get<std::optional<SaiRouteTraits::Attributes::Metadata>>(
          route->attributes());
  if (!expectedMetadata) {
    // SAI object of route pointing to CPU, and directly reloaded from store may
    // have reserved metadata (set by SDK) in its attributes.
    expectedMetadata = SaiRouteTraits::Attributes::Metadata::defaultValue();
  }

  if (expectedMetadata.value() != currentMetadata) {
    /*
     * Set Sai object attr to actual metadata and then update.
     * In BRCM-SAI there is a case where the SAI SDK itself updates
     * metadata of a route when it points to CPU. Now in SaiObject
     * the metatdata may still be set to expectedMetadata, while the
     * HW would have it be == actualMetadata. So force our in memory
     * state to reconcile with HW value and then set to desired
     * expectedMetadata value
     */
    route->setOptionalAttribute(
        SaiRouteTraits::Attributes::Metadata{currentMetadata},
        true /*skip HW write*/);
    route->setAttribute(expectedMetadata);
  }
}

template <typename NextHopTraitsT>
typename NextHopTraitsT::AdapterHostKey
ManagedRouteNextHop<NextHopTraitsT>::adapterHostKey() const {
  return this->managedNextHop_->adapterHostKey();
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
