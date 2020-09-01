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

SaiBufferManager::SaiBufferManager(
    SaiManagerTable* managerTable,
    const SaiPlatform* platform)
    : managerTable_(managerTable), platform_(platform) {
  if (platform_->getAsic()->isSupported(HwAsic::Feature::BUFFER_PROFILE)) {
    setupEgressBufferPool();
  }
}

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
    XLOG(INFO) << " Device watermark: "
               << counters[SAI_BUFFER_POOL_STAT_WATERMARK_BYTES];
    // TODO publish device watermarks
  }
}

SaiBufferProfileTraits::CreateAttributes SaiBufferManager::profileCreateAttrs(
    const PortQueue& queue) const {
  SaiBufferProfileTraits::Attributes::PoolId pool{
      egressBufferPoolHandle_->bufferPool->adapterKey()};
  uint64_t resBytes =
      queue.getReservedBytes() ? queue.getReservedBytes().value() : 0;
  SaiBufferProfileTraits::Attributes::ReservedBytes reservedBytes{resBytes};
  SaiBufferProfileTraits::Attributes::ThresholdMode mode{
      SAI_BUFFER_PROFILE_THRESHOLD_MODE_DYNAMIC};
  // TODO convert scaling factor to sai values
  int8_t dynThresh = queue.getScalingFactor()
      ? static_cast<int>(queue.getScalingFactor().value())
      : 0;
  SaiBufferProfileTraits::Attributes::SharedDynamicThreshold dynamicThresh{
      static_cast<int8_t>(dynThresh)};
  // TODO: handle static threshold
  SaiBufferProfileTraits::Attributes::SharedStaticThreshold staticThresh{0};
  return SaiBufferProfileTraits::CreateAttributes{
      pool, reservedBytes, mode, dynamicThresh, staticThresh};
}

std::shared_ptr<SaiBufferProfile> SaiBufferManager::getOrCreateProfile(
    const PortQueue& queue) {
  setupEgressBufferPool();
  auto c = profileCreateAttrs(queue);
  return bufferProfiles_
      .refOrEmplace(c, c, c, managerTable_->switchManager().getSwitchSaiId())
      .first;
}

} // namespace facebook::fboss
