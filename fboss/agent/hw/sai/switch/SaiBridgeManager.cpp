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

namespace facebook::fboss {

std::shared_ptr<SaiBridgePort> SaiBridgeManager::addBridgePort(
    SaiPortDescriptor portDescriptor,
    PortDescriptorSaiId saiId) {
  // Lazily re-load or create the default bridge if it is missing
  createBridgeHandle();
  auto& store = saiStore_->get<SaiBridgePortTraits>();
  auto saiObjectId = saiId.intID();
  SaiBridgePortTraits::AdapterHostKey k{saiObjectId};
  SaiBridgePortTraits::CreateAttributes attributes{
      SAI_BRIDGE_PORT_TYPE_PORT, saiObjectId, true, fdbLearningMode_};
  return store.setObject(k, attributes, portDescriptor);
}

void SaiBridgeManager::createBridgeHandle() {
  if (UNLIKELY(!bridgeHandle_)) {
    auto& store = saiStore_->get<SaiBridgeTraits>();
    bridgeHandle_ = std::make_unique<SaiBridgeHandle>();
    SwitchSaiId switchId = managerTable_->switchManager().getSwitchSaiId();
    BridgeSaiId default1QBridgeId{
        SaiApiTable::getInstance()->switchApi().getAttribute(
            switchId, SaiSwitchTraits::Attributes::Default1QBridgeId{})};
    bridgeHandle_->bridge = store.loadObjectOwnedByAdapter(
        SaiBridgeTraits::AdapterKey{default1QBridgeId});
    CHECK(bridgeHandle_->bridge);
  }
}
SaiBridgeManager::SaiBridgeManager(
    SaiStore* saiStore,
    SaiManagerTable* managerTable,
    const SaiPlatform* platform)
    : saiStore_(saiStore), managerTable_(managerTable), platform_(platform) {}

sai_bridge_port_fdb_learning_mode_t SaiBridgeManager::getFdbLearningMode(
    cfg::L2LearningMode l2LearningMode) const {
  sai_bridge_port_fdb_learning_mode_t fdbLearningMode;
  switch (l2LearningMode) {
    case cfg::L2LearningMode::HARDWARE:
      fdbLearningMode = SAI_BRIDGE_PORT_FDB_LEARNING_MODE_HW;
      break;
    case cfg::L2LearningMode::SOFTWARE:
      if (platform_->getAsic()->isSupported(
              HwAsic::Feature::PENDING_L2_ENTRY)) {
        // entry learnt and but not forwarding packets in hw (in pending state),
        // fboss need to validate entry in fdb callback
        fdbLearningMode = SAI_BRIDGE_PORT_FDB_LEARNING_MODE_FDB_NOTIFICATION;
      } else {
        // entry learnt and forwarding packets in hw (in validated state),
        // fboss may update fdb entry in fdb callback if necessary
        fdbLearningMode = SAI_BRIDGE_PORT_FDB_LEARNING_MODE_HW;
      }
      break;
  }
  return fdbLearningMode;
}

cfg::L2LearningMode SaiBridgeManager::getL2LearningMode() const {
  return mode_;
}

void SaiBridgeManager::setL2LearningMode(
    std::optional<cfg::L2LearningMode> l2LearningMode) {
  if (l2LearningMode) {
    mode_ = l2LearningMode.value();
    fdbLearningMode_ = getFdbLearningMode(l2LearningMode.value());
  }
  XLOG(DBG2) << "FDB learning mode set to "
             << (getL2LearningMode() == cfg::L2LearningMode::HARDWARE
                     ? "hardware"
                     : "software");
}
} // namespace facebook::fboss
