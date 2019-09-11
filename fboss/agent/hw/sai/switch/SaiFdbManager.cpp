/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/sai/switch/SaiFdbManager.h"
#include "fboss/agent/hw/sai/store/SaiStore.h"
#include "fboss/agent/hw/sai/switch/SaiBridgeManager.h"
#include "fboss/agent/hw/sai/switch/SaiManagerTable.h"
#include "fboss/agent/hw/sai/switch/SaiPortManager.h"
#include "fboss/agent/hw/sai/switch/SaiRouterInterfaceManager.h"
#include "fboss/agent/hw/sai/switch/SaiSwitchManager.h"
#include "fboss/agent/hw/sai/switch/SaiVlanManager.h"

#include <memory>
#include <tuple>

namespace facebook {
namespace fboss {

SaiFdbManager::SaiFdbManager(
    SaiApiTable* apiTable,
    SaiManagerTable* managerTable,
    const SaiPlatform* platform)
    : apiTable_(apiTable), managerTable_(managerTable), platform_(platform) {}

std::shared_ptr<SaiFdbEntry> SaiFdbManager::addFdbEntry(
    const InterfaceID& intfId,
    const folly::MacAddress& mac,
    const PortDescriptor& portDesc) {
  XLOG(INFO) << "addFdb " << mac;
  SaiRouterInterfaceHandle* routerInterfaceHandle =
      managerTable_->routerInterfaceManager().getRouterInterfaceHandle(intfId);
  if (!routerInterfaceHandle) {
    throw FbossError(
        "Attempted to add non-existent interface to Fdb: ", intfId);
  }
  auto rif = routerInterfaceHandle->routerInterface;
  auto vlanId = GET_ATTR(RouterInterface, VlanId, rif->attributes());
  // TODO(srikrishnagopu): Can it be an AGG Port ?
  auto portId = portDesc.phyPortID();
  auto portHandle = managerTable_->portManager().getPortHandle(portId);
  if (!portHandle) {
    throw FbossError("Attempted to add non-existent port to Fdb: ", portId);
  }
  auto switchId = managerTable_->switchManager().getSwitchSaiId();
  auto bridgePortId = portHandle->bridgePort->adapterKey();
  SaiFdbTraits::FdbEntry entry{switchId, vlanId, mac};
  SaiFdbTraits::CreateAttributes attributes{SAI_FDB_ENTRY_TYPE_STATIC,
                                            bridgePortId};
  auto& store = SaiStore::getInstance()->get<SaiFdbTraits>();
  return store.setObject(entry, attributes);
}

} // namespace fboss
} // namespace facebook
