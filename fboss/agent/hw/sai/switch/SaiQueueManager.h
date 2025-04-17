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
#include "fboss/agent/hw/sai/api/QueueApi.h"
#include "fboss/agent/hw/sai/store/SaiObjectWithCounters.h"
#include "fboss/agent/hw/sai/store/SaiStore.h"
#include "fboss/agent/hw/sai/switch/SaiBufferManager.h"
#include "fboss/agent/hw/sai/switch/SaiSchedulerManager.h"
#include "fboss/agent/hw/sai/switch/SaiWredManager.h"
#include "fboss/agent/state/Port.h"
#include "fboss/agent/state/PortQueue.h"
#include "fboss/agent/state/StateDelta.h"
#include "fboss/agent/types.h"

#include "folly/container/F14Map.h"

#include <memory>

namespace facebook::fboss {

class SaiManagerTable;
class SaiPlatform;
class HwPortStats;
class SaiStore;

using SaiQueue = SaiObjectWithCounters<SaiQueueTraits>;
using SaiQueueConfig = std::pair<uint8_t, cfg::StreamType>;

struct SaiQueueHandle {
  explicit SaiQueueHandle(std::shared_ptr<SaiQueue> queue) : queue(queue) {}
  ~SaiQueueHandle();
  void resetQueue();
  std::shared_ptr<SaiScheduler> scheduler;
  std::shared_ptr<SaiWred> wredProfile;
  std::shared_ptr<SaiBufferProfile> bufferProfile;
  std::shared_ptr<SaiQueue> queue;
};

using SaiQueueHandles =
    folly::F14FastMap<SaiQueueConfig, std::unique_ptr<SaiQueueHandle>>;

bool fillQueueExtensionStats(
    const uint8_t queueId,
    const sai_stat_id_t& counterId,
    const uint64_t& counterValue,
    HwPortStats& hwPortStats);

class SaiQueueManager {
 public:
  SaiQueueManager(
      SaiStore* saiStore,
      SaiManagerTable* managerTable,
      const SaiPlatform* platform);
  SaiQueueHandles loadQueues(const std::vector<QueueSaiId>& queueSaiIds);
  void changeQueue(
      SaiQueueHandle* queueHandle,
      const PortQueue& newPortQueue,
      const Port* swPort = nullptr,
      const std::optional<cfg::PortType> portType = std::nullopt);
  void changeQueueBufferProfile(
      SaiQueueHandle* queueHandle,
      const PortQueue& newPortQueue);
  void changeQueueEcnWred(
      SaiQueueHandle* queueHandle,
      const PortQueue& newPortQueue);
  void changeQueueScheduler(
      SaiQueueHandle* queueHandle,
      const PortQueue& newPortQueue,
      const Port* swPort);
  void changeQueueDeadlockEnable(
      SaiQueueHandle* queueHandle,
      const Port* swPort);
  void queuePfcDeadlockDetectionRecoveryEnable(
      SaiQueueHandle* queueHandle,
      const bool portPfcWdEnabled);
  void ensurePortQueueConfig(
      PortSaiId portSaiId,
      const SaiQueueHandles& queueHandles,
      const QueueConfig& queues,
      const facebook::fboss::Port* swPort = nullptr);
  void updateStats(
      const std::vector<SaiQueueHandle*>& queues,
      HwPortStats& stats,
      bool updateWatermarks);
  void updateStats(
      const std::vector<SaiQueueHandle*>& queues,
      HwSysPortStats& stats,
      bool updateWatermarks,
      bool updateVoqStats);
  void getStats(SaiQueueHandles& queueHandles, HwPortStats& hwPortStats);
  void clearStats(const std::vector<SaiQueueHandle*>& queueHandles);
  QueueConfig getQueueSettings(const SaiQueueHandles& queueHandles) const;
  std::optional<std::tuple<uint8_t, PortSaiId>> getQueueIndexAndPortSaiId(
      const QueueSaiId& queueSaiId);

 private:
  bool isVoqSwitchAndQueueHandleNotForVoq(SaiQueueHandle* queueHandle);
  const std::vector<sai_stat_id_t>& supportedNonWatermarkCounterIdsRead(
      int queueType,
      SaiQueueHandle* queueHandle) const;
  const std::vector<sai_stat_id_t>&
  supportedVoqWatermarkCounterIdsReadAndClear() const;

  const std::vector<sai_stat_id_t>& egressQueueNonWatermarkCounterIdsRead(
      int queueType) const;

  const std::vector<sai_stat_id_t>& voqNonWatermarkCounterIdsRead(
      int queueType,
      SaiQueueHandle* queueHandle) const;

  const std::vector<sai_stat_id_t>& supportedWatermarkCounterIdsReadAndClear(
      int queueType) const;
  SaiStore* saiStore_;
  SaiManagerTable* managerTable_;
  const SaiPlatform* platform_;
};

} // namespace facebook::fboss
