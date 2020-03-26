/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/sai/switch/SaiQueueManager.h"
#include "fboss/agent/hw/sai/store/SaiStore.h"
#include "fboss/agent/hw/sai/switch/SaiManagerTable.h"
#include "fboss/agent/hw/sai/switch/SaiPortManager.h"
#include "fboss/agent/hw/sai/switch/SaiSwitchManager.h"
#include "fboss/lib/TupleUtils.h"

namespace facebook::fboss {

namespace {

void fillHwQueueStats(
    uint8_t queueId,
    const std::vector<uint64_t>& counters,
    HwPortStats& hwPortStats) {
  uint16_t index = 0;
  if (counters.size() != SaiQueueTraits::CounterIds.size()) {
    throw FbossError("queue counter size does not match counter id size");
  }
  for (auto counterId : SaiQueueTraits::CounterIds) {
    switch (counterId) {
      case SAI_QUEUE_STAT_PACKETS:
        hwPortStats.queueOutPackets_[queueId] = counters[index];
        break;
      case SAI_QUEUE_STAT_BYTES:
        hwPortStats.queueOutBytes_[queueId] = counters[index];
        break;
      case SAI_QUEUE_STAT_DROPPED_BYTES:
        hwPortStats.queueOutDiscardBytes_[queueId] = counters[index];
        break;
      case SAI_QUEUE_STAT_DROPPED_PACKETS:
        hwPortStats.queueOutDiscardPackets_[queueId] = counters[index];
        break;
      default:
        throw FbossError("Got unexpected queue counter id: ", counterId);
    }
    index++;
  }
}
} // namespace

namespace detail {

SaiQueueTraits::CreateAttributes makeQueueAttributes(
    PortSaiId portSaiId,
    const PortQueue& portQueue) {
  sai_queue_type_t type;
  if (portQueue.getStreamType() == cfg::StreamType::UNICAST) {
    type = SAI_QUEUE_TYPE_UNICAST;
  } else if (portQueue.getStreamType() == cfg::StreamType::MULTICAST) {
    type = SAI_QUEUE_TYPE_MULTICAST;
  } else {
    type = SAI_QUEUE_TYPE_ALL;
  }
  return SaiQueueTraits::CreateAttributes{
      type, portSaiId, portQueue.getID(), portSaiId};
}

SaiQueueConfig makeSaiQueueConfig(
    const SaiQueueTraits::AdapterHostKey& adapterHostKey) {
  auto queueIndex = std::get<SaiQueueTraits::Attributes::Index>(adapterHostKey);
  auto queueType = std::get<SaiQueueTraits::Attributes::Type>(adapterHostKey);
  cfg::StreamType streamType;
  if (queueType.value() == SAI_QUEUE_TYPE_UNICAST) {
    streamType = cfg::StreamType::UNICAST;
  } else if (queueType.value() == SAI_QUEUE_TYPE_MULTICAST) {
    streamType = cfg::StreamType::MULTICAST;
  } else {
    streamType = cfg::StreamType::ALL;
  }
  return std::make_pair(queueIndex.value(), streamType);
}
} // namespace detail

void SaiQueueHandle::resetQueue() {
  /*
   * Queues cannot be deleted as it is owned by the adapter but all
   * the queue attributes has to be reset to default. As a
   * temporary solution, resetting the queue attributes to their
   * defaults. For long term, this will be removed and resetting the
   * objects will be part of SaiObject.
   */
  SaiQueueTraits::Attributes::SchedulerProfileId schedulerId{
      SAI_NULL_OBJECT_ID};
  SaiApiTable::getInstance()->queueApi().setAttribute(
      queue->adapterKey(), schedulerId);
}

SaiQueueHandle::SaiQueueHandle(QueueSaiId queueSaiId) {
  auto& store = SaiStore::getInstance()->get<SaiQueueTraits>();
  queue = store.loadObjectOwnedByAdapter(queueSaiId);
}

SaiQueueManager::SaiQueueManager(
    SaiManagerTable* managerTable,
    const SaiPlatform* platform)
    : managerTable_(managerTable), platform_(platform) {}

void SaiQueueManager::changeQueue(
    SaiQueueHandle* queueHandle,
    const PortQueue& newPortQueue) {
  CHECK(queueHandle);
  queueHandle->scheduler =
      managerTable_->schedulerManager().createScheduler(newPortQueue);
}

void SaiQueueManager::ensurePortQueueConfig(
    PortSaiId portSaiId,
    const SaiQueueHandles& queueHandles,
    const QueueConfig& queues) {
  for (const auto& portQueue : queues) {
    SaiQueueTraits::CreateAttributes attributes =
        detail::makeQueueAttributes(portSaiId, *portQueue);
    SaiQueueTraits::AdapterHostKey adapterHostKey = tupleProjection<
        SaiQueueTraits::CreateAttributes,
        SaiQueueTraits::AdapterHostKey>(attributes);
    auto& store = SaiStore::getInstance()->get<SaiQueueTraits>();
    auto queue = store.get(adapterHostKey);
    if (!queue) {
      throw FbossError(
          "failed to find queue in store for queue id: ", portQueue->getID());
    }
    SaiQueueConfig queueConfig =
        std::make_pair(portQueue->getID(), portQueue->getStreamType());
    auto queueHandleEntry = queueHandles.find(queueConfig);
    if (queueHandleEntry == queueHandles.end()) {
      throw FbossError(
          "failed to find queue handle for queue id: ", (*portQueue).getID());
    }
    changeQueue(queueHandleEntry->second.get(), *portQueue);
  }
}

SaiQueueHandles SaiQueueManager::loadQueues(
    PortSaiId portSaiId,
    const std::vector<QueueSaiId>& queueSaiIds) {
  SaiQueueHandles queueHandles;
  auto& store = SaiStore::getInstance()->get<SaiQueueTraits>();
  std::vector<std::shared_ptr<SaiQueue>> loadedQueues;
  for (auto queueSaiId : queueSaiIds) {
    auto queueHandle = std::make_unique<SaiQueueHandle>(queueSaiId);
    store.loadObjectOwnedByAdapter(SaiQueueTraits::AdapterKey{queueSaiId});
    auto saiQueueConfig =
        detail::makeSaiQueueConfig(queueHandle->queue->adapterHostKey());
    queueHandles[saiQueueConfig] = std::move(queueHandle);
  }
  return queueHandles;
}

void SaiQueueManager::updateStats(
    SaiQueueHandles& queueHandles,
    HwPortStats& hwPortStats) {
  for (auto& queueHandle : queueHandles) {
    queueHandle.second->queue->updateStats();
  }
  getStats(queueHandles, hwPortStats);
}

void SaiQueueManager::getStats(
    SaiQueueHandles& queueHandles,
    HwPortStats& hwPortStats) {
  for (auto& queueHandle : queueHandles) {
    const auto& counters = queueHandle.second->queue->getStats();
    fillHwQueueStats(queueHandle.first.first, counters, hwPortStats);
  }
}

QueueConfig SaiQueueManager::getQueueSettings(
    const SaiQueueHandles& queueHandles) const {
  QueueConfig queueConfig;
  for (auto& queueHandle : queueHandles) {
    auto portQueue = std::make_shared<PortQueue>(queueHandle.first.first);
    portQueue->setStreamType(queueHandle.first.second);
    managerTable_->schedulerManager().fillSchedulerSettings(
        queueHandle.second->scheduler.get(), portQueue.get());
    queueConfig.push_back(std::move(portQueue));
  }
  return queueConfig;
}

} // namespace facebook::fboss
