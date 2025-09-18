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
  std::optional<SaiArsTraits::Attributes::AlternatePathCost>
      alternatePathCostForArs = std::nullopt;
  std::optional<SaiArsTraits::Attributes::AlternatePathBias>
      alternatePathBiasForArs = std::nullopt;
#if SAI_API_VERSION >= SAI_VERSION(1, 16, 0)
  // Need to set default values as these attributes are part of host adapter key
  alternatePathCostForArs = SaiArsTraits::Attributes::AlternatePathCost{0};
  alternatePathBiasForArs = SaiArsTraits::Attributes::AlternatePathBias{0};
#endif
  SaiArsTraits::CreateAttributes attributes{
      SaiArsTraits::Attributes::Mode{
          cfgSwitchingModeToSai(flowletSwitchConfig->getSwitchingMode())},
      SaiArsTraits::Attributes::IdleTime{
          flowletSwitchConfig->getInactivityIntervalUsecs()},
      SaiArsTraits::Attributes::MaxFlows{
          flowletSwitchConfig->getFlowletTableSize()},
      std::nullopt, // PrimaryPathQualityThreshold
      alternatePathCostForArs,
      alternatePathBiasForArs};

  auto& store = saiStore_->get<SaiArsTraits>();
  arsHandle_->ars = store.setObject(getAdapterHostKey(attributes), attributes);

  bool needAlternateArs =
      flowletSwitchConfig->getAlternatePathCost().has_value() &&
      flowletSwitchConfig->getAlternatePathBias().has_value();

  if (needAlternateArs) {
    std::optional<SaiArsTraits::Attributes::AlternatePathCost>
        alternatePathCost = std::nullopt;
    std::optional<SaiArsTraits::Attributes::AlternatePathBias>
        alternatePathBias = std::nullopt;
    std::optional<SaiArsTraits::Attributes::PrimaryPathQualityThreshold>
        primaryPathQualityThreshold = std::nullopt;
    if (auto cost = flowletSwitchConfig->getAlternatePathCost()) {
      alternatePathCost = SaiArsTraits::Attributes::AlternatePathCost{
          static_cast<sai_uint32_t>(*cost)};
    }

    if (auto bias = flowletSwitchConfig->getAlternatePathBias()) {
      alternatePathBias = SaiArsTraits::Attributes::AlternatePathBias{
          static_cast<sai_uint32_t>(*bias)};
    }

    if (auto threshold =
            flowletSwitchConfig->getPrimaryPathQualityThreshold()) {
      primaryPathQualityThreshold =
          SaiArsTraits::Attributes::PrimaryPathQualityThreshold{
              static_cast<sai_uint32_t>(*threshold)};
    }
    SaiArsTraits::CreateAttributes alternateMemAttributes{
        SaiArsTraits::Attributes::Mode{
            cfgSwitchingModeToSai(flowletSwitchConfig->getSwitchingMode())},
        SaiArsTraits::Attributes::IdleTime{
            flowletSwitchConfig->getInactivityIntervalUsecs()},
        SaiArsTraits::Attributes::MaxFlows{
            flowletSwitchConfig->getFlowletTableSize()},
        primaryPathQualityThreshold,
        alternatePathCost,
        alternatePathBias};
    alternateMemberArsHandle_->ars = store.setObject(
        getAdapterHostKey(alternateMemAttributes), alternateMemAttributes);
  }
}

// Use custom attribute SAI_SWITCH_ATTR_ARS_AVAILABLE_FLOWS to query remaining
// entries in flowset table
bool SaiArsManager::isFlowsetTableFull(const ArsSaiId& arsSaiId) {
  // Flowset table is specific to BCM
  if (!FLAGS_flowletSwitchingEnable ||
      !platform_->getAsic()->isSupported(HwAsic::Feature::ARS)) {
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
