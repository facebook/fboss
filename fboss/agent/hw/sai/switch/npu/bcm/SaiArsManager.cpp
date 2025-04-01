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

#include "fboss/agent/hw/sai/store/SaiStore.h"
#include "fboss/agent/hw/sai/switch/SaiManagerTable.h"
#include "fboss/agent/hw/sai/switch/SaiSwitchManager.h"

#include "fboss/agent/platforms/sai/SaiPlatform.h"

namespace facebook::fboss {

#if SAI_API_VERSION >= SAI_VERSION(1, 14, 0)
void SaiArsManager::addArs(
    const std::shared_ptr<FlowletSwitchingConfig>& flowletSwitchConfig) {
  SaiArsTraits::CreateAttributes attributes{
      cfgSwitchingModeToSai(flowletSwitchConfig->getSwitchingMode()),
      flowletSwitchConfig->getInactivityIntervalUsecs(),
      flowletSwitchConfig->getFlowletTableSize()};
  auto& store = saiStore_->get<SaiArsTraits>();
  arsHandle_->ars = store.setObject(std::monostate{}, attributes);
}

// Use custom attribute SAI_SWITCH_ATTR_ARS_AVAILABLE_FLOWS to query remaining
// entries in flowset table
bool SaiArsManager::isFlowsetTableFull(const ArsSaiId& arsSaiId) {
  // Flowset table is specific to BCM
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
