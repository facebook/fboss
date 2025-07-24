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
    case cfg::SwitchingMode::PER_PACKET_RANDOM:
      return SAI_ARS_MODE_PER_PACKET_RANDOM;
  }

  // should return in one of the cases
  throw FbossError("Unsupported flowlet switching mode");
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

SaiArsHandle* SaiArsManager::getArsHandle() const {
  return arsHandle_.get();
}
#endif

} // namespace facebook::fboss
