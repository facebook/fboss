/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/sai/switch/SaiNextHopManager.h"

#include "fboss/agent/FbossError.h"
#include "fboss/agent/hw/sai/store/SaiStore.h"
#include "fboss/agent/hw/sai/switch/SaiManagerTable.h"
#include "fboss/agent/hw/sai/switch/SaiRouterInterfaceManager.h"
#include "fboss/agent/hw/sai/switch/SaiSrv6Manager.h"
#include "fboss/agent/hw/sai/switch/SaiSrv6TunnelManager.h"
#include "fboss/agent/hw/sai/switch/SaiSwitchManager.h"
#include "fboss/agent/hw/switch_asics/HwAsic.h"
#include "fboss/agent/platforms/sai/SaiPlatform.h"

#include <folly/logging/xlog.h>

#include "fboss/agent/state/RouteNextHop.h"

namespace facebook::fboss {

std::shared_ptr<SaiIpNextHop> SaiNextHopManager::addNextHop(
    RouterInterfaceSaiId routerInterfaceId,
    const folly::IPAddress& ip) {
  SaiIpNextHopTraits::AdapterHostKey k{routerInterfaceId, ip};
  SaiIpNextHopTraits::CreateAttributes attributes{
      SAI_NEXT_HOP_TYPE_IP, routerInterfaceId, ip, std::nullopt};
  return createSaiObject<SaiIpNextHopTraits>(k, attributes);
}

SaiNextHopManager::SaiNextHopManager(
    SaiStore* saiStore,
    SaiManagerTable* managerTable,
    const SaiPlatform* platform)
    : saiStore_(saiStore), managerTable_(managerTable), platform_(platform) {}

SaiNextHopTraits::AdapterHostKey SaiNextHopManager::getAdapterHostKey(
    const ResolvedNextHop& swNextHop,
    std::optional<sai_object_id_t> sidListId) {
  auto interfaceId = swNextHop.intfID().value();
  auto routerInterfaceHandle =
      managerTable_->routerInterfaceManager().getRouterInterfaceHandle(
          interfaceId);
  if (!routerInterfaceHandle) {
    throw FbossError("Missing SAI router interface for ", interfaceId);
  }
  auto rifId = routerInterfaceHandle->adapterKey();
  folly::IPAddress ip = swNextHop.addr();

#if SAI_API_VERSION >= SAI_VERSION(1, 12, 0)
  if (!swNextHop.srv6SegmentList().empty() &&
      swNextHop.tunnelId().has_value()) {
    auto tunnelHandle = managerTable_->srv6TunnelManager().getSrv6TunnelHandle(
        swNextHop.tunnelId().value());
    if (!tunnelHandle) {
      throw FbossError(
          "Missing SRv6 tunnel for tunnel ID: ", swNextHop.tunnelId().value());
    }
    auto tunnelSaiId = tunnelHandle->tunnel->adapterKey();
    CHECK(sidListId.has_value())
        << "sidListId must be provided for next hop with non-empty srv6SegmentList";
    auto sidListSaiId = sidListId.value();
    return SaiSrv6SidlistNextHopTraits::AdapterHostKey{
        rifId, ip, tunnelSaiId, sidListSaiId};
  }
#endif

  auto labelForwardingAction = swNextHop.labelForwardingAction();
  if (!labelForwardingAction ||
      LabelForwardingAction::LabelForwardingType::PHP ==
          labelForwardingAction->type()) {
    return SaiIpNextHopTraits::AdapterHostKey{rifId, ip};
  }

  CHECK(
      labelForwardingAction->type() !=
      LabelForwardingAction::LabelForwardingType::POP_AND_LOOKUP)
      << "action pop is not supported";

  std::vector<sai_uint32_t> labels;
  if (LabelForwardingAction::LabelForwardingType::SWAP ==
      labelForwardingAction->type()) {
    labels.push_back(swNextHop.labelForwardingAction()->swapWith().value());
    return SaiMplsNextHopTraits::AdapterHostKey{rifId, ip, labels};
  }
  for (auto label : labelForwardingAction->pushStack().value()) {
    labels.push_back(label);
  }

  return SaiMplsNextHopTraits::AdapterHostKey{rifId, ip, labels};
}

#if SAI_API_VERSION >= SAI_VERSION(1, 12, 0)
std::shared_ptr<SaiSrv6SidList> SaiNextHopManager::createSrv6SidList(
    const ResolvedNextHop& swNextHop) {
  if (swNextHop.srv6SegmentList().empty()) {
    throw FbossError(
        "Cannot create SRv6 SID list for next hop with empty srv6SegmentList");
  }
  auto interfaceId = swNextHop.intfID().value();
  auto routerInterfaceHandle =
      managerTable_->routerInterfaceManager().getRouterInterfaceHandle(
          interfaceId);
  if (!routerInterfaceHandle) {
    throw FbossError("Missing SAI router interface for ", interfaceId);
  }
  auto rifId = routerInterfaceHandle->adapterKey();
  folly::IPAddress ip = swNextHop.addr();
  SaiSrv6SidListTraits::CreateAttributes sidListCreateAttrs{
      SAI_SRV6_SIDLIST_TYPE_ENCAPS_RED,
      swNextHop.srv6SegmentList(),
      std::nullopt};
  SaiSrv6SidListTraits::AdapterHostKey sidListKey{
      SAI_SRV6_SIDLIST_TYPE_ENCAPS_RED, swNextHop.srv6SegmentList(), rifId, ip};
  auto& store = saiStore_->get<SaiSrv6SidListTraits>();
  return store.setObject(sidListKey, sidListCreateAttrs);
}
#endif

ManagedSaiNextHop SaiNextHopManager::addManagedSaiNextHop(
    const ResolvedNextHop& swNextHop
#if SAI_API_VERSION >= SAI_VERSION(1, 12, 0)
    ,
    std::shared_ptr<SaiSrv6SidList> srv6SidList
#endif
) {
  auto switchId = managerTable_->switchManager().getSwitchSaiId();
  folly::IPAddress ip = swNextHop.addr();

  XLOG(DBG2) << "SaiNextHopManager::addManagedSaiNextHop: " << ip.str();

#if SAI_API_VERSION >= SAI_VERSION(1, 12, 0)
  std::shared_ptr<SaiSrv6SidList> sidList;
  std::optional<sai_object_id_t> sidListId;
  if (!swNextHop.srv6SegmentList().empty()) {
    CHECK(srv6SidList)
        << "srv6SidList must be provided for next hop with non-empty srv6SegmentList";
    sidList = std::move(srv6SidList);
    sidListId = sidList->adapterKey();
  }
  auto nexthopKey = getAdapterHostKey(swNextHop, sidListId);
#else
  auto nexthopKey = getAdapterHostKey(swNextHop);
#endif

  if (auto ipNextHopKey =
          std::get_if<typename SaiIpNextHopTraits::AdapterHostKey>(
              &nexthopKey)) {
    auto [entry, emplaced] = managedIpNextHops_.refOrEmplace(
        *ipNextHopKey,
        this,
        SaiNeighborTraits::NeighborEntry{
            switchId, std::get<0>(*ipNextHopKey).value(), ip},
        *ipNextHopKey,
        swNextHop.disableTTLDecrement());

    if (emplaced) {
      SaiObjectEventPublisher::getInstance()
          ->get<SaiNeighborTraits>()
          .subscribe(entry);
    }
    entry->setDisableTTLDecrement(swNextHop.disableTTLDecrement());

    return entry;
  } else if (
      auto mplsNextHopKey =
          std::get_if<typename SaiMplsNextHopTraits::AdapterHostKey>(
              &nexthopKey)) {
    auto [entry, emplaced] = managedMplsNextHops_.refOrEmplace(
        *mplsNextHopKey,
        this,
        SaiNeighborTraits::NeighborEntry{
            switchId, std::get<0>(*mplsNextHopKey).value(), ip},
        *mplsNextHopKey,
        swNextHop.disableTTLDecrement());

    if (emplaced) {
      SaiObjectEventPublisher::getInstance()
          ->get<SaiNeighborTraits>()
          .subscribe(entry);
    }
    entry->setDisableTTLDecrement(swNextHop.disableTTLDecrement());

    return entry;
  }
#if SAI_API_VERSION >= SAI_VERSION(1, 12, 0)
  else if (
      auto srv6NextHopKey =
          std::get_if<typename SaiSrv6SidlistNextHopTraits::AdapterHostKey>(
              &nexthopKey)) {
    auto [entry, emplaced] = managedSrv6NextHops_.refOrEmplace(
        *srv6NextHopKey,
        this,
        SaiNeighborTraits::NeighborEntry{
            switchId, std::get<0>(*srv6NextHopKey).value(), ip},
        *srv6NextHopKey,
        swNextHop.disableTTLDecrement());

    entry->setDisableTTLDecrement(swNextHop.disableTTLDecrement());
    CHECK(emplaced) << "SRv6 managed next hop must always be emplaced";
    CHECK(sidList) << "SRv6 managed next hop must have a SID list";

    SaiObjectEventPublisher::getInstance()->get<SaiNeighborTraits>().subscribe(
        entry);

    // After subscribe, the SAI next hop should be created
    CHECK(entry->getSaiObject())
        << "SRv6 managed next hop must have underlying SAI object";

    // Set NextHopId in sidList before inserting into srv6Manager
    SaiSrv6SidListTraits::Attributes::NextHopId nextHopIdAttr{
        entry->getSaiObject()->adapterKey()};
    sidList->setOptionalAttribute(std::move(nextHopIdAttr));

    auto srv6SidListHandle =
        managerTable_->srv6Manager().insertSrv6SidList(std::move(sidList));
    entry->setSrv6SidListHandle(std::move(srv6SidListHandle));

    return entry;
  }
#endif

  throw FbossError("next hop key not found for a given next hop subscriber");
}

template <typename NextHopTraits>
std::shared_ptr<SaiObject<NextHopTraits>> SaiNextHopManager::createSaiObject(
    typename NextHopTraits::AdapterHostKey adapterHostKey,
    typename NextHopTraits::CreateAttributes attributes) {
  auto& store = saiStore_->get<NextHopTraits>();
  return store.setObject(adapterHostKey, attributes);
}

std::string SaiNextHopManager::listManagedObjects() const {
  std::string output;
  for (auto entry : managedIpNextHops_) {
    auto entryPtr = entry.second.lock();
    if (!entryPtr) {
      continue;
    }
    output += entryPtr->toString();
    output += "\n";
  }
  for (auto entry : managedMplsNextHops_) {
    auto entryPtr = entry.second.lock();
    if (!entryPtr) {
      continue;
    }
    output += entryPtr->toString();
    output += "\n";
  }
#if SAI_API_VERSION >= SAI_VERSION(1, 12, 0)
  for (auto entry : managedSrv6NextHops_) {
    auto entryPtr = entry.second.lock();
    if (!entryPtr) {
      continue;
    }
    output += entryPtr->toString();
    output += "\n";
  }
#endif
  return output;
}

template <typename NextHopTraits>
void ManagedNextHop<NextHopTraits>::createObject(PublishedObjects /*added*/) {
  CHECK(this->allPublishedObjectsAlive()) << "neighbors are not ready";

  std::optional<typename NextHopTraits::Attributes::DisableTtlDecrement>
      disableTtlDecrement{};
  if (manager_->getPlatform()->getAsic()->isSupported(
          HwAsic::Feature::NEXTHOP_TTL_DECREMENT_DISABLE) &&
      disableTTLDecrement_.has_value()) {
    disableTtlDecrement = disableTTLDecrement_.value();
  }

  std::shared_ptr<SaiObject<NextHopTraits>> object{};
  /* when neighbor is created setup next hop */
  if constexpr (std::is_same_v<NextHopTraits, SaiIpNextHopTraits>) {
    object = manager_->createSaiObject<NextHopTraits>(
        key_,
        {SAI_NEXT_HOP_TYPE_IP,
         std::get<typename NextHopTraits::Attributes::RouterInterfaceId>(key_),
         std::get<typename NextHopTraits::Attributes::Ip>(key_),
         std::nullopt});

  } else if constexpr (std::is_same_v<NextHopTraits, SaiMplsNextHopTraits>) {
    object = manager_->createSaiObject<NextHopTraits>(
        key_,
        {SAI_NEXT_HOP_TYPE_MPLS,
         std::get<typename NextHopTraits::Attributes::RouterInterfaceId>(key_),
         std::get<typename NextHopTraits::Attributes::Ip>(key_),
         std::get<typename NextHopTraits::Attributes::LabelStack>(key_),
         std::nullopt});
  }
#if SAI_API_VERSION >= SAI_VERSION(1, 12, 0)
  else if constexpr (std::is_same_v<
                         NextHopTraits,
                         SaiSrv6SidlistNextHopTraits>) {
    object = manager_->createSaiObject<NextHopTraits>(
        key_,
        {SAI_NEXT_HOP_TYPE_SRV6_SIDLIST,
         std::get<typename NextHopTraits::Attributes::RouterInterfaceId>(key_),
         std::get<typename NextHopTraits::Attributes::Ip>(key_),
         std::get<typename NextHopTraits::Attributes::TunnelId>(key_),
         std::get<typename NextHopTraits::Attributes::Srv6SidlistId>(key_),
         std::nullopt});
  }
#endif
  this->setObject(object);

#if SAI_API_VERSION >= SAI_VERSION(1, 12, 0)
  if constexpr (std::is_same_v<NextHopTraits, SaiSrv6SidlistNextHopTraits>) {
    if (srv6SidListHandle_ && srv6SidListHandle_->sidList) {
      SaiSrv6SidListTraits::Attributes::NextHopId nextHopIdAttr{
          this->getObject()->adapterKey()};
      srv6SidListHandle_->sidList->setOptionalAttribute(
          std::move(nextHopIdAttr));
    }
  }
#endif

  XLOG(DBG2) << "ManagedNeighbor::createObject: " << toString();
}

template <typename NextHopTraits>
void ManagedNextHop<NextHopTraits>::clearSrv6SidListNextHopId() {
#if SAI_API_VERSION >= SAI_VERSION(1, 12, 0)
  if constexpr (std::is_same_v<NextHopTraits, SaiSrv6SidlistNextHopTraits>) {
    if (srv6SidListHandle_) {
      CHECK(srv6SidListHandle_->sidList)
          << "SRv6 SID list handle must have a SID list";
      SaiSrv6SidListTraits::Attributes::NextHopId nextHopIdAttr{
          SAI_NULL_OBJECT_ID};
      srv6SidListHandle_->sidList->setOptionalAttribute(
          std::move(nextHopIdAttr));
    }
  }
#endif
}

template <typename NextHopTraits>
std::string ManagedNextHop<NextHopTraits>::toString() const {
  if constexpr (std::is_same_v<NextHopTraits, SaiIpNextHopTraits>) {
    return folly::to<std::string>(
        this->getObject() ? "active " : "inactive ",
        "managed ip nexthop: "
        "ip: ",
        GET_ATTR(IpNextHop, Ip, adapterHostKey()).str(),
        " routerInterfaceId:",
        GET_ATTR(IpNextHop, RouterInterfaceId, adapterHostKey()));
  } else if constexpr (std::is_same_v<NextHopTraits, SaiMplsNextHopTraits>) {
    auto stack = GET_ATTR(MplsNextHop, LabelStack, adapterHostKey());
    std::stringstream stringstream;
    stringstream << "[";
    for (auto label : stack) {
      stringstream << label << ", ";
    }
    stringstream << "]";

    return folly::to<std::string>(
        this->getObject() ? "active " : "inactive ",
        "managed mpls nexthop: "
        "ip: ",
        GET_ATTR(MplsNextHop, Ip, adapterHostKey()).str(),
        " routerInterfaceId:",
        GET_ATTR(MplsNextHop, RouterInterfaceId, adapterHostKey()),
        " labelStack:",
        stringstream.str());
  }
#if SAI_API_VERSION >= SAI_VERSION(1, 12, 0)
  else if constexpr (std::is_same_v<
                         NextHopTraits,
                         SaiSrv6SidlistNextHopTraits>) {
    return folly::to<std::string>(
        this->getObject() ? "active " : "inactive ",
        "managed srv6 nexthop: "
        "ip: ",
        GET_ATTR(Srv6SidlistNextHop, Ip, adapterHostKey()).str(),
        " routerInterfaceId:",
        GET_ATTR(Srv6SidlistNextHop, RouterInterfaceId, adapterHostKey()),
        " tunnelId:",
        GET_ATTR(Srv6SidlistNextHop, TunnelId, adapterHostKey()),
        " srv6SidlistId:",
        GET_ATTR(Srv6SidlistNextHop, Srv6SidlistId, adapterHostKey()));
  }
#endif
}

template <typename NextHopTraits>
void ManagedNextHop<NextHopTraits>::setDisableTTLDecrement(
    std::optional<bool> disableTTLDecrement) {
  disableTTLDecrement_ = disableTTLDecrement;
  if (this->isAlive() &&
      manager_->getPlatform()->getAsic()->isSupported(
          HwAsic::Feature::NEXTHOP_TTL_DECREMENT_DISABLE) &&
      disableTTLDecrement_.has_value()) {
    std::optional<typename NextHopTraits::Attributes::DisableTtlDecrement> attr{
        disableTTLDecrement_.value()};
    this->getObject()->setAttribute(attr);
  }
}

} // namespace facebook::fboss
