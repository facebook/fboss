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
    const ResolvedNextHop& swNextHop) {
  auto interfaceId = swNextHop.intfID().value();
  auto routerInterfaceHandle =
      managerTable_->routerInterfaceManager().getRouterInterfaceHandle(
          interfaceId);
  if (!routerInterfaceHandle) {
    throw FbossError("Missing SAI router interface for ", interfaceId);
  }
  auto rifId = routerInterfaceHandle->adapterKey();
  folly::IPAddress ip = swNextHop.addr();

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

ManagedSaiNextHop SaiNextHopManager::addManagedSaiNextHop(
    const ResolvedNextHop& swNextHop) {
  auto switchId = managerTable_->switchManager().getSwitchSaiId();
  auto nexthopKey = getAdapterHostKey(swNextHop);
  folly::IPAddress ip = swNextHop.addr();

  XLOG(DBG2) << "SaiNextHopManager::addManagedSaiNextHop: " << ip.str();

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

  } else {
    object = manager_->createSaiObject<NextHopTraits>(
        key_,
        {SAI_NEXT_HOP_TYPE_MPLS,
         std::get<typename NextHopTraits::Attributes::RouterInterfaceId>(key_),
         std::get<typename NextHopTraits::Attributes::Ip>(key_),
         std::get<typename NextHopTraits::Attributes::LabelStack>(key_),
         std::nullopt});
  }
  this->setObject(object);

  XLOG(DBG2) << "ManagedNeighbor::createObject: " << toString();
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
  } else {
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
