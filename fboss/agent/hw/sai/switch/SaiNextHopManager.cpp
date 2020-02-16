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
      SAI_NEXT_HOP_TYPE_IP, routerInterfaceId, ip};
  auto& store = SaiStore::getInstance()->get<SaiIpNextHopTraits>();
  return store.setObject(k, attributes);
}

SaiNextHopManager::SaiNextHopManager(
    SaiManagerTable* managerTable,
    const SaiPlatform* platform)
    : managerTable_(managerTable), platform_(platform) {}

SaiNextHop SaiNextHopManager::refOrEmplace(const ResolvedNextHop& swNextHop) {
  auto interfaceId = swNextHop.intfID().value();
  auto routerInterfaceHandle =
      managerTable_->routerInterfaceManager().getRouterInterfaceHandle(
          interfaceId);
  if (!routerInterfaceHandle) {
    throw FbossError("Missing SAI router interface for ", interfaceId);
  }
  auto rifId = routerInterfaceHandle->routerInterface->adapterKey();
  folly::IPAddress ip = swNextHop.addr();
  if (!swNextHop.labelForwardingAction() ||
      LabelForwardingAction::LabelForwardingType::PHP ==
          swNextHop.labelForwardingAction()->type()) {
    return refOrEmplace(rifId, ip);
  }
  CHECK(
      swNextHop.labelForwardingAction()->type() !=
      LabelForwardingAction::LabelForwardingType::POP_AND_LOOKUP)
      << "action pop is not supported";
  if (LabelForwardingAction::LabelForwardingType::SWAP ==
      swNextHop.labelForwardingAction()->type()) {
    return refOrEmplace(
        rifId, ip, swNextHop.labelForwardingAction()->swapWith().value());
  }
  return refOrEmplace(
      rifId, ip, swNextHop.labelForwardingAction()->pushStack().value());
}

SaiNextHop SaiNextHopManager::refOrEmplace(
    SaiRouterInterfaceTraits::AdapterKey interface,
    const folly::IPAddress& ip) {
  SaiIpNextHopTraits::AdapterHostKey k{interface, ip};
  auto& store = SaiStore::getInstance()->get<SaiIpNextHopTraits>();
  return store.setObject(
      k,
      SaiIpNextHopTraits::CreateAttributes{
          SAI_NEXT_HOP_TYPE_IP, interface, ip});
}

SaiNextHop SaiNextHopManager::refOrEmplace(
    SaiRouterInterfaceTraits::AdapterKey interface,
    const folly::IPAddress& ip,
    LabelForwardingAction::Label label) {
  return refOrEmplace(interface, ip, LabelForwardingAction::LabelStack{label});
}

SaiNextHop SaiNextHopManager::refOrEmplace(
    SaiRouterInterfaceTraits::AdapterKey interface,
    const folly::IPAddress& ip,
    const LabelForwardingAction::LabelStack& stack) {
  std::vector<sai_uint32_t> labels;
  for (auto label : stack) {
    labels.push_back(label);
  }
  SaiMplsNextHopTraits::AdapterHostKey k{interface, ip, std::move(labels)};
  auto& store = SaiStore::getInstance()->get<SaiMplsNextHopTraits>();
  return store.setObject(
      k,
      SaiMplsNextHopTraits::CreateAttributes{
          SAI_NEXT_HOP_TYPE_MPLS, interface, ip, labels});
}

} // namespace facebook::fboss
