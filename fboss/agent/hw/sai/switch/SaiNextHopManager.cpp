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

#include <folly/logging/xlog.h>

#include "fboss/agent/state/RouteNextHop.h"

namespace facebook::fboss {

std::shared_ptr<SaiIpNextHop> SaiNextHopManager::addNextHop(
    RouterInterfaceSaiId routerInterfaceId,
    const folly::IPAddress& ip) {
  SaiIpNextHopTraits::AdapterHostKey k{routerInterfaceId, ip};
  SaiIpNextHopTraits::CreateAttributes attributes{
      SAI_NEXT_HOP_TYPE_IP, routerInterfaceId, ip, std::nullopt};
  auto& store = SaiStore::getInstance()->get<SaiIpNextHopTraits>();
  return store.setObject(k, attributes);
}

SaiNextHopManager::SaiNextHopManager(
    SaiManagerTable* managerTable,
    const SaiPlatform* platform)
    : managerTable_(managerTable), platform_(platform) {}

SaiNextHop SaiNextHopManager::refOrEmplace(const ResolvedNextHop& swNextHop) {
  auto nexthopKey = getAdapterHostKey(swNextHop);

  if (auto ipNextHopKey =
          std::get_if<typename SaiIpNextHopTraits::AdapterHostKey>(
              &nexthopKey)) {
    return SaiStore::getInstance()->get<SaiIpNextHopTraits>().setObject(
        *ipNextHopKey,
        SaiIpNextHopTraits::CreateAttributes{SAI_NEXT_HOP_TYPE_IP,
                                             std::get<0>(*ipNextHopKey),
                                             std::get<1>(*ipNextHopKey),
                                             std::nullopt});
  } else if (
      auto mplsNextHopKey =
          std::get_if<typename SaiMplsNextHopTraits::AdapterHostKey>(
              &nexthopKey)) {
    return SaiStore::getInstance()->get<SaiMplsNextHopTraits>().setObject(
        *mplsNextHopKey,
        SaiMplsNextHopTraits::CreateAttributes{SAI_NEXT_HOP_TYPE_MPLS,
                                               std::get<0>(*mplsNextHopKey),
                                               std::get<1>(*mplsNextHopKey),
                                               std::get<2>(*mplsNextHopKey),
                                               std::nullopt});
  }

  throw FbossError("next hop key not found for a given next hop");
}

SaiNextHopTraits::AdapterHostKey SaiNextHopManager::getAdapterHostKey(
    const ResolvedNextHop& swNextHop) {
  auto interfaceId = swNextHop.intfID().value();
  auto routerInterfaceHandle =
      managerTable_->routerInterfaceManager().getRouterInterfaceHandle(
          interfaceId);
  if (!routerInterfaceHandle) {
    throw FbossError("Missing SAI router interface for ", interfaceId);
  }
  auto rifId = routerInterfaceHandle->routerInterface->adapterKey();
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

ManagedSaiNextHop SaiNextHopManager::refOrEmplaceNextHop(
    const ResolvedNextHop& swNextHop) {
  auto switchId = managerTable_->switchManager().getSwitchSaiId();
  auto nexthopKey = getAdapterHostKey(swNextHop);
  folly::IPAddress ip = swNextHop.addr();

  if (auto ipNextHopKey =
          std::get_if<typename SaiIpNextHopTraits::AdapterHostKey>(
              &nexthopKey)) {
    auto [entry, emplaced] = managedIpNextHops_.refOrEmplace(
        *ipNextHopKey,
        SaiNeighborTraits::NeighborEntry{
            switchId, std::get<0>(*ipNextHopKey).value(), ip},
        *ipNextHopKey);

    if (emplaced) {
      SaiObjectEventPublisher::getInstance()
          ->get<SaiNeighborTraits>()
          .subscribe(entry);
    }

    return entry;
  } else if (
      auto mplsNextHopKey =
          std::get_if<typename SaiMplsNextHopTraits::AdapterHostKey>(
              &nexthopKey)) {
    auto [entry, emplaced] = managedMplsNextHops_.refOrEmplace(
        *mplsNextHopKey,
        SaiNeighborTraits::NeighborEntry{
            switchId, std::get<0>(*mplsNextHopKey).value(), ip},
        *mplsNextHopKey);

    if (emplaced) {
      SaiObjectEventPublisher::getInstance()
          ->get<SaiNeighborTraits>()
          .subscribe(entry);
    }

    return entry;
  }

  throw FbossError("next hop key not found for a given next hop subscriber");
}

} // namespace facebook::fboss
