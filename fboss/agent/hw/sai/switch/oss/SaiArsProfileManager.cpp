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

namespace facebook::fboss {

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
#endif

} // namespace facebook::fboss
