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
#include "fboss/agent/hw/sai/api/SaiApiTable.h"
#include "fboss/agent/hw/sai/api/SwitchApi.h"
#include "fboss/agent/hw/sai/store/SaiStore.h"
#include "fboss/agent/hw/sai/switch/SaiManagerTable.h"
#include "fboss/agent/hw/sai/switch/SaiSwitch.h"
#include "fboss/agent/hw/sai/switch/SaiSwitchManager.h"
#include "fboss/agent/hw/switch_asics/HwAsic.h"
#include "fboss/agent/hw/switch_asics/Tomahawk3Asic.h"
#include "fboss/agent/hw/switch_asics/TomahawkAsic.h"
#include "fboss/agent/hw/switch_asics/Trident2Asic.h"
#include "fboss/agent/platforms/sai/SaiBcmPlatform.h"
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

void assertMaxBufferPoolSize(const SaiPlatform* platform) {
  auto saiSwitch = static_cast<SaiSwitch*>(platform->getHwSwitch());
  if (saiSwitch->getBootType() != BootType::COLD_BOOT) {
    return;
  }
  auto asic = platform->getAsic();
  if (asic->getAsicType() == HwAsic::AsicType::ASIC_TYPE_TAJO) {
    return;
  }
  const auto switchId = saiSwitch->getSwitchId();
  auto& switchApi = SaiApiTable::getInstance()->switchApi();
  auto availableBuffer = switchApi.getAttribute(
      switchId, SaiSwitchTraits::Attributes::EgressPoolAvaialableSize{});
  auto maxEgressPoolSize = SaiBufferManager::getMaxEgressPoolBytes(platform);
  switch (asic->getAsicType()) {
    case HwAsic::AsicType::ASIC_TYPE_TAJO:
    case HwAsic::AsicType::ASIC_TYPE_ELBERT_8DD:
      XLOG(FATAL) << " Not supported";
      break;
    case HwAsic::AsicType::ASIC_TYPE_FAKE:
    case HwAsic::AsicType::ASIC_TYPE_MOCK:
      break;
    case HwAsic::AsicType::ASIC_TYPE_TOMAHAWK:
      // Available buffer is per XPE
      CHECK_EQ(maxEgressPoolSize, availableBuffer * 4);
      break;
    case HwAsic::AsicType::ASIC_TYPE_TRIDENT2:
    case HwAsic::AsicType::ASIC_TYPE_TOMAHAWK3:
      CHECK_EQ(maxEgressPoolSize, availableBuffer);
      break;
    case HwAsic::AsicType::ASIC_TYPE_TOMAHAWK4:
      XLOG(FATAL) << " TODO, Handle buffer pools for ASIC ";
  }
}

} // namespace
SaiBufferManager::SaiBufferManager(
    SaiStore* saiStore,
    SaiManagerTable* managerTable,
    const SaiPlatform* platform)
    : saiStore_(saiStore), managerTable_(managerTable), platform_(platform) {}

uint64_t SaiBufferManager::getMaxEgressPoolBytes(const SaiPlatform* platform) {
  auto asic = platform->getAsic();
  switch (asic->getAsicType()) {
    case HwAsic::AsicType::ASIC_TYPE_FAKE:
    case HwAsic::AsicType::ASIC_TYPE_MOCK:
    case HwAsic::AsicType::ASIC_TYPE_TAJO:
      return asic->getMMUSizeBytes();
    case HwAsic::AsicType::ASIC_TYPE_TOMAHAWK: {
      auto constexpr kPerXpeCellsAvailable = 0x436e;
      auto constexpr kPerXpeCellsAvailableOptimized = 0x454A;
      auto constexpr kNumXpes = 4;
      auto perXpeCells = kPerXpeCellsAvailable;
      auto saiBcmPlatform = static_cast<const SaiBcmPlatform*>(platform);
      if (saiBcmPlatform->getHwConfigValue("buf.mqueue.guarantee.0") &&
          saiBcmPlatform->getHwConfigValue("mmu_config_override")) {
        // MMU optimized, more cells available
        perXpeCells = kPerXpeCellsAvailableOptimized;
      }
      return perXpeCells * kNumXpes *
          static_cast<const TomahawkAsic*>(asic)->getMMUCellSize();
    }
    case HwAsic::AsicType::ASIC_TYPE_TRIDENT2: {
      auto constexpr kCellsAvailable = 0xbd0f;
      return kCellsAvailable *
          static_cast<const Trident2Asic*>(asic)->getMMUCellSize();
    }
    case HwAsic::AsicType::ASIC_TYPE_TOMAHAWK3: {
      auto constexpr kCellsAvailable = 0x1fca9;
      return kCellsAvailable *
          static_cast<const Tomahawk3Asic*>(asic)->getMMUCellSize();
    }
    case HwAsic::AsicType::ASIC_TYPE_TOMAHAWK4:
    case HwAsic::AsicType::ASIC_TYPE_ELBERT_8DD:
      throw FbossError(
          "Not supported to get max egress pool for ASIC: ",
          asic->getAsicType());
  }
  CHECK(0) << "Should never get here";
  return 0;
}

void SaiBufferManager::setupEgressBufferPool() {
  if (egressBufferPoolHandle_) {
    return;
  }
  assertMaxBufferPoolSize(platform_);
  egressBufferPoolHandle_ = std::make_unique<SaiBufferPoolHandle>();
  auto& store = saiStore_->get<SaiBufferPoolTraits>();
  SaiBufferPoolTraits::CreateAttributes c{
      SAI_BUFFER_POOL_TYPE_EGRESS,
      getMaxEgressPoolBytes(platform_),
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
      pool, reservedBytes, mode, dynThresh};
}

std::shared_ptr<SaiBufferProfile> SaiBufferManager::getOrCreateProfile(
    const PortQueue& queue) {
  setupEgressBufferPool();
  // TODO throw error if shared bytes is set. We don't handle that in SAI
  auto attributes = profileCreateAttrs(queue);
  auto& store = saiStore_->get<SaiBufferProfileTraits>();
  SaiBufferProfileTraits::AdapterHostKey k = tupleProjection<
      SaiBufferProfileTraits::CreateAttributes,
      SaiBufferProfileTraits::AdapterHostKey>(attributes);
  return store.setObject(k, attributes);
}

} // namespace facebook::fboss
