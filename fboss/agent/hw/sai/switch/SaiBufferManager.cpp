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
#include "fboss/agent/hw/switch_asics/IndusAsic.h"
#include "fboss/agent/hw/switch_asics/Tomahawk3Asic.h"
#include "fboss/agent/hw/switch_asics/Tomahawk4Asic.h"
#include "fboss/agent/hw/switch_asics/TomahawkAsic.h"
#include "fboss/agent/hw/switch_asics/Trident2Asic.h"
#include "fboss/agent/platforms/sai/SaiBcmPlatform.h"
#include "fboss/agent/platforms/sai/SaiPlatform.h"
#include "fboss/agent/state/PortQueue.h"

namespace facebook::fboss {

namespace {
uint64_t getSwitchEgressPoolAvailableSize(const SaiPlatform* platform) {
  auto saiSwitch = static_cast<SaiSwitch*>(platform->getHwSwitch());
  const auto switchId = saiSwitch->getSaiSwitchId();
  auto& switchApi = SaiApiTable::getInstance()->switchApi();
  return switchApi.getAttribute(
      switchId, SaiSwitchTraits::Attributes::EgressPoolAvaialableSize{});
}

void assertMaxBufferPoolSize(const SaiPlatform* platform) {
  auto saiSwitch = static_cast<SaiSwitch*>(platform->getHwSwitch());
  if (saiSwitch->getBootType() != BootType::COLD_BOOT) {
    return;
  }
  auto asic = platform->getAsic();
  if (asic->getAsicVendor() == HwAsic::AsicVendor::ASIC_VENDOR_TAJO) {
    return;
  }
  auto availableBuffer = getSwitchEgressPoolAvailableSize(platform);
  auto maxEgressPoolSize = SaiBufferManager::getMaxEgressPoolBytes(platform);
  switch (asic->getAsicType()) {
    case cfg::AsicType::ASIC_TYPE_EBRO:
    case cfg::AsicType::ASIC_TYPE_GARONNE:
    case cfg::AsicType::ASIC_TYPE_ELBERT_8DD:
    case cfg::AsicType::ASIC_TYPE_SANDIA_PHY:
    case cfg::AsicType::ASIC_TYPE_BEAS:
      XLOG(FATAL) << " Not supported";
      break;
    case cfg::AsicType::ASIC_TYPE_FAKE:
    case cfg::AsicType::ASIC_TYPE_MOCK:
      break;
    case cfg::AsicType::ASIC_TYPE_TOMAHAWK:
      // Available buffer is per XPE
      CHECK_EQ(maxEgressPoolSize, availableBuffer * 4);
      break;
    case cfg::AsicType::ASIC_TYPE_INDUS:
    case cfg::AsicType::ASIC_TYPE_TRIDENT2:
    case cfg::AsicType::ASIC_TYPE_TOMAHAWK3:
    case cfg::AsicType::ASIC_TYPE_TOMAHAWK4:
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
    case cfg::AsicType::ASIC_TYPE_FAKE:
    case cfg::AsicType::ASIC_TYPE_MOCK:
    case cfg::AsicType::ASIC_TYPE_EBRO:
    case cfg::AsicType::ASIC_TYPE_GARONNE:
      return asic->getMMUSizeBytes();
    case cfg::AsicType::ASIC_TYPE_TOMAHAWK: {
      auto constexpr kNumXpes = 4;
      auto saiBcmPlatform = static_cast<const SaiBcmPlatform*>(platform);
      auto perXpeCells = saiBcmPlatform->numCellsAvailable();
      return perXpeCells * kNumXpes *
          static_cast<const TomahawkAsic*>(asic)->getMMUCellSize();
    }
    case cfg::AsicType::ASIC_TYPE_TRIDENT2: {
      auto saiBcmPlatform = static_cast<const SaiBcmPlatform*>(platform);
      auto kCellsAvailable = saiBcmPlatform->numCellsAvailable();
      return kCellsAvailable *
          static_cast<const Trident2Asic*>(asic)->getMMUCellSize();
    }
    case cfg::AsicType::ASIC_TYPE_TOMAHAWK3: {
      auto saiBcmPlatform = static_cast<const SaiBcmPlatform*>(platform);
      auto kCellsAvailable = saiBcmPlatform->numCellsAvailable();
      return kCellsAvailable *
          static_cast<const Tomahawk3Asic*>(asic)->getMMUCellSize();
    }
    case cfg::AsicType::ASIC_TYPE_TOMAHAWK4: {
      auto saiBcmPlatform = static_cast<const SaiBcmPlatform*>(platform);
      auto kCellsAvailable = saiBcmPlatform->numCellsAvailable();
      return kCellsAvailable *
          static_cast<const Tomahawk4Asic*>(asic)->getMMUCellSize();
    }
    case cfg::AsicType::ASIC_TYPE_INDUS: {
      /*
       * XXX: TODO: Need to check if there is a way to compute the
       * buffers available for use in Indus without using the
       * egress buffer attribute.
       */
      auto saiSwitch = static_cast<SaiSwitch*>(platform->getHwSwitch());
      const auto switchId = saiSwitch->getSaiSwitchId();
      return SaiApiTable::getInstance()->switchApi().getAttribute(
          switchId, SaiSwitchTraits::Attributes::EgressPoolAvaialableSize{});
    }
    case cfg::AsicType::ASIC_TYPE_ELBERT_8DD:
    case cfg::AsicType::ASIC_TYPE_SANDIA_PHY:
    case cfg::AsicType::ASIC_TYPE_BEAS:
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
  std::optional<SaiBufferPoolTraits::Attributes::XoffSize> xoffSize{};
  SaiBufferPoolTraits::CreateAttributes c{
      SAI_BUFFER_POOL_TYPE_EGRESS,
      getMaxEgressPoolBytes(platform_),
      SAI_BUFFER_POOL_THRESHOLD_MODE_DYNAMIC,
      xoffSize};
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

  // Pool size is the sum of (shared + headroom) * number of memory buffers
  auto poolSize =
      (bufferPoolCfg->getSharedBytes() + bufferPoolCfg->getHeadroomBytes()) *
      platform_->getAsic()->getNumMemoryBuffers();
  // XoffSize configuration is needed only when PFC is supported
  std::optional<SaiBufferPoolTraits::Attributes::XoffSize> xoffSize;
#if defined(TAJO_SDK) || defined(SAI_VERSION_8_2_0_0_ODP) ||                   \
    defined(SAI_VERSION_8_2_0_0_SIM_ODP) ||                                    \
    defined(SAI_VERSION_8_2_0_0_DNX_ODP) || defined(SAI_VERSION_9_0_EA_ODP) || \
    defined(SAI_VERSION_9_0_EA_DNX_ODP)
  if (platform_->getAsic()->isSupported(HwAsic::Feature::PFC)) {
    xoffSize = bufferPoolCfg->getHeadroomBytes() *
        platform_->getAsic()->getNumMemoryBuffers();
  }
#endif
  SaiBufferPoolTraits::CreateAttributes c{
      SAI_BUFFER_POOL_TYPE_INGRESS,
      poolSize,
      SAI_BUFFER_POOL_THRESHOLD_MODE_STATIC,
      xoffSize};
  ingressBufferPoolHandle_->bufferPool =
      store.setObject(SAI_BUFFER_POOL_TYPE_INGRESS, c);
}

void SaiBufferManager::setupIngressEgressBufferPool(
    const PortPgConfig* portPgCfg) {
  std::optional<SaiBufferPoolTraits::Attributes::XoffSize> xoffSize;
  if (!ingressEgressBufferPoolHandle_) {
    auto availableBuffer = getSwitchEgressPoolAvailableSize(platform_);
    auto poolSize =
        availableBuffer * platform_->getAsic()->getNumMemoryBuffers();

    ingressEgressBufferPoolHandle_ = std::make_unique<SaiBufferPoolHandle>();
    SaiBufferPoolTraits::CreateAttributes c{
        SAI_BUFFER_POOL_TYPE_BOTH,
        poolSize,
        SAI_BUFFER_POOL_THRESHOLD_MODE_STATIC,
        xoffSize};

    auto& store = saiStore_->get<SaiBufferPoolTraits>();
    ingressEgressBufferPoolHandle_->bufferPool =
        store.setObject(SAI_BUFFER_POOL_TYPE_BOTH, c);
  }
  // XoffSize is set separately as it is part of ingress config alone
  if (portPgCfg && platform_->getAsic()->isSupported(HwAsic::Feature::PFC)) {
#if defined(TAJO_SDK) || defined(SAI_VERSION_8_2_0_0_ODP) ||                   \
    defined(SAI_VERSION_8_2_0_0_DNX_ODP) || defined(SAI_VERSION_9_0_EA_ODP) || \
    defined(SAI_VERSION_9_0_EA_DNX_ODP)
    auto bufferPoolCfg = portPgCfg->getBufferPoolConfig().value();
    SaiBufferPoolTraits::Attributes::XoffSize xoffSize =
        bufferPoolCfg->getHeadroomBytes() *
        platform_->getAsic()->getNumMemoryBuffers();
    SaiApiTable::getInstance()->bufferApi().setAttribute(
        ingressEgressBufferPoolHandle_->bufferPool->adapterKey(), xoffSize);
#endif
  }
}

SaiBufferPoolHandle* SaiBufferManager::getIngressBufferPoolHandle() const {
  return ingressEgressBufferPoolHandle_ ? ingressEgressBufferPoolHandle_.get()
                                        : ingressBufferPoolHandle_.get();
}

SaiBufferPoolHandle* SaiBufferManager::getEgressBufferPoolHandle() const {
  return ingressEgressBufferPoolHandle_ ? ingressEgressBufferPoolHandle_.get()
                                        : egressBufferPoolHandle_.get();
}

void SaiBufferManager::updateStats() {
  auto bufferPoolHandle = getEgressBufferPoolHandle();
  if (bufferPoolHandle) {
    bufferPoolHandle->bufferPool->updateStats();
    auto counters = bufferPoolHandle->bufferPool->getStats();
    deviceWatermarkBytes_ = counters[SAI_BUFFER_POOL_STAT_WATERMARK_BYTES];
    publishDeviceWatermark(deviceWatermarkBytes_);
  }
}

SaiBufferProfileTraits::CreateAttributes SaiBufferManager::profileCreateAttrs(
    const PortQueue& queue) const {
  SaiBufferProfileTraits::Attributes::PoolId pool{
      getEgressBufferPoolHandle()->bufferPool->adapterKey()};
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

void SaiBufferManager::setupBufferPool(const PortPgConfig* ingressPgCfg) {
  if (platform_->getAsic()->isSupported(
          HwAsic::Feature::SHARED_INGRESS_EGRESS_BUFFER_POOL)) {
    setupIngressEgressBufferPool(ingressPgCfg);
  } else {
    // Call ingress or egress buffer pool based on the cfg passed in!
    if (ingressPgCfg) {
      setupIngressBufferPool(*ingressPgCfg);
    } else {
      setupEgressBufferPool();
    }
  }
}

std::shared_ptr<SaiBufferProfile> SaiBufferManager::getOrCreateProfile(
    const PortQueue& queue) {
  setupBufferPool();
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
      getIngressBufferPoolHandle()->bufferPool->adapterKey()};
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
  setupBufferPool(&portPgConfig);
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
  const auto& portPgCfgs = port->getPortPgConfigs();
  if (portPgCfgs.has_value()) {
    const auto& portPgCfg = (*portPgCfgs)[0];
    setupBufferPool(portPgCfg.get());
  }
}

void SaiBufferManager::setIngressPriorityGroupBufferProfile(
    IngressPriorityGroupSaiId pgId,
    std::shared_ptr<SaiBufferProfile> bufferProfile) {
  SaiIngressPriorityGroupTraits::Attributes::BufferProfile bufferProfileId{
      SAI_NULL_OBJECT_ID};
  if (bufferProfile) {
    bufferProfileId = SaiIngressPriorityGroupTraits::Attributes::BufferProfile{
        bufferProfile->adapterKey()};
  }
  SaiApiTable::getInstance()->bufferApi().setAttribute(pgId, bufferProfileId);
}

} // namespace facebook::fboss
