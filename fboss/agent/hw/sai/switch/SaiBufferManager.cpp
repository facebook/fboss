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
    case cfg::AsicType::ASIC_TYPE_ELBERT_8DD:
    case cfg::AsicType::ASIC_TYPE_SANDIA_PHY:
    case cfg::AsicType::ASIC_TYPE_RAMON:
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

void SaiBufferManager::setupIngressBufferPool(
    const state::BufferPoolFields& bufferPoolCfg) {
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
#if defined(TAJO_SDK) || defined(SAI_VERSION_8_2_0_0_ODP) ||                   \
    defined(SAI_VERSION_8_2_0_0_SIM_ODP) ||                                    \
    defined(SAI_VERSION_8_2_0_0_DNX_ODP) || defined(SAI_VERSION_9_0_EA_ODP) || \
    defined(SAI_VERSION_9_0_EA_SIM_ODP) ||                                     \
    defined(SAI_VERSION_9_0_EA_DNX_ODP) ||                                     \
    defined(SAI_VERSION_9_0_EA_DNX_SIM_ODP)
  if (platform_->getAsic()->isSupported(HwAsic::Feature::PFC)) {
    xoffSize = *bufferPoolCfg.headroomBytes() *
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
    const std::optional<state::BufferPoolFields> bufferPoolCfg) {
  std::optional<SaiBufferPoolTraits::Attributes::XoffSize> xoffSize;
  if (!ingressEgressBufferPoolHandle_) {
    auto availableBuffer = getSwitchEgressPoolAvailableSize(platform_);
    uint64_t poolSize;
    if (FLAGS_ingress_egress_buffer_pool_size) {
      // An option for test to override the buffer pool size to be used.
      poolSize = FLAGS_ingress_egress_buffer_pool_size *
          platform_->getAsic()->getNumMemoryBuffers();
    } else {
      poolSize = availableBuffer * platform_->getAsic()->getNumMemoryBuffers();
    }

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
  if (bufferPoolCfg &&
      platform_->getAsic()->isSupported(HwAsic::Feature::PFC)) {
#if defined(TAJO_SDK) || defined(SAI_VERSION_8_2_0_0_ODP) ||                   \
    defined(SAI_VERSION_8_2_0_0_DNX_ODP) || defined(SAI_VERSION_9_0_EA_ODP) || \
    defined(SAI_VERSION_9_0_EA_DNX_ODP)
    SaiBufferPoolTraits::Attributes::XoffSize xoffSize =
        *(*bufferPoolCfg).headroomBytes() *
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

void SaiBufferManager::updateEgressBufferPoolStats() {
  if (!egressBufferPoolHandle_) {
    // Applies to platforms with SHARED_INGRESS_EGRESS_BUFFER_POOL, where
    // watermarks are polled as part of ingress itself.
    return;
  }
  egressBufferPoolHandle_->bufferPool->updateStats();
  auto counters = egressBufferPoolHandle_->bufferPool->getStats();
  deviceWatermarkBytes_ = counters[SAI_BUFFER_POOL_STAT_WATERMARK_BYTES];
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
  auto maxGlobalSharedBytes = counters[SAI_BUFFER_POOL_STAT_WATERMARK_BYTES];
  auto maxGlobalHeadroomBytes =
      counters[SAI_BUFFER_POOL_STAT_XOFF_ROOM_WATERMARK_BYTES];
  publishGlobalWatermarks(maxGlobalHeadroomBytes, maxGlobalSharedBytes);

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
  // some other platforms, hence publish it here.
  publishDeviceWatermark(deviceWatermarkBytes_);
}

void SaiBufferManager::updateIngressPriorityGroupStats(
    const PortID& portId,
    const std::string& portName,
    bool updateWatermarks) {
  /*
   * As of now, only watermark stats are supported for IngressPriorityGroup.
   * Hence returning here if watermark stats collection is not needed. Will
   * need modifications to this code if we enable non-watermark stats as well,
   * to poll watermark and non-watermark stats differently based on the
   * updateWatermarks option.
   */
  if (!updateWatermarks) {
    return;
  }
  SaiPortHandle* portHandle =
      managerTable_->portManager().getPortHandle(portId);
  static std::vector<sai_stat_id_t> ingressPriorityGroupWatermarkStats(
      SaiIngressPriorityGroupTraits::CounterIdsToReadAndClear.begin(),
      SaiIngressPriorityGroupTraits::CounterIdsToReadAndClear.end());
  // TODO: Only DNX supports watermarks as of now, this would need
  // modifications once XGS side support is in place.
  if (platform_->getAsic()->getAsicType() ==
          cfg::AsicType::ASIC_TYPE_JERICHO2 ||
      platform_->getAsic()->getAsicType() ==
          cfg::AsicType::ASIC_TYPE_JERICHO3) {
    ingressPriorityGroupWatermarkStats.emplace_back(
        SAI_INGRESS_PRIORITY_GROUP_STAT_SHARED_WATERMARK_BYTES);
  }
  for (const auto& ipgInfo : portHandle->configuredIngressPriorityGroups) {
    const auto& ingressPriorityGroup =
        ipgInfo.second.pgHandle->ingressPriorityGroup;
    ingressPriorityGroup->updateStats(
        ingressPriorityGroupWatermarkStats, SAI_STATS_MODE_READ_AND_CLEAR);
    auto counters = ingressPriorityGroup->getStats();
    auto iter =
        counters.find(SAI_INGRESS_PRIORITY_GROUP_STAT_SHARED_WATERMARK_BYTES);
    auto maxPgSharedBytes = iter != counters.end() ? iter->second : 0;
    iter = counters.find(
        SAI_INGRESS_PRIORITY_GROUP_STAT_XOFF_ROOM_WATERMARK_BYTES);
    auto maxPgHeadroomBytes = iter != counters.end() ? iter->second : 0;
    publishPgWatermarks(
        portName, ipgInfo.first, maxPgHeadroomBytes, maxPgSharedBytes);
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

void SaiBufferManager::setupBufferPool(
    std::optional<state::BufferPoolFields> bufferPoolCfg) {
  if (platform_->getAsic()->isSupported(
          HwAsic::Feature::SHARED_INGRESS_EGRESS_BUFFER_POOL)) {
    setupIngressEgressBufferPool(bufferPoolCfg);
  } else {
    // Call ingress or egress buffer pool based on the cfg passed in!
    if (bufferPoolCfg) {
      setupIngressBufferPool(*bufferPoolCfg);
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
  return SaiBufferProfileTraits::CreateAttributes{
      pool, reservedBytes, mode, dynThresh, xoffTh, xonTh, xonOffsetTh};
}

std::shared_ptr<SaiBufferProfile> SaiBufferManager::getOrCreateIngressProfile(
    const state::PortPgFields& portPgConfig) {
  if (!portPgConfig.bufferPoolConfig()) {
    XLOG(FATAL) << "Empty buffer pool config from portPgConfig.";
  }
  setupBufferPool(*portPgConfig.bufferPoolConfig());
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
    if (portPgCfgs) {
      const auto& portPgCfg = (*portPgCfgs).at(0);
      // THRIFT_COPY
      setupBufferPool(
          portPgCfg->cref<switch_state_tags::bufferPoolConfig>()->toThrift());
    }
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
