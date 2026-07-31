/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/sai/switch/SaiBufferManager.h"

#include "fboss/agent/hw/switch_asics/TomahawkAsic.h"
#include "fboss/agent/platforms/sai/SaiBcmPlatform.h"

namespace facebook::fboss {

uint32_t SaiBufferManager::getNumCellsAvailable(const SaiPlatform* platform) {
  auto asic = platform->getAsic();
  if (asic->getAsicType() == cfg::AsicType::ASIC_TYPE_TOMAHAWK) {
    auto saiBcmPlatform = static_cast<const SaiBcmPlatform*>(platform);
    return static_cast<const TomahawkAsic*>(asic)->getNumCellsAvailable(
        platform->getType(),
        saiBcmPlatform->getHwConfigValue(
            TomahawkAsic::optimizedMqueueGuaranteeHwConfig()) != nullptr,
        saiBcmPlatform->getHwConfigValue(
            TomahawkAsic::optimizedMmuConfigOverrideHwConfig()) != nullptr);
  }
  return asic->getNumCellsAvailable(platform->getType());
}

void SaiBufferManager::loadCpuPortEgressBufferPool() {}
} // namespace facebook::fboss
