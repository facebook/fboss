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
#include "fboss/agent/Utils.h"
#include "fboss/agent/hw/sai/api/SaiApiTable.h"
#include "fboss/agent/hw/sai/api/SwitchApi.h"
#include "fboss/agent/hw/sai/store/SaiStore.h"
#include "fboss/agent/hw/sai/switch/SaiManagerTable.h"
#include "fboss/agent/hw/sai/switch/SaiSwitch.h"
#include "fboss/agent/hw/sai/switch/SaiSwitchManager.h"
#include "fboss/agent/hw/switch_asics/HwAsic.h"
#include "fboss/agent/hw/switch_asics/Jericho2Asic.h"
#include "fboss/agent/hw/switch_asics/Jericho3Asic.h"
#include "fboss/agent/hw/switch_asics/Tomahawk3Asic.h"
#include "fboss/agent/hw/switch_asics/Tomahawk4Asic.h"
#include "fboss/agent/hw/switch_asics/Tomahawk5Asic.h"
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
    case cfg::AsicType::ASIC_TYPE_YUBA:
    case cfg::AsicType::ASIC_TYPE_ELBERT_8DD:
    case cfg::AsicType::ASIC_TYPE_SANDIA_PHY:
    case cfg::AsicType::ASIC_TYPE_RAMON:
    case cfg::AsicType::ASIC_TYPE_RAMON3:
      XLOG(FATAL) << " Not supported";
      break;
    case cfg::AsicType::ASIC_TYPE_FAKE:
    case cfg::AsicType::ASIC_TYPE_MOCK:
      break;
    case cfg::AsicType::ASIC_TYPE_TOMAHAWK:
      // Available buffer is per XPE
      CHECK_EQ(maxEgressPoolSize, availableBuffer * 4);
      break;
    case cfg::AsicType::ASIC_TYPE_JERICHO2:
    case cfg::AsicType::ASIC_TYPE_JERICHO3:
    case cfg::AsicType::ASIC_TYPE_TRIDENT2:
    case cfg::AsicType::ASIC_TYPE_TOMAHAWK3:
    case cfg::AsicType::ASIC_TYPE_TOMAHAWK4:
    case cfg::AsicType::ASIC_TYPE_TOMAHAWK5:
      CHECK_EQ(maxEgressPoolSize, availableBuffer);
      break;
  }
}

} // namespace

const std::string kDefaultEgressBufferPoolName{"default"};

SaiBufferManager::SaiBufferManager(
    SaiStore* saiStore,
    SaiManagerTable* managerTable,
    const SaiPlatform* platform)
    : saiStore_(saiStore), managerTable_(managerTable), platform_(platform) {}

uint64_t SaiBufferManager::getMaxEgressPoolBytes(const SaiPlatform* platform) {
  SaiSwitch* saiSwitch;
  SwitchSaiId switchId;
  auto asic = platform->getAsic();
  switch (asic->getAsicType()) {
    case cfg::AsicType::ASIC_TYPE_FAKE:
    case cfg::AsicType::ASIC_TYPE_MOCK:
    case cfg::AsicType::ASIC_TYPE_EBRO:
    case cfg::AsicType::ASIC_TYPE_GARONNE:
    case cfg::AsicType::ASIC_TYPE_YUBA:
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
    case cfg::AsicType::ASIC_TYPE_TOMAHAWK5: {
      auto saiBcmPlatform = static_cast<const SaiBcmPlatform*>(platform);
      auto kCellsAvailable = saiBcmPlatform->numCellsAvailable();
      return kCellsAvailable *
          static_cast<const Tomahawk5Asic*>(asic)->getMMUCellSize();
    }
    case cfg::AsicType::ASIC_TYPE_JERICHO2:
    case cfg::AsicType::ASIC_TYPE_JERICHO3:
      /*
       * XXX: TODO: Need to check if there is a way to compute the
       * buffers available for use in Jericho2 without using the
       * egress buffer attribute.
       */
      saiSwitch = static_cast<SaiSwitch*>(platform->getHwSwitch());
      switchId = saiSwitch->getSaiSwitchId();
      return SaiApiTable::getInstance()->switchApi().getAttribute(
          switchId, SaiSwitchTraits::Attributes::EgressPoolAvaialableSize{});
    case cfg::AsicType::ASIC_TYPE_ELBERT_8DD:
    case cfg::AsicType::ASIC_TYPE_SANDIA_PHY:
    case cfg::AsicType::ASIC_TYPE_RAMON:
    case cfg::AsicType::ASIC_TYPE_RAMON3:
      throw FbossError(
          "Not supported to get max egress pool for ASIC: ",
          asic->getAsicType());
  }
  CHECK(0) << "Should never get here";
  return 0;
}

void SaiBufferManager::setupEgressBufferPool(
    const std::optional<BufferPoolFields>& bufferPoolCfg) {
  auto bufferPoolName = bufferPoolCfg.has_value()
      ? *bufferPoolCfg->id()
      : kDefaultEgressBufferPoolName;
  if (egressBufferPoolHandle_.find(bufferPoolName) !=
      egressBufferPoolHandle_.end()) {
    return;
  }
  assertMaxBufferPoolSize(platform_);
  uint64_t poolSize;
  std::optional<SaiBufferPoolTraits::Attributes::XoffSize> xoffSize;
  if (bufferPoolCfg.has_value() && *bufferPoolCfg->sharedBytes()) {
    // TODO: Account for reserved once available
    poolSize = *bufferPoolCfg->sharedBytes();
  } else {
    poolSize = getMaxEgressPoolBytes(platform_);
  }
  SaiBufferPoolTraits::CreateAttributes attributes{
      SAI_BUFFER_POOL_TYPE_EGRESS,
      poolSize,
      SAI_BUFFER_POOL_THRESHOLD_MODE_DYNAMIC,
      xoffSize};
  SaiBufferPoolTraits::AdapterHostKey k = tupleProjection<
      SaiBufferPoolTraits::CreateAttributes,
      SaiBufferPoolTraits::AdapterHostKey>(attributes);
  auto bufferPoolHandle = std::make_unique<SaiBufferPoolHandle>();
  auto& store = saiStore_->get<SaiBufferPoolTraits>();
  bufferPoolHandle->bufferPool = store.setObject(k, attributes);
  egressBufferPoolHandle_[bufferPoolName] = std::move(bufferPoolHandle);
}

void SaiBufferManager::setupBufferPool(const PortQueue& queue) {
  if (platform_->getAsic()->isSupported(
          HwAsic::Feature::SHARED_INGRESS_EGRESS_BUFFER_POOL)) {
    setupIngressEgressBufferPool();
  } else {
    std::optional<BufferPoolFields> bufferPoolCfg;
    if (queue.getBufferPoolConfig().has_value() &&
        queue.getBufferPoolConfig().value()) {
      bufferPoolCfg = queue.getBufferPoolConfig().value()->toThrift();
    }
    setupEgressBufferPool(bufferPoolCfg);
  }
}

void SaiBufferManager::setupIngressBufferPool(
    const std::string& bufferPoolName,
    const BufferPoolFields& bufferPoolCfg) {
  if (ingressBufferPoolHandle_) {
    return;
  }
  ingressBufferPoolHandle_ = std::make_unique<SaiBufferPoolHandle>();
  auto& store = saiStore_->get<SaiBufferPoolTraits>();

  // Pool size is the sum of (shared + headroom) * number of memory buffers
  auto poolSize =
      (*bufferPoolCfg.sharedBytes() + *bufferPoolCfg.headroomBytes()) *
      platform_->getAsic()->getNumMemoryBuffers();
  // XoffSize configuration is needed only when PFC is supported
  std::optional<SaiBufferPoolTraits::Attributes::XoffSize> xoffSize;
#if defined(TAJO_SDK) || defined(BRCM_SAI_SDK_XGS_AND_DNX)
  if (platform_->getAsic()->isSupported(HwAsic::Feature::PFC)) {
    xoffSize = *bufferPoolCfg.headroomBytes() *
        platform_->getAsic()->getNumMemoryBuffers();
  }
#endif
  SaiBufferPoolTraits::CreateAttributes attributes{
      SAI_BUFFER_POOL_TYPE_INGRESS,
      poolSize,
      SAI_BUFFER_POOL_THRESHOLD_MODE_STATIC,
      xoffSize};
  SaiBufferPoolTraits::AdapterHostKey k = tupleProjection<
      SaiBufferPoolTraits::CreateAttributes,
      SaiBufferPoolTraits::AdapterHostKey>(attributes);
  ingressBufferPoolHandle_->bufferPool = store.setObject(k, attributes);
  ingressBufferPoolHandle_->bufferPoolName = bufferPoolName;
}

void SaiBufferManager::createOrUpdateIngressEgressBufferPool(
    uint64_t poolSize,
    std::optional<int32_t> newXoffSize) {
  std::optional<SaiBufferPoolTraits::Attributes::XoffSize> xoffSize;
  if (newXoffSize.has_value()) {
    xoffSize = *newXoffSize;
  }
  XLOG(DBG2) << "Pool size: " << poolSize
             << ", Xoff size: " << (newXoffSize.has_value() ? *newXoffSize : 0);
  SaiBufferPoolTraits::CreateAttributes attributes{
      SAI_BUFFER_POOL_TYPE_BOTH,
      poolSize,
      SAI_BUFFER_POOL_THRESHOLD_MODE_STATIC,
      xoffSize};

  auto& store = saiStore_->get<SaiBufferPoolTraits>();
  if (!ingressEgressBufferPoolHandle_) {
    ingressEgressBufferPoolHandle_ = std::make_unique<SaiBufferPoolHandle>();
  }
  SaiBufferPoolTraits::AdapterHostKey k = tupleProjection<
      SaiBufferPoolTraits::CreateAttributes,
      SaiBufferPoolTraits::AdapterHostKey>(attributes);
  ingressEgressBufferPoolHandle_->bufferPool = store.setObject(k, attributes);
}

void SaiBufferManager::setupIngressEgressBufferPool(
    const std::optional<std::string>& bufferPoolName,
    const std::optional<BufferPoolFields>& bufferPoolCfg) {
  uint64_t poolSize{0};
  if (FLAGS_ingress_egress_buffer_pool_size) {
    // An option for test to override the buffer pool size to be used.
    poolSize = FLAGS_ingress_egress_buffer_pool_size *
        platform_->getAsic()->getNumMemoryBuffers();
  } else {
    // For Jericho ASIC family, there is a single ingress/egress buffer
    // pool and hence the usage getSwitchEgressPoolAvailableSize() might
    // be a bit confusing. Using this attribute to avoid a new attribute
    // to get buffers size for these ASICs. Also, the size returned is
    // the combined SRAM/DRAM size. From DRAM however, the whole size is
    // not available for use, a note on the same from Broadcom is captured
    // in CS00012297372. The size returned is per core and hence multiplying
    // with number of cores to get the whole buffer pool size.
    poolSize = getSwitchEgressPoolAvailableSize(platform_) *
        platform_->getAsic()->getNumCores();
  }
  std::optional<int32_t> newXoffSize;
  if (bufferPoolCfg &&
      platform_->getAsic()->isSupported(HwAsic::Feature::PFC)) {
    newXoffSize = *(*bufferPoolCfg).headroomBytes() *
        platform_->getAsic()->getNumMemoryBuffers();
  }
  if (!ingressEgressBufferPoolHandle_) {
    createOrUpdateIngressEgressBufferPool(poolSize, newXoffSize);
  } else if (newXoffSize.has_value()) {
    // XoffSize might have to be set separately as it is part of
    // ingress config alone
    auto oldXoffSize =
        std::get<std::optional<SaiBufferPoolTraits::Attributes::XoffSize>>(
            ingressEgressBufferPoolHandle_->bufferPool->attributes());
    if (!oldXoffSize.has_value() ||
        (oldXoffSize.value() != newXoffSize.value())) {
      createOrUpdateIngressEgressBufferPool(poolSize, newXoffSize);
    }
  }
  if (bufferPoolName.has_value()) {
    ingressEgressBufferPoolHandle_->bufferPoolName = *bufferPoolName;
  }
}

SaiBufferPoolHandle* SaiBufferManager::getIngressBufferPoolHandle() const {
  return ingressEgressBufferPoolHandle_ ? ingressEgressBufferPoolHandle_.get()
                                        : ingressBufferPoolHandle_.get();
}

SaiBufferPoolHandle* SaiBufferManager::getEgressBufferPoolHandle(
    const PortQueue& queue) const {
  if (ingressEgressBufferPoolHandle_) {
    return ingressEgressBufferPoolHandle_.get();
  } else {
    // There are multiple egress pools possible, so get
    // information for the right egress pool
    std::string poolName = kDefaultEgressBufferPoolName;
    if (queue.getBufferPoolConfig().has_value() &&
        queue.getBufferPoolConfig().value()) {
      auto bufferPoolCfg = queue.getBufferPoolConfig().value()->toThrift();
      poolName = *bufferPoolCfg.id();
    }
    auto poolIter = egressBufferPoolHandle_.find(poolName);
    if (poolIter == egressBufferPoolHandle_.end()) {
      throw FbossError("Egress pool ", poolName, " not created yet!");
    }
    return poolIter->second.get();
  }
}

void SaiBufferManager::updateEgressBufferPoolStats() {
  if (!egressBufferPoolHandle_.size()) {
    // Applies to platforms with SHARED_INGRESS_EGRESS_BUFFER_POOL, where
    // watermarks are polled as part of ingress itself.
    return;
  }
  // As of now, we just need to be exporting the buffer pool stats for
  // default pool, will expose the buffer pool stats for non-default if
  // needed in future.
  auto poolIter = egressBufferPoolHandle_.find(kDefaultEgressBufferPoolName);
  if (poolIter != egressBufferPoolHandle_.end()) {
    poolIter->second->bufferPool->updateStats();
    auto counters = poolIter->second->bufferPool->getStats();
    deviceWatermarkBytes_ = counters[SAI_BUFFER_POOL_STAT_WATERMARK_BYTES];
  }
}

void SaiBufferManager::updateIngressBufferPoolStats() {
  auto ingressBufferPoolHandle = getIngressBufferPoolHandle();
  if (!ingressBufferPoolHandle) {
    return;
  }
  static std::vector<sai_stat_id_t> counterIdsToReadAndClear;
  if (!counterIdsToReadAndClear.size()) {
    // TODO: Request for per ITM buffer pool stats in SAI
    counterIdsToReadAndClear.push_back(SAI_BUFFER_POOL_STAT_WATERMARK_BYTES);
    if ((platform_->getAsic()->getAsicType() ==
         cfg::AsicType::ASIC_TYPE_JERICHO2) ||
        (platform_->getAsic()->getAsicType() ==
         cfg::AsicType::ASIC_TYPE_JERICHO3)) {
      // TODO: Wait for the fix for CS00012274607 to enable this for all!
      counterIdsToReadAndClear.push_back(
          SAI_BUFFER_POOL_STAT_XOFF_ROOM_WATERMARK_BYTES);
    }
  }
  ingressBufferPoolHandle->bufferPool->updateStats(
      counterIdsToReadAndClear, SAI_STATS_MODE_READ_AND_CLEAR);
  auto counters = ingressBufferPoolHandle->bufferPool->getStats();
  // Store the buffer pool watermarks!
  globalSharedWatermarkBytes_[ingressBufferPoolHandle->bufferPoolName] =
      counters[SAI_BUFFER_POOL_STAT_WATERMARK_BYTES];
  globalHeadroomWatermarkBytes_[ingressBufferPoolHandle->bufferPoolName] =
      counters[SAI_BUFFER_POOL_STAT_XOFF_ROOM_WATERMARK_BYTES];
  publishGlobalWatermarks(
      globalHeadroomWatermarkBytes_[ingressBufferPoolHandle->bufferPoolName],
      globalSharedWatermarkBytes_[ingressBufferPoolHandle->bufferPoolName]);

  if (platform_->getAsic()->isSupported(
          HwAsic::Feature::SHARED_INGRESS_EGRESS_BUFFER_POOL)) {
    /*
     * There is only a single buffer pool for these devices and hence the
     * same stats needs to be updated as device watermark as well.
     */
    deviceWatermarkBytes_ = counters[SAI_BUFFER_POOL_STAT_WATERMARK_BYTES];
  }
}

void SaiBufferManager::updateStats() {
  updateIngressBufferPoolStats();
  updateEgressBufferPoolStats();
  // Device watermarks are collected from ingress for some and egress for
  // some other platforms. This approach is used as this is the best way
  // to get device/switch level watermarks. Do not publish this watermark
  // here. Instead, publish this from SwitchManager to keep all the switch
  // level watermarks in the same place.
}

const std::vector<sai_stat_id_t>&
SaiBufferManager::supportedIngressPriorityGroupWatermarkStats() const {
  static std::vector<sai_stat_id_t> stats;
  if (stats.size()) {
    // initialized
    return stats;
  }
  stats.insert(
      stats.end(),
      SaiIngressPriorityGroupTraits::CounterIdsToReadAndClear.begin(),
      SaiIngressPriorityGroupTraits::CounterIdsToReadAndClear.end());
  if (platform_->getAsic()->isSupported(
          HwAsic::Feature::INGRESS_PRIORITY_GROUP_SHARED_WATERMARK)) {
    stats.emplace_back(SAI_INGRESS_PRIORITY_GROUP_STAT_SHARED_WATERMARK_BYTES);
  }
  if (platform_->getAsic()->isSupported(
          HwAsic::Feature::INGRESS_PRIORITY_GROUP_HEADROOM_WATERMARK)) {
    stats.emplace_back(
        SAI_INGRESS_PRIORITY_GROUP_STAT_XOFF_ROOM_WATERMARK_BYTES);
  }
  return stats;
}

const std::vector<sai_stat_id_t>&
SaiBufferManager::supportedIngressPriorityGroupNonWatermarkStats() const {
  static std::vector<sai_stat_id_t> stats;
  if (!stats.size()) {
    stats.insert(
        stats.end(),
        SaiIngressPriorityGroupTraits::CounterIdsToRead.begin(),
        SaiIngressPriorityGroupTraits::CounterIdsToRead.end());
    if (platform_->getAsic()->isSupported(
            HwAsic::Feature::INGRESS_PRIORITY_GROUP_DROPPED_PACKETS)) {
      stats.push_back(SAI_INGRESS_PRIORITY_GROUP_STAT_DROPPED_PACKETS);
    }
  }
  return stats;
}

void SaiBufferManager::updateIngressPriorityGroupWatermarkStats(
    const std::shared_ptr<SaiIngressPriorityGroup>& ingressPriorityGroup,
    const IngressPriorityGroupID& pgId,
    const HwPortStats& hwPortStats) {
  const auto& ingressPriorityGroupWatermarkStats =
      supportedIngressPriorityGroupWatermarkStats();
  const std::string& portName = *hwPortStats.portName_();
  ingressPriorityGroup->updateStats(
      ingressPriorityGroupWatermarkStats, SAI_STATS_MODE_READ_AND_CLEAR);
  auto counters = ingressPriorityGroup->getStats();
  auto iter =
      counters.find(SAI_INGRESS_PRIORITY_GROUP_STAT_SHARED_WATERMARK_BYTES);
  auto maxPgSharedBytes = iter != counters.end() ? iter->second : 0;
  iter =
      counters.find(SAI_INGRESS_PRIORITY_GROUP_STAT_XOFF_ROOM_WATERMARK_BYTES);
  auto maxPgHeadroomBytes = iter != counters.end() ? iter->second : 0;
  publishPgWatermarks(portName, pgId, maxPgHeadroomBytes, maxPgSharedBytes);
}

void SaiBufferManager::updateIngressPriorityGroupNonWatermarkStats(
    const std::shared_ptr<SaiIngressPriorityGroup>& ingressPriorityGroup,
    const IngressPriorityGroupID& /*pgId*/,
    HwPortStats& hwPortStats) {
  const auto& ingressPriorityGroupStats =
      supportedIngressPriorityGroupNonWatermarkStats();
  ingressPriorityGroup->updateStats(
      ingressPriorityGroupStats, SAI_STATS_MODE_READ);
  auto counters = ingressPriorityGroup->getStats();
  auto iter = counters.find(SAI_INGRESS_PRIORITY_GROUP_STAT_DROPPED_PACKETS);
  if (iter != counters.end()) {
    // In Congestion Discards are exposed at port level
    *hwPortStats.inCongestionDiscards_() += iter->second;
  }
}

void SaiBufferManager::updateIngressPriorityGroupStats(
    const PortID& portId,
    HwPortStats& hwPortStats,
    bool updateWatermarks) {
  SaiPortHandle* portHandle =
      managerTable_->portManager().getPortHandle(portId);
  for (const auto& ipgInfo : portHandle->configuredIngressPriorityGroups) {
    const auto& ingressPriorityGroup =
        ipgInfo.second.pgHandle->ingressPriorityGroup;
    if (updateWatermarks) {
      updateIngressPriorityGroupWatermarkStats(
          ingressPriorityGroup, ipgInfo.first, hwPortStats);
    }
    updateIngressPriorityGroupNonWatermarkStats(
        ingressPriorityGroup, ipgInfo.first, hwPortStats);
  }
}

SaiBufferProfileTraits::CreateAttributes SaiBufferManager::profileCreateAttrs(
    const PortQueue& queue) const {
  SaiBufferProfileTraits::Attributes::PoolId pool{
      getEgressBufferPoolHandle(queue)->bufferPool->adapterKey()};
  std::optional<SaiBufferProfileTraits::Attributes::ReservedBytes>
      reservedBytes;
  if (queue.getReservedBytes()) {
    reservedBytes = queue.getReservedBytes().value();
  } else {
    // don't set nullopt for reserved bytes as SAI always return non-null
    // value during get/warmboot
    reservedBytes = 0;
  }
  SaiBufferProfileTraits::Attributes::ThresholdMode mode{
      SAI_BUFFER_PROFILE_THRESHOLD_MODE_DYNAMIC};
  SaiBufferProfileTraits::Attributes::SharedDynamicThreshold dynThresh{0};
  if (platform_->getAsic()->scalingFactorBasedDynamicThresholdSupported() &&
      queue.getScalingFactor()) {
    dynThresh = platform_->getAsic()->getBufferDynThreshFromScalingFactor(
        queue.getScalingFactor().value());
  }
  std::optional<SaiBufferProfileTraits::Attributes::SharedFadtMaxTh>
      sharedFadtMaxTh;
#if defined(BRCM_SAI_SDK_DNX_GTE_11_0)
  if (queue.getMaxDynamicSharedBytes()) {
    sharedFadtMaxTh = queue.getMaxDynamicSharedBytes().value();
  } else {
    // use default value 0
    sharedFadtMaxTh = 0;
  }
#endif
  return SaiBufferProfileTraits::CreateAttributes{
      pool, reservedBytes, mode, dynThresh, 0, 0, 0, sharedFadtMaxTh};
}

void SaiBufferManager::setupBufferPool(
    const state::PortPgFields& portPgConfig) {
  if (!portPgConfig.bufferPoolName().has_value() ||
      !portPgConfig.bufferPoolConfig().has_value()) {
    throw FbossError(
        "Ingress buffer pool config should have pool name and config specified!");
  }
  if (platform_->getAsic()->isSupported(
          HwAsic::Feature::SHARED_INGRESS_EGRESS_BUFFER_POOL)) {
    setupIngressEgressBufferPool(
        *portPgConfig.bufferPoolName(), *portPgConfig.bufferPoolConfig());
  } else {
    // Call ingress or egress buffer pool based on the cfg passed in!
    setupIngressBufferPool(
        *portPgConfig.bufferPoolName(), *portPgConfig.bufferPoolConfig());
  }
}

std::shared_ptr<SaiBufferProfile> SaiBufferManager::getOrCreateProfile(
    const PortQueue& queue) {
  setupBufferPool(queue);
  // TODO throw error if shared bytes is set. We don't handle that in SAI
  auto attributes = profileCreateAttrs(queue);
  auto& store = saiStore_->get<SaiBufferProfileTraits>();
  SaiBufferProfileTraits::AdapterHostKey k = tupleProjection<
      SaiBufferProfileTraits::CreateAttributes,
      SaiBufferProfileTraits::AdapterHostKey>(attributes);
  return store.setObject(k, attributes);
}

SaiBufferProfileTraits::CreateAttributes
SaiBufferManager::ingressProfileCreateAttrs(
    const state::PortPgFields& config) const {
  SaiBufferProfileTraits::Attributes::PoolId pool{
      getIngressBufferPoolHandle()->bufferPool->adapterKey()};
  SaiBufferProfileTraits::Attributes::ReservedBytes reservedBytes =
      *config.minLimitBytes();
  SaiBufferProfileTraits::Attributes::ThresholdMode mode{
      SAI_BUFFER_PROFILE_THRESHOLD_MODE_DYNAMIC};
  SaiBufferProfileTraits::Attributes::SharedDynamicThreshold dynThresh{0};
  if (config.scalingFactor() &&
      platform_->getAsic()->scalingFactorBasedDynamicThresholdSupported()) {
    // If scalingFactor is specified, configure the same!
    dynThresh = platform_->getAsic()->getBufferDynThreshFromScalingFactor(
        nameToEnum<cfg::MMUScalingFactor>(*config.scalingFactor()));
  }
  SaiBufferProfileTraits::Attributes::XoffTh xoffTh{0};
  if (config.headroomLimitBytes()) {
    xoffTh = *config.headroomLimitBytes();
  }
  SaiBufferProfileTraits::Attributes::XonTh xonTh{0}; // Not configured!
  SaiBufferProfileTraits::Attributes::XonOffsetTh xonOffsetTh{0};
  if (config.resumeOffsetBytes()) {
    xonOffsetTh = *config.resumeOffsetBytes();
  }
  std::optional<SaiBufferProfileTraits::Attributes::SharedFadtMaxTh>
      sharedFadtMaxTh;
#if defined(BRCM_SAI_SDK_DNX_GTE_11_0)
  // use default 0 since this attribute currently only used by profile for
  // cpu/eventor/rcy port queues
  sharedFadtMaxTh = 0;
#endif
  return SaiBufferProfileTraits::CreateAttributes{
      pool,
      reservedBytes,
      mode,
      dynThresh,
      xoffTh,
      xonTh,
      xonOffsetTh,
      sharedFadtMaxTh};
}

std::shared_ptr<SaiBufferProfile> SaiBufferManager::getOrCreateIngressProfile(
    const state::PortPgFields& portPgConfig) {
  if (!portPgConfig.bufferPoolConfig()) {
    XLOG(FATAL) << "Empty buffer pool config from portPgConfig.";
  }
  setupBufferPool(portPgConfig);
  auto attributes = ingressProfileCreateAttrs(portPgConfig);
  auto& store = saiStore_->get<SaiBufferProfileTraits>();
  SaiBufferProfileTraits::AdapterHostKey k = tupleProjection<
      SaiBufferProfileTraits::CreateAttributes,
      SaiBufferProfileTraits::AdapterHostKey>(attributes);
  return store.setObject(k, attributes);
}

void SaiBufferManager::setIngressPriorityGroupBufferProfile(
    const std::shared_ptr<SaiIngressPriorityGroup> ingressPriorityGroup,
    std::shared_ptr<SaiBufferProfile> bufferProfile) {
  if (bufferProfile) {
    ingressPriorityGroup->setOptionalAttribute(
        SaiIngressPriorityGroupTraits::Attributes::BufferProfile{
            bufferProfile->adapterKey()});
  } else {
    ingressPriorityGroup->setOptionalAttribute(
        SaiIngressPriorityGroupTraits::Attributes::BufferProfile{
            SAI_NULL_OBJECT_ID});
  }
}

SaiIngressPriorityGroupHandles SaiBufferManager::loadIngressPriorityGroups(
    const std::vector<IngressPriorityGroupSaiId>& ingressPriorityGroupSaiIds) {
  SaiIngressPriorityGroupHandles ingressPriorityGroupHandles;
  auto& store = saiStore_->get<SaiIngressPriorityGroupTraits>();

  for (auto ingressPriorityGroupSaiId : ingressPriorityGroupSaiIds) {
    auto ingressPriorityGroupHandle =
        std::make_unique<SaiIngressPriorityGroupHandle>();
    ingressPriorityGroupHandle->ingressPriorityGroup =
        store.loadObjectOwnedByAdapter(
            SaiIngressPriorityGroupTraits::AdapterKey{
                ingressPriorityGroupSaiId});
    auto index = SaiApiTable::getInstance()->bufferApi().getAttribute(
        ingressPriorityGroupSaiId,
        SaiIngressPriorityGroupTraits::Attributes::Index{});
    ingressPriorityGroupHandles[index] = std::move(ingressPriorityGroupHandle);
  }
  return ingressPriorityGroupHandles;
}

} // namespace facebook::fboss
