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
#include "fboss/agent/hw/sai/switch/SaiSwitchManager.h"

#include <folly/logging/xlog.h>

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

} // namespace facebook::fboss
