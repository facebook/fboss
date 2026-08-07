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

#if defined(BRCM_SAI_SDK_GTE_14_0)
#include <experimental/saiarsextensions.h>
#endif

namespace facebook::fboss {

#if SAI_API_VERSION >= SAI_VERSION(1, 14, 0)
void SaiArsManager::addArs(
    const std::shared_ptr<FlowletSwitchingConfig>& flowletSwitchConfig) {
  auto switchingMode = flowletSwitchConfig->getSwitchingMode();
  auto idleTime = flowletSwitchConfig->getInactivityIntervalUsecs();
  auto maxFlows = flowletSwitchConfig->getFlowletTableSize();

  std::optional<SaiArsTraits::Attributes::AlternatePathCost>
      alternatePathCostForArs = std::nullopt;
  std::optional<SaiArsTraits::Attributes::AlternatePathBias>
      alternatePathBiasForArs = std::nullopt;
#if SAI_API_VERSION >= SAI_VERSION(1, 16, 0)
  if (platform_->getAsic()->isSupported(
          HwAsic::Feature::ARS_ALTERNATE_MEMBERS)) {
    // Need to set default values as these attributes are part of adapter key
    alternatePathCostForArs = SaiArsTraits::Attributes::AlternatePathCost{0};
    alternatePathBiasForArs = SaiArsTraits::Attributes::AlternatePathBias{0};
  }
#endif
  std::optional<SaiArsTraits::Attributes::NextHopGroupType> nextHopGroupType =
      std::nullopt;
#if defined(BRCM_SAI_SDK_GTE_14_0) && defined(BRCM_SAI_SDK_XGS)
  nextHopGroupType = SaiArsTraits::Attributes::NextHopGroupType{
      SAI_ARS_NEXT_HOP_GROUP_TYPE_REGULAR};
#endif

  setArsObject(
      arsHandle_.get(),
      makeArsAttributes(
          switchingMode,
          idleTime,
          maxFlows,
          std::nullopt,
          alternatePathCostForArs,
          alternatePathBiasForArs,
          nextHopGroupType,
          getSourcePortPrune(flowletSwitchConfig)));

  auto cost = flowletSwitchConfig->getAlternatePathCost();
  auto bias = flowletSwitchConfig->getAlternatePathBias();
  if (cost.has_value() && bias.has_value()) {
    std::optional<SaiArsTraits::Attributes::PrimaryPathQualityThreshold>
        primaryPathQualityThreshold = std::nullopt;
    if (auto threshold =
            flowletSwitchConfig->getPrimaryPathQualityThreshold()) {
      primaryPathQualityThreshold =
          SaiArsTraits::Attributes::PrimaryPathQualityThreshold{
              static_cast<sai_uint32_t>(*threshold)};
    }
    setArsObject(
        alternateMemberArsHandle_.get(),
        makeArsAttributes(
            switchingMode,
            idleTime,
            maxFlows,
            primaryPathQualityThreshold,
            SaiArsTraits::Attributes::AlternatePathCost{
                static_cast<sai_uint32_t>(*cost)},
            SaiArsTraits::Attributes::AlternatePathBias{
                static_cast<sai_uint32_t>(*bias)},
            nextHopGroupType,
            getSourcePortPrune(flowletSwitchConfig)));
  }

#if SAI_API_VERSION >= SAI_VERSION(1, 16, 0)
  auto standbySwitchingMode = flowletSwitchConfig->getStandbySwitchingMode();
  if (standbySwitchingMode.has_value()) {
    // ThriftConfigApplier guarantees the standby idle time and table size are
    // set alongside the mode, and that the mode differs from the primary's.
    setArsObject(
        standbyArsHandle_.get(),
        makeArsAttributes(
            *standbySwitchingMode,
            *flowletSwitchConfig->getStandbyInactivityIntervalUsecs(),
            *flowletSwitchConfig->getStandbyFlowletTableSize(),
            std::nullopt,
            alternatePathCostForArs,
            alternatePathBiasForArs,
            nextHopGroupType,
            getSourcePortPrune(flowletSwitchConfig)));
  }
#endif

#if defined(BRCM_SAI_SDK_GTE_14_0) && defined(BRCM_SAI_SDK_XGS)
  if (platform_->getAsic()->isSupported(HwAsic::Feature::VIRTUAL_ARS_GROUP)) {
    setArsObject(
        virtualArsGroupHandle_.get(),
        makeArsAttributes(
            switchingMode,
            idleTime,
            maxFlows,
            std::nullopt,
            std::nullopt,
            std::nullopt,
            SaiArsTraits::Attributes::NextHopGroupType{
                SAI_ARS_NEXT_HOP_GROUP_TYPE_VIRTUAL},
            getSourcePortPrune(flowletSwitchConfig)));
  }
#endif
}
#endif

} // namespace facebook::fboss
