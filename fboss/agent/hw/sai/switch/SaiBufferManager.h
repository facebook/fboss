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
using SaiBufferProfile = SaiObject<SaiBufferProfileTraits>;
using SaiIngressPriorityGroup =
    SaiObjectWithCounters<SaiIngressPriorityGroupTraits>;

struct SaiBufferPoolHandle {
  std::shared_ptr<SaiBufferPool> bufferPool;
};

struct SaiIngressPriorityGroupHandle {
  std::shared_ptr<SaiIngressPriorityGroup> ingressPriorityGroup;
};

using SaiIngressPriorityGroupHandles =
    folly::F14FastMap<int, std::unique_ptr<SaiIngressPriorityGroupHandle>>;

struct SaiIngressPriorityGroupHandleAndProfile {
  std::unique_ptr<SaiIngressPriorityGroupHandle> pgHandle;
  std::shared_ptr<SaiBufferProfile> bufferProfile;
};

class SaiBufferManager {
 public:
  SaiBufferManager(
      SaiStore* saiStore,
      SaiManagerTable* managerTable,
      const SaiPlatform* platform);

  std::shared_ptr<SaiBufferProfile> getOrCreateProfile(const PortQueue& queue);
  std::shared_ptr<SaiBufferProfile> getOrCreateIngressProfile(
      const state::PortPgFields& portPgConfig);

  void setupBufferPool(
      const std::optional<state::BufferPoolFields> ingressPgCfg = std::nullopt);

  void updateStats();
  void updateIngressBufferPoolStats();
  void updateEgressBufferPoolStats();
  void createIngressBufferPool(const std::shared_ptr<Port> port);
  uint64_t getDeviceWatermarkBytes() const {
    return deviceWatermarkBytes_;
  }
  static uint64_t getMaxEgressPoolBytes(const SaiPlatform* platform);
  void setIngressPriorityGroupBufferProfile(
      IngressPriorityGroupSaiId pdId,
      std::shared_ptr<SaiBufferProfile> bufferProfile);
  SaiIngressPriorityGroupHandles loadIngressPriorityGroups(
      const std::vector<IngressPriorityGroupSaiId>& ingressPriorityGroupSaiIds);

 private:
  void publishDeviceWatermark(uint64_t peakBytes) const;
  void publishGlobalWatermarks(
      const uint64_t& globalHeadroomBytes,
      const uint64_t& globalSharedBytes) const;
  void publishPgWatermarks(
      const std::string& portName,
      const int& pg,
      const uint64_t& pgHeadroomBytes,
      const uint64_t& pgSharedBytes) const;
  SaiBufferProfileTraits::CreateAttributes profileCreateAttrs(
      const PortQueue& queue) const;
  SaiBufferProfileTraits::CreateAttributes ingressProfileCreateAttrs(
      const state::PortPgFields& config) const;
  void setBufferPoolXoffSize(
      std::shared_ptr<SaiBufferPoolHandle> bufferPoolHandle,
      const PortPgConfig* portPgCfg);
  void setupEgressBufferPool();
  void setupIngressBufferPool(const state::BufferPoolFields& bufferPoolCfg);
  void setupIngressEgressBufferPool(
      const std::optional<state::BufferPoolFields> ingressPgCfg);
  SaiBufferPoolHandle* getEgressBufferPoolHandle() const;
  SaiBufferPoolHandle* getIngressBufferPoolHandle() const;

  SaiStore* saiStore_;
  SaiManagerTable* managerTable_;
  const SaiPlatform* platform_;
  std::unique_ptr<SaiBufferPoolHandle> egressBufferPoolHandle_;
  std::unique_ptr<SaiBufferPoolHandle> ingressBufferPoolHandle_;
  std::unique_ptr<SaiBufferPoolHandle> ingressEgressBufferPoolHandle_;
  UnorderedRefMap<SaiBufferProfileTraits::AdapterHostKey, SaiBufferProfile>
      bufferProfiles_;
  uint64_t deviceWatermarkBytes_{0};
};

} // namespace facebook::fboss
