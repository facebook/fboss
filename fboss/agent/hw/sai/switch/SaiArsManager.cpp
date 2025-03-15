/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/sai/switch/SaiArsManager.h"

#include "fboss/agent/FbossError.h"
#include "fboss/agent/hw/sai/store/SaiStore.h"
#include "fboss/agent/hw/sai/switch/SaiManagerTable.h"
#include "fboss/agent/hw/sai/switch/SaiSwitchManager.h"

#include "fboss/agent/platforms/sai/SaiPlatform.h"

namespace facebook::fboss {

SaiArsManager::SaiArsManager(
    SaiStore* saiStore,
    SaiManagerTable* managerTable,
    const SaiPlatform* platform)
    : saiStore_(saiStore), managerTable_(managerTable), platform_(platform) {
#if SAI_API_VERSION >= SAI_VERSION(1, 14, 0)
  arsHandle_ = std::make_unique<SaiArsHandle>();
#endif
}

#if SAI_API_VERSION >= SAI_VERSION(1, 14, 0)
sai_int32_t SaiArsManager::cfgSwitchingModeToSai(
    cfg::SwitchingMode switchingMode) const {
  switch (switchingMode) {
    case cfg::SwitchingMode::FLOWLET_QUALITY:
      return SAI_ARS_MODE_FLOWLET_QUALITY;
    case cfg::SwitchingMode::PER_PACKET_QUALITY:
      return SAI_ARS_MODE_PER_PACKET_QUALITY;
    case cfg::SwitchingMode::FIXED_ASSIGNMENT:
      return SAI_ARS_MODE_FIXED;
  }

  // should return in one of the cases
  throw FbossError("Unsupported flowlet switching mode");
}

void SaiArsManager::addArs(
    const std::shared_ptr<FlowletSwitchingConfig>& flowletSwitchConfig) {
  SaiArsTraits::CreateAttributes attributes{
      cfgSwitchingModeToSai(flowletSwitchConfig->getSwitchingMode()),
      flowletSwitchConfig->getInactivityIntervalUsecs(),
      flowletSwitchConfig->getFlowletTableSize()};
  auto& store = saiStore_->get<SaiArsTraits>();
  arsHandle_->ars = store.setObject(std::monostate{}, attributes);
}

void SaiArsManager::removeArs(
    const std::shared_ptr<FlowletSwitchingConfig>& flowletSwitchConfig) {
  if (arsHandle_->ars) {
    arsHandle_->ars.reset();
  }
}

void SaiArsManager::changeArs(
    const std::shared_ptr<FlowletSwitchingConfig>& oldFlowletSwitchConfig,
    const std::shared_ptr<FlowletSwitchingConfig>& newFlowletSwitchConfig) {
  addArs(newFlowletSwitchConfig);
}

SaiArsHandle* SaiArsManager::getArsHandle() {
  return arsHandle_.get();
}

// Use custom attribute SAI_SWITCH_ATTR_ARS_AVAILABLE_FLOWS to query remaining
// entries in flowset table
bool SaiArsManager::isFlowsetTableFull(ArsSaiId arsSaiId) {
  if (!FLAGS_flowletSwitchingEnable ||
      !platform_->getAsic()->isSupported(HwAsic::Feature::FLOWLET)) {
    return false;
  }

  auto switchId = managerTable_->switchManager().getSwitchSaiId();
  auto arsAvailableFlows = SaiApiTable::getInstance()->switchApi().getAttribute(
      switchId, SaiSwitchTraits::Attributes::ArsAvailableFlows{});

  // get required flowlet table entries per nexthop
  auto flowletTableSize = SaiApiTable::getInstance()->arsApi().getAttribute(
      arsSaiId, SaiArsTraits::Attributes::MaxFlows());
  if (arsAvailableFlows >= flowletTableSize) {
    return false;
  } else {
    XLOG(DBG2) << "Flowset table full, available entries: "
               << arsAvailableFlows;
    return true;
  }

  return false;
}
#endif

} // namespace facebook::fboss
