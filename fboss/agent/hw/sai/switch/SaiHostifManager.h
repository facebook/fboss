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

#include "fboss/agent/hw/gen-cpp2/hardware_stats_types.h"

#include "fboss/agent/hw/HwCpuFb303Stats.h"
#include "fboss/agent/hw/sai/api/CounterApi.h"
#include "fboss/agent/hw/sai/api/HostifApi.h"
#include "fboss/agent/hw/sai/store/SaiObject.h"
#include "fboss/agent/hw/sai/switch/SaiPortManager.h"
#include "fboss/agent/state/ControlPlane.h"
#include "fboss/agent/state/StateDelta.h"
#include "fboss/agent/types.h"
#include "fboss/lib/RefMap.h"

#include "folly/container/F14Map.h"

#include <memory>
#include <mutex>

namespace facebook::fboss {

struct ConcurrentIndices;
class SaiManagerTable;
class SaiPlatform;
class SaiStore;

using SaiHostifTrapGroup = SaiObject<SaiHostifTrapGroupTraits>;
using SaiHostifTrap = SaiObject<SaiHostifTrapTraits>;
using SaiHostifTrapCounter = SaiObject<SaiCounterTraits>;

struct SaiCpuPortHandle {
  PortSaiId cpuPortId;
  SaiQueueHandles queues;
  std::vector<SaiQueueHandle*> configuredQueues;
};

struct SaiHostifTrapHandle {
  std::shared_ptr<SaiHostifTrapGroup> trapGroup;
  std::shared_ptr<SaiHostifTrap> trap;
  std::shared_ptr<SaiHostifTrapCounter> counter;
};

class SaiHostifManager {
 public:
  explicit SaiHostifManager(
      SaiStore* saiStore,
      SaiManagerTable* managerTable,
      const SaiPlatform* platform,
      ConcurrentIndices* concurrentIndices);
  ~SaiHostifManager();
  HostifTrapSaiId addHostifTrap(
      cfg::PacketRxReason trapId,
      uint32_t queueId,
      uint16_t priority);
  void removeHostifTrap(cfg::PacketRxReason trapId);
  void changeHostifTrap(
      cfg::PacketRxReason trapId,
      uint32_t queueId,
      uint16_t priority);
  static std::pair<sai_hostif_trap_type_t, sai_packet_action_t>
  packetReasonToHostifTrap(
      cfg::PacketRxReason reason,
      const SaiPlatform* platform);
  static SaiHostifTrapTraits::CreateAttributes makeHostifTrapAttributes(
      cfg::PacketRxReason trapId,
      HostifTrapGroupSaiId trapGroupId,
      uint16_t priority,
      const SaiPlatform* platform);
  void processHostifDelta(const DeltaValue<ControlPlane>& delta);
  SaiQueueHandle* getQueueHandle(const SaiQueueConfig& saiQueueConfig);
  const SaiQueueHandle* getQueueHandle(
      const SaiQueueConfig& saiQueueConfig) const;
  void updateStats(bool updateWatermarks = false);
  HwPortStats getCpuPortStats() const;
  QueueConfig getQueueSettings() const;
  const HwCpuFb303Stats& getCpuFb303Stats() const {
    return cpuStats_;
  }
  const SaiHostifTrapHandle* getHostifTrapHandle(
      cfg::PacketRxReason rxReason) const;
  SaiHostifTrapHandle* getHostifTrapHandle(cfg::PacketRxReason rxReason);
  std::shared_ptr<SaiHostifTrapCounter> createHostifTrapCounter(
      cfg::PacketRxReason rxReason);

 private:
  uint32_t getMaxCpuQueues() const;
  void setQosPolicy();
  void clearQosPolicy();
  void setCpuQosPolicy(QosMapSaiId dscpToTc, QosMapSaiId tcToQueue);
  std::shared_ptr<SaiHostifTrapGroup> ensureHostifTrapGroup(uint32_t queueId);
  void processQueueDelta(const DeltaValue<ControlPlane>& delta);
  void processRxReasonToQueueDelta(const DeltaValue<ControlPlane>& delta);
  void processQosDelta(const DeltaValue<ControlPlane>& delta);

  void loadCpuPort();
  void loadCpuPortQueues();
  void changeCpuQueue(
      const ControlPlane::PortQueues& oldQueueConfig,
      const ControlPlane::PortQueues& newQueueConfig);
  SaiQueueHandle* getQueueHandleImpl(
      const SaiQueueConfig& saiQueueConfig) const;
  SaiHostifTrapHandle* getHostifTrapHandleImpl(
      cfg::PacketRxReason rxReason) const;
  void publishCpuQueueWatermark(int cosq, uint64_t peakBytes) const;

  SaiStore* saiStore_;
  SaiManagerTable* managerTable_;
  const SaiPlatform* platform_;
  ConcurrentIndices* concurrentIndices_;
  folly::F14FastMap<cfg::PacketRxReason, std::unique_ptr<SaiHostifTrapHandle>>
      handles_;
  /*
   * We only permit a single QoS policy across the
   * front panel and CPU ports. So when set, these
   * pointers hold references to that policy
   */
  std::shared_ptr<SaiQosMap> globalDscpToTcQosMap_;
  std::shared_ptr<SaiQosMap> globalTcToQueueQosMap_;
  std::unique_ptr<SaiCpuPortHandle> cpuPortHandle_;
  HwCpuFb303Stats cpuStats_;
  HwRxReasonStats rxReasonStats_;
};

} // namespace facebook::fboss
