/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#pragma once

#include "fboss/agent/hw/sai/api/BufferApi.h"
#include "fboss/agent/hw/sai/api/Types.h"
#include "fboss/agent/hw/sai/store/SaiObjectWithCounters.h"
#include "fboss/agent/state/PortPgConfig.h"
#include "fboss/agent/types.h"
#include "fboss/lib/RefMap.h"

#include <memory>

namespace facebook::fboss {

class SaiManagerTable;
class SaiPlatform;
class Port;
class PortQueue;
class PortPgConfig;
class HwAsic;
class SaiStore;

using SaiBufferPool = SaiObjectWithCounters<SaiBufferPoolTraits>;

using SaiBufferProfile = std::variant<
    std::shared_ptr<SaiObject<SaiStaticBufferProfileTraits>>,
    std::shared_ptr<SaiObject<SaiDynamicBufferProfileTraits>>>;

struct SaiBufferProfileHandle {
  explicit SaiBufferProfileHandle(SaiBufferProfile bufferProfile)
      : bufferProfile(std::move(bufferProfile)) {}

  BufferProfileSaiId adapterKey() const {
    return std::visit(
        [](auto& handle) { return handle->adapterKey(); }, bufferProfile);
  }

  SaiBufferProfile bufferProfile;
};

using SaiIngressPriorityGroup =
    SaiObjectWithCounters<SaiIngressPriorityGroupTraits>;

struct SaiBufferPoolHandle {
  std::shared_ptr<SaiBufferPool> bufferPool;
  std::string bufferPoolName;
};

struct SaiIngressPriorityGroupHandle {
  std::shared_ptr<SaiIngressPriorityGroup> ingressPriorityGroup;
};

using SaiIngressPriorityGroupHandles =
    folly::F14FastMap<int, std::unique_ptr<SaiIngressPriorityGroupHandle>>;

struct SaiIngressPriorityGroupHandleAndProfile {
  std::unique_ptr<SaiIngressPriorityGroupHandle> pgHandle;
  std::shared_ptr<SaiBufferProfileHandle> bufferProfile;
};

class SaiBufferManager {
 public:
  SaiBufferManager(
      SaiStore* saiStore,
      SaiManagerTable* managerTable,
      const SaiPlatform* platform);

  std::shared_ptr<SaiBufferProfileHandle> getOrCreateProfile(
      const PortQueue& queue,
      cfg::PortType type);
  std::shared_ptr<SaiBufferProfileHandle> getOrCreateIngressProfile(
      const state::PortPgFields& portPgConfig);

  void updateStats();
  void updateIngressPriorityGroupStats(
      const PortID& portId,
      HwPortStats& hwPortStats,
      bool updateWatermarks);
  uint64_t getDeviceWatermarkBytes() const {
    return deviceWatermarkBytes_;
  }
  const std::map<std::string, uint64_t>& getGlobalHeadroomWatermarkBytes()
      const {
    return globalHeadroomWatermarkBytes_;
  }
  const std::map<std::string, uint64_t>& getGlobalSharedWatermarkBytes() const {
    return globalSharedWatermarkBytes_;
  }
  void setIngressPriorityGroupBufferProfile(
      const std::shared_ptr<SaiIngressPriorityGroup> ingressPriorityGroup,
      std::shared_ptr<SaiBufferProfileHandle> bufferProfile);
  void setIngressPriorityGroupLosslessEnable(
      const std::shared_ptr<SaiIngressPriorityGroup> ingressPriorityGroup,
      bool isLossless);
  SaiIngressPriorityGroupHandles loadIngressPriorityGroups(
      const std::vector<IngressPriorityGroupSaiId>& ingressPriorityGroupSaiIds);
  SaiBufferPoolHandle* getIngressBufferPoolHandle() const;
  static uint64_t getMaxEgressPoolBytes(const SaiPlatform* platform);

 private:
  void setupBufferPool(const PortQueue& queue);
  void setupBufferPool(const state::PortPgFields& portPgConfig);
  void loadCpuPortEgressBufferPool();

  void updateIngressBufferPoolStats();
  void updateEgressBufferPoolStats();

  void updateIngressPriorityGroupWatermarkStats(
      const std::shared_ptr<SaiIngressPriorityGroup>& ingressPriorityGroup,
      const IngressPriorityGroupID& pgId,
      const HwPortStats& hwPortStats);
  void updateIngressPriorityGroupNonWatermarkStats(
      const std::shared_ptr<SaiIngressPriorityGroup>& ingressPriorityGroup,
      const IngressPriorityGroupID& pgId,
      HwPortStats& hwPortStats);

  void publishGlobalWatermarks(
      const uint64_t& globalHeadroomBytes,
      const uint64_t& globalSharedBytes) const;
  void publishPgWatermarks(
      const std::string& portName,
      const int& pg,
      const uint64_t& pgHeadroomBytes,
      const uint64_t& pgSharedBytes) const;
  template <typename BufferProfileTraits>
  BufferProfileTraits::CreateAttributes profileCreateAttrs(
      const PortQueue& queue,
      cfg::PortType type) const;
  template <typename BufferProfileTraits>
  BufferProfileTraits::CreateAttributes profileCreateAttrs(
      typename BufferProfileTraits::Attributes::PoolId poolId,
      const PortQueue& queue,
      cfg::PortType type) const;
  template <typename BufferProfileTraits>
  BufferProfileTraits::CreateAttributes ingressProfileCreateAttrs(
      const state::PortPgFields& config) const;
  void setBufferPoolXoffSize(
      std::shared_ptr<SaiBufferPoolHandle> bufferPoolHandle,
      const PortPgConfig* portPgCfg);
  void setupEgressBufferPool();
  void setupEgressBufferPool(
      const std::optional<BufferPoolFields>& bufferPoolCfg);
  void setupIngressBufferPool(
      const std::string& bufferPoolName,
      const BufferPoolFields& bufferPoolCfg);
  void setupIngressEgressBufferPool(
      const std::optional<std::string>& bufferPoolName = std::nullopt,
      const std::optional<BufferPoolFields>& ingressPgCfg = std::nullopt);
  void createOrUpdateIngressEgressBufferPool(
      uint64_t poolSize,
      std::optional<int32_t> newXoffSize);
  SaiBufferPoolHandle* getEgressBufferPoolHandle(
      const PortQueue& queue,
      cfg::PortType type) const;
  const std::vector<sai_stat_id_t>&
  supportedIngressPriorityGroupWatermarkStats() const;
  const std::vector<sai_stat_id_t>&
  supportedIngressPriorityGroupNonWatermarkStats() const;

  SaiStore* saiStore_;
  SaiManagerTable* managerTable_;
  const SaiPlatform* platform_;
  std::map<std::string, std::unique_ptr<SaiBufferPoolHandle>>
      egressBufferPoolHandle_;
  std::unique_ptr<SaiBufferPoolHandle> ingressBufferPoolHandle_;
  std::unique_ptr<SaiBufferPoolHandle> ingressEgressBufferPoolHandle_;
  UnorderedRefMap<SaiBufferProfileTraits::AdapterHostKey, SaiBufferProfile>
      bufferProfiles_;
  uint64_t deviceWatermarkBytes_{0};
  std::map<std::string, uint64_t> globalHeadroomWatermarkBytes_{};
  std::map<std::string, uint64_t> globalSharedWatermarkBytes_{};
  std::unique_ptr<SaiBufferPoolHandle> egressCpuPortBufferPoolHandle_;
};

} // namespace facebook::fboss
