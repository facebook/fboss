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
#include "fboss/agent/hw/switch_asics/HwAsic.h"

#include "fboss/agent/platforms/sai/SaiPlatform.h"

#include <folly/logging/xlog.h>

namespace facebook::fboss {

std::shared_ptr<SaiBridgePort> SaiBridgeManager::addBridgePort(
    SaiPortDescriptor portDescriptor,
    PortDescriptorSaiId saiId) {
  // Lazily re-load or create the default bridge if it is missing
  if (UNLIKELY(!bridgeHandle_)) {
    auto& store = SaiStore::getInstance()->get<SaiBridgeTraits>();
    bridgeHandle_ = std::make_unique<SaiBridgeHandle>();
    SwitchSaiId switchId = managerTable_->switchManager().getSwitchSaiId();
    BridgeSaiId default1QBridgeId{
        SaiApiTable::getInstance()->switchApi().getAttribute(
            switchId, SaiSwitchTraits::Attributes::Default1QBridgeId{})};
    bridgeHandle_->bridge = store.loadObjectOwnedByAdapter(
        SaiBridgeTraits::AdapterKey{default1QBridgeId});
    CHECK(bridgeHandle_->bridge);
  }
  auto& store = SaiStore::getInstance()->get<SaiBridgePortTraits>();
  auto saiObjectId =
      saiId.isPhysicalPort() ? saiId.phyPortID() : saiId.aggPortID();
  SaiBridgePortTraits::AdapterHostKey k{saiObjectId};
  SaiBridgePortTraits::CreateAttributes attributes{
      SAI_BRIDGE_PORT_TYPE_PORT,
      saiObjectId,
      true,
      SAI_BRIDGE_PORT_FDB_LEARNING_MODE_HW};
  return store.setObject(k, attributes, portDescriptor);
}

SaiBridgeManager::SaiBridgeManager(
    SaiManagerTable* managerTable,
    const SaiPlatform* platform)
    : managerTable_(managerTable), platform_(platform) {
  auto& store = SaiStore::getInstance()->get<SaiBridgeTraits>();
  store.setObjectOwnedByAdapter(true);
}

sai_bridge_port_fdb_learning_mode_t SaiBridgeManager::getFdbLearningMode(
    cfg::L2LearningMode l2LearningMode) const {
  sai_bridge_port_fdb_learning_mode_t fdbLearningMode;
  switch (l2LearningMode) {
    case cfg::L2LearningMode::HARDWARE:
      fdbLearningMode = SAI_BRIDGE_PORT_FDB_LEARNING_MODE_HW;
      break;
    case cfg::L2LearningMode::SOFTWARE:
      fdbLearningMode = SAI_BRIDGE_PORT_FDB_LEARNING_MODE_FDB_NOTIFICATION;
      break;
  }
  return fdbLearningMode;
}

} // namespace facebook::fboss
