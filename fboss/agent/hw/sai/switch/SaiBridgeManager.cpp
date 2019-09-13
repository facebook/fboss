/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/sai/switch/SaiBridgeManager.h"

#include "fboss/agent/FbossError.h"
#include "fboss/agent/hw/sai/store/SaiStore.h"
#include "fboss/agent/hw/sai/switch/SaiManagerTable.h"
#include "fboss/agent/hw/sai/switch/SaiSwitchManager.h"

#include <folly/logging/xlog.h>

namespace facebook {
namespace fboss {

std::shared_ptr<SaiBridgePort> SaiBridgeManager::addBridgePort(
    PortSaiId portId) {
  // Lazily re-load or create the default bridge if it is missing
  if (UNLIKELY(!bridgeHandle_)) {
    auto& store = SaiStore::getInstance()->get<SaiBridgeTraits>();
    SaiBridgeTraits::CreateAttributes attributes{SAI_BRIDGE_TYPE_1Q};
    bridgeHandle_ = std::make_unique<SaiBridgeHandle>();
    bridgeHandle_->bridge = store.setObject(std::monostate{}, attributes);
  }
  auto& store = SaiStore::getInstance()->get<SaiBridgePortTraits>();
  SaiBridgePortTraits::AdapterHostKey k{portId};
  SaiBridgePortTraits::CreateAttributes attributes{SAI_BRIDGE_PORT_TYPE_PORT,
                                                   portId};
  return store.setObject(k, attributes);
}

SaiBridgeManager::SaiBridgeManager(
    SaiManagerTable* managerTable,
    const SaiPlatform* platform)
    : managerTable_(managerTable), platform_(platform) {}

} // namespace fboss
} // namespace facebook
