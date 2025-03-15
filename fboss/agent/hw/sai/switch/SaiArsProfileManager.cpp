/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/sai/switch/SaiArsProfileManager.h"

#include "fboss/agent/hw/sai/store/SaiStore.h"
#include "fboss/agent/hw/sai/switch/SaiManagerTable.h"

#include "fboss/agent/platforms/sai/SaiPlatform.h"

namespace facebook::fboss {

SaiArsProfileManager::SaiArsProfileManager(
    SaiStore* saiStore,
    SaiManagerTable* managerTable,
    const SaiPlatform* platform)
    : saiStore_(saiStore), managerTable_(managerTable), platform_(platform) {
#if SAI_API_VERSION >= SAI_VERSION(1, 14, 0)
  arsProfileHandle_ = std::make_unique<SaiArsProfileHandle>();
#endif
}

#if SAI_API_VERSION >= SAI_VERSION(1, 14, 0)

SaiArsProfileTraits::CreateAttributes SaiArsProfileManager::createAttributes(
    const std::shared_ptr<FlowletSwitchingConfig>& flowletSwitchConfig) {
  auto samplingInterval = flowletSwitchConfig->getDynamicSampleRate();
  sai_uint32_t randomSeed = kArsRandomSeed;
  auto portLoadPastWeight = flowletSwitchConfig->getDynamicEgressLoadExponent();
  auto portLoadFutureWeight = flowletSwitchConfig->getDynamicQueueExponent();
  auto portLoadExponent =
      flowletSwitchConfig->getDynamicPhysicalQueueExponent();
  auto loadPastMinVal =
      flowletSwitchConfig->getDynamicEgressMinThresholdBytes();
  auto loadPastMaxVal =
      flowletSwitchConfig->getDynamicEgressMaxThresholdBytes();
  auto loadFutureMinVal =
      flowletSwitchConfig->getDynamicQueueMinThresholdBytes();
  auto loadFutureMaxVal =
      flowletSwitchConfig->getDynamicQueueMaxThresholdBytes();
  // this is per ITM and should be half of newDynamicQueueMinThresholdBytes
  // above (as we have 2 ITMs)
  auto loadCurrentMinVal =
      flowletSwitchConfig->getDynamicQueueMinThresholdBytes() >> 1;
  auto loadCurrentMaxVal =
      flowletSwitchConfig->getDynamicQueueMaxThresholdBytes() >> 1;
  SaiArsProfileTraits::CreateAttributes attributes{
      SAI_ARS_PROFILE_ALGO_EWMA,
      samplingInterval,
      randomSeed,
      false, // EnableIPv4
      false, // EnableIPv6
      true,
      portLoadPastWeight,
      loadPastMinVal,
      loadPastMaxVal,
      true,
      portLoadFutureWeight,
      loadFutureMinVal,
      loadFutureMaxVal,
      true,
      portLoadExponent,
      loadCurrentMinVal,
      loadCurrentMaxVal};

  return attributes;
}

void SaiArsProfileManager::addArsProfile(
    const std::shared_ptr<FlowletSwitchingConfig>& flowletSwitchConfig) {
  auto attributes = createAttributes(flowletSwitchConfig);
  auto& store = saiStore_->get<SaiArsProfileTraits>();
  arsProfileHandle_->arsProfile = store.setObject(std::monostate{}, attributes);
}

void SaiArsProfileManager::removeArsProfile(
    const std::shared_ptr<FlowletSwitchingConfig>& flowletSwitchConfig) {
  if (arsProfileHandle_->arsProfile) {
    arsProfileHandle_->arsProfile.reset();
  }
}

void SaiArsProfileManager::changeArsProfile(
    const std::shared_ptr<FlowletSwitchingConfig>& oldFlowletSwitchConfig,
    const std::shared_ptr<FlowletSwitchingConfig>& newFlowletSwitchConfig) {
  addArsProfile(newFlowletSwitchConfig);
}

SaiArsProfileHandle* SaiArsProfileManager::getArsProfileHandle() {
  return arsProfileHandle_.get();
}
#endif

} // namespace facebook::fboss
