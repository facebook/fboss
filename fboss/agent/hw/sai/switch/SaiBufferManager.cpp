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
#include "fboss/agent/hw/switch_asics/Tomahawk4Asic.h"
#include "fboss/agent/hw/switch_asics/TomahawkAsic.h"
#include "fboss/agent/hw/switch_asics/Trident2Asic.h"
#include "fboss/agent/platforms/sai/SaiBcmPlatform.h"
#include "fboss/agent/platforms/sai/SaiPlatform.h"
#include "fboss/agent/state/PortQueue.h"

namespace facebook::fboss {

namespace {
void assertMaxBufferPoolSize(const SaiPlatform* platform) {
  auto saiSwitch = static_cast<SaiSwitch*>(platform->getHwSwitch());
  if (saiSwitch->getBootType() != BootType::COLD_BOOT) {
    return;
  }
  auto asic = platform->getAsic();
  if (asic->getAsicVendor() == HwAsic::AsicVendor::ASIC_VENDOR_TAJO) {
    return;
  }
  const auto switchId = saiSwitch->getSwitchId();
  auto& switchApi = SaiApiTable::getInstance()->switchApi();
  auto availableBuffer = switchApi.getAttribute(
      switchId, SaiSwitchTraits::Attributes::EgressPoolAvaialableSize{});
  auto maxEgressPoolSize = SaiBufferManager::getMaxEgressPoolBytes(platform);
  switch (asic->getAsicType()) {
    case HwAsic::AsicType::ASIC_TYPE_EBRO:
    case HwAsic::AsicType::ASIC_TYPE_GARONNE:
    case HwAsic::AsicType::ASIC_TYPE_ELBERT_8DD:
    case HwAsic::AsicType::ASIC_TYPE_SANDIA_PHY:
    case HwAsic::AsicType::ASIC_TYPE_INDUS:
    case HwAsic::AsicType::ASIC_TYPE_BEAS:
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
    case HwAsic::AsicType::ASIC_TYPE_TOMAHAWK4:
      CHECK_EQ(maxEgressPoolSize, availableBuffer);
      break;
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
    case HwAsic::AsicType::ASIC_TYPE_EBRO:
    case HwAsic::AsicType::ASIC_TYPE_GARONNE:
      return asic->getMMUSizeBytes();
    case HwAsic::AsicType::ASIC_TYPE_TOMAHAWK: {
      auto constexpr kNumXpes = 4;
      auto saiBcmPlatform = static_cast<const SaiBcmPlatform*>(platform);
      auto perXpeCells = saiBcmPlatform->numCellsAvailable();
      return perXpeCells * kNumXpes *
          static_cast<const TomahawkAsic*>(asic)->getMMUCellSize();
    }
    case HwAsic::AsicType::ASIC_TYPE_TRIDENT2: {
      auto saiBcmPlatform = static_cast<const SaiBcmPlatform*>(platform);
      auto kCellsAvailable = saiBcmPlatform->numCellsAvailable();
      return kCellsAvailable *
          static_cast<const Trident2Asic*>(asic)->getMMUCellSize();
    }
    case HwAsic::AsicType::ASIC_TYPE_TOMAHAWK3: {
      auto saiBcmPlatform = static_cast<const SaiBcmPlatform*>(platform);
      auto kCellsAvailable = saiBcmPlatform->numCellsAvailable();
      return kCellsAvailable *
          static_cast<const Tomahawk3Asic*>(asic)->getMMUCellSize();
    }
    case HwAsic::AsicType::ASIC_TYPE_TOMAHAWK4: {
      auto saiBcmPlatform = static_cast<const SaiBcmPlatform*>(platform);
      auto kCellsAvailable = saiBcmPlatform->numCellsAvailable();
      return kCellsAvailable *
          static_cast<const Tomahawk4Asic*>(asic)->getMMUCellSize();
    }
    case HwAsic::AsicType::ASIC_TYPE_ELBERT_8DD:
    case HwAsic::AsicType::ASIC_TYPE_SANDIA_PHY:
    case HwAsic::AsicType::ASIC_TYPE_INDUS:
    case HwAsic::AsicType::ASIC_TYPE_BEAS:
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
  SaiBufferPoolTraits::CreateAttributes c {
    SAI_BUFFER_POOL_TYPE_EGRESS, getMaxEgressPoolBytes(platform_),
        SAI_BUFFER_POOL_THRESHOLD_MODE_DYNAMIC
#if defined(TAJO_SDK)
        ,
        0 /* XoffSize for Egress Pool */
#endif
  };
  egressBufferPoolHandle_->bufferPool =
      store.setObject(SAI_BUFFER_POOL_TYPE_EGRESS, c);
}

void SaiBufferManager::setupIngressBufferPool(const PortPgConfig& portPgCfg) {
  if (ingressBufferPoolHandle_) {
    return;
  }
  ingressBufferPoolHandle_ = std::make_unique<SaiBufferPoolHandle>();
  auto& store = saiStore_->get<SaiBufferPoolTraits>();
  auto bufferPoolCfg = portPgCfg.getBufferPoolConfig().value();
  SaiBufferPoolTraits::CreateAttributes c {
    SAI_BUFFER_POOL_TYPE_INGRESS, bufferPoolCfg->getSharedBytes(),
        SAI_BUFFER_POOL_THRESHOLD_MODE_DYNAMIC
#if defined(TAJO_SDK)
        ,
        bufferPoolCfg->getHeadroomBytes()
#endif
  };
  ingressBufferPoolHandle_->bufferPool =
      store.setObject(SAI_BUFFER_POOL_TYPE_INGRESS, c);
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
  SaiBufferProfileTraits::Attributes::SharedDynamicThreshold dynThresh{0};
  if (platform_->getAsic()->scalingFactorBasedDynamicThresholdSupported() &&
      queue.getScalingFactor()) {
    dynThresh = platform_->getAsic()->getBufferDynThreshFromScalingFactor(
        queue.getScalingFactor().value());
  }
  return SaiBufferProfileTraits::CreateAttributes{
      pool, reservedBytes, mode, dynThresh, 0, 0, 0};
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

SaiBufferProfileTraits::CreateAttributes
SaiBufferManager::ingressProfileCreateAttrs(const PortPgConfig& config) const {
  SaiBufferProfileTraits::Attributes::PoolId pool{
      ingressBufferPoolHandle_->bufferPool->adapterKey()};
  SaiBufferProfileTraits::Attributes::ReservedBytes reservedBytes =
      config.getMinLimitBytes();
  SaiBufferProfileTraits::Attributes::ThresholdMode mode{
      SAI_BUFFER_PROFILE_THRESHOLD_MODE_STATIC};
  SaiBufferProfileTraits::Attributes::SharedDynamicThreshold dynThresh{0};
  if (config.getScalingFactor()) {
    mode = SAI_BUFFER_PROFILE_THRESHOLD_MODE_DYNAMIC;
    dynThresh = static_cast<sai_uint8_t>(config.getScalingFactor().value());
  }
  SaiBufferProfileTraits::Attributes::XoffTh xoffTh{0};
  if (config.getHeadroomLimitBytes()) {
    xoffTh = config.getHeadroomLimitBytes().value();
  }
  SaiBufferProfileTraits::Attributes::XonTh xonTh{0}; // Not configured!
  SaiBufferProfileTraits::Attributes::XonOffsetTh xonOffsetTh{0};
  if (config.getResumeOffsetBytes()) {
    xonOffsetTh = config.getResumeOffsetBytes().value();
  }
  return SaiBufferProfileTraits::CreateAttributes{
      pool, reservedBytes, mode, dynThresh, xoffTh, xonTh, xonOffsetTh};
}

std::shared_ptr<SaiBufferProfile> SaiBufferManager::getOrCreateIngressProfile(
    const PortPgConfig& portPgConfig) {
  setupIngressBufferPool(portPgConfig);
  auto attributes = ingressProfileCreateAttrs(portPgConfig);
  auto& store = saiStore_->get<SaiBufferProfileTraits>();
  SaiBufferProfileTraits::AdapterHostKey k = tupleProjection<
      SaiBufferProfileTraits::CreateAttributes,
      SaiBufferProfileTraits::AdapterHostKey>(attributes);
  return store.setObject(k, attributes);
}

void SaiBufferManager::createIngressBufferPool(
    const std::shared_ptr<Port> port) {
  /*
   * We expect to create a single ingress buffer pool and there
   * are checks in place to ensure more than 1 buffer pool is
   * not configured. Although there are multiple possible port
   * PG configs, all of it can point only to the same buffer
   * pool config, hence we need to just process a single
   * port PG config.
   * Buffer configuration change would fall under disruptive
   * config change and hence is not expected to happen as part
   * of warm boot, hence we dont need to handle update case for
   * buffer pool config as well.
   */
  if (!ingressBufferPoolHandle_) {
    const auto& portPgCfgs = port->getPortPgConfigs();
    const auto& portPgCfg = (*portPgCfgs)[0];
    setupIngressBufferPool(*portPgCfg);
  }
}

} // namespace facebook::fboss
