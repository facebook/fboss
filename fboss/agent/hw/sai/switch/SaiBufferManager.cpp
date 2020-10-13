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

#include "fboss/agent/FbossError.h"
#include "fboss/agent/Platform.h"
#include "fboss/agent/hw/sai/store/SaiStore.h"
#include "fboss/agent/hw/sai/switch/SaiManagerTable.h"
#include "fboss/agent/hw/sai/switch/SaiSwitchManager.h"
#include "fboss/agent/hw/switch_asics/HwAsic.h"
#include "fboss/agent/platforms/sai/SaiPlatform.h"
#include "fboss/agent/state/PortQueue.h"

namespace facebook::fboss {

namespace {
int scalingFactorToBufferDynThresh(cfg::MMUScalingFactor scalingFactor) {
  switch (scalingFactor) {
    case cfg::MMUScalingFactor::ONE:
      return 0;
    case cfg::MMUScalingFactor::EIGHT:
      return 3;
    case cfg::MMUScalingFactor::ONE_128TH:
      return -7;
    case cfg::MMUScalingFactor::ONE_64TH:
      return -6;
    case cfg::MMUScalingFactor::ONE_32TH:
      return -5;
    case cfg::MMUScalingFactor::ONE_16TH:
      return -4;
    case cfg::MMUScalingFactor::ONE_8TH:
      return -3;
    case cfg::MMUScalingFactor::ONE_QUARTER:
      return -2;
    case cfg::MMUScalingFactor::ONE_HALF:
      return -1;
    case cfg::MMUScalingFactor::TWO:
      return 1;
    case cfg::MMUScalingFactor::FOUR:
      return 2;
  }
  CHECK(0) << "Should never get here";
  return -1;
}
} // namespace
SaiBufferManager::SaiBufferManager(
    SaiManagerTable* managerTable,
    const SaiPlatform* platform)
    : managerTable_(managerTable), platform_(platform) {}

void SaiBufferManager::setupEgressBufferPool() {
  if (egressBufferPoolHandle_) {
    return;
  }
  egressBufferPoolHandle_ = std::make_unique<SaiBufferPoolHandle>();
  auto& store = SaiStore::getInstance()->get<SaiBufferPoolTraits>();
  auto mmuSize = platform_->getAsic()->getMMUSizeBytes();
  SaiBufferPoolTraits::CreateAttributes c{
      SAI_BUFFER_POOL_TYPE_EGRESS,
      mmuSize,
      SAI_BUFFER_POOL_THRESHOLD_MODE_DYNAMIC};
  egressBufferPoolHandle_->bufferPool =
      store.setObject(SAI_BUFFER_POOL_TYPE_EGRESS, c);
}

void SaiBufferManager::updateStats() {
  if (egressBufferPoolHandle_) {
    egressBufferPoolHandle_->bufferPool->updateStats();
    auto counters = egressBufferPoolHandle_->bufferPool->getStats();
    deviceWatermarkBytes_ = counters[SAI_BUFFER_POOL_STAT_WATERMARK_BYTES];
    publishDeviceWatermark(deviceWatermarkBytes_);
  }
}

SaiBufferProfileTraits::CreateAttributes SaiBufferManager::profileCreateAttrs(
    const PortQueue& queue) const {
  SaiBufferProfileTraits::Attributes::PoolId pool{
      egressBufferPoolHandle_->bufferPool->adapterKey()};
  std::optional<SaiBufferProfileTraits::Attributes::ReservedBytes>
      reservedBytes;
  if (queue.getReservedBytes()) {
    reservedBytes = queue.getReservedBytes().value();
  }
  SaiBufferProfileTraits::Attributes::ThresholdMode mode{
      SAI_BUFFER_PROFILE_THRESHOLD_MODE_DYNAMIC};
  std::optional<SaiBufferProfileTraits::Attributes::SharedDynamicThreshold>
      dynThresh;
  if (queue.getScalingFactor()) {
    dynThresh =
        scalingFactorToBufferDynThresh(queue.getScalingFactor().value());
  }
  return SaiBufferProfileTraits::CreateAttributes{
      pool, reservedBytes, mode, dynThresh, std::nullopt};
}

std::shared_ptr<SaiBufferProfile> SaiBufferManager::getOrCreateProfile(
    const PortQueue& queue) {
  setupEgressBufferPool();
  // TODO throw error if shared bytes is set. We don't handle that in SAI
  auto c = profileCreateAttrs(queue);
  return bufferProfiles_
      .refOrEmplace(c, c, c, managerTable_->switchManager().getSwitchSaiId())
      .first;
}

} // namespace facebook::fboss
