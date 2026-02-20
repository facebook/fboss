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

#if SAI_API_VERSION >= SAI_VERSION(1, 14, 0)
void SaiArsManager::addArs(
    const std::shared_ptr<FlowletSwitchingConfig>& flowletSwitchConfig) {
  SaiArsTraits::CreateAttributes attributes{
      SaiArsTraits::Attributes::Mode{
          cfgSwitchingModeToSai(flowletSwitchConfig->getSwitchingMode())},
      SaiArsTraits::Attributes::IdleTime{
          flowletSwitchConfig->getInactivityIntervalUsecs()},
      SaiArsTraits::Attributes::MaxFlows{
          flowletSwitchConfig->getFlowletTableSize()},
      std::nullopt, // PrimaryPathQualityThreshold
      SaiArsTraits::Attributes::AlternatePathCost{0},
      SaiArsTraits::Attributes::AlternatePathBias{0},
      std::nullopt}; // NextHopGroupType

  auto hostKey = getAdapterHostKey(attributes);

  auto& store = saiStore_->get<SaiArsTraits>();
  arsHandle_->ars = store.setObject(hostKey, attributes);

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
    SaiArsTraits::CreateAttributes alternateMemArsAttributes{
        SaiArsTraits::Attributes::Mode{
            cfgSwitchingModeToSai(flowletSwitchConfig->getSwitchingMode())},
        SaiArsTraits::Attributes::IdleTime{
            flowletSwitchConfig->getInactivityIntervalUsecs()},
        SaiArsTraits::Attributes::MaxFlows{
            flowletSwitchConfig->getFlowletTableSize()},
        primaryPathQualityThreshold,
        alternatePathCost,
        alternatePathBias,
        std::nullopt}; // NextHopGroupType
    alternateMemberArsHandle_->ars = store.setObject(
        getAdapterHostKey(alternateMemArsAttributes),
        alternateMemArsAttributes);
  }
}

#endif

} // namespace facebook::fboss
