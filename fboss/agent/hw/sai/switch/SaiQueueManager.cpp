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

namespace detail {
SaiQueueTraits::CreateAttributes makeQueueAttributes(
    PortSaiId portSaiId,
    const PortQueue& portQueue) {
  auto type = portQueue.getStreamType() == cfg::StreamType::UNICAST
      ? SAI_QUEUE_TYPE_UNICAST
      : SAI_QUEUE_TYPE_MULTICAST;
  return SaiQueueTraits::CreateAttributes{
      type, portSaiId, portQueue.getID(), portSaiId};
}

SaiQueueConfig makeSaiQueueConfig(
    const SaiQueueTraits::AdapterHostKey& adapterHostKey) {
  auto queueIndex = std::get<SaiQueueTraits::Attributes::Index>(adapterHostKey);
  auto queueType = std::get<SaiQueueTraits::Attributes::Type>(adapterHostKey);
  cfg::StreamType streamType = queueType.value() == SAI_QUEUE_TYPE_UNICAST
      ? cfg::StreamType::UNICAST
      : cfg::StreamType::MULTICAST;
  return std::make_pair(queueIndex.value(), streamType);
}
} // namespace detail

SaiQueueManager::SaiQueueManager(
    SaiManagerTable* managerTable,
    const SaiPlatform* platform)
    : managerTable_(managerTable), platform_(platform) {}

void SaiQueueManager::changeQueue(
    SaiQueueHandle* queueHandle,
    const PortQueue& newPortQueue) {
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
    const std::vector<QueueSaiId>& queueSaiIds,
    const QueueConfig& queues) {
  SaiQueueHandles queueHandles;
  auto& store = SaiStore::getInstance()->get<SaiQueueTraits>();
  std::vector<std::shared_ptr<SaiQueue>> loadedQueues;
  for (auto queueSaiId : queueSaiIds) {
    auto queueHandle = std::make_unique<SaiQueueHandle>();
    queueHandle->queue =
        store.loadObjectOwnedByAdapter(SaiQueueTraits::AdapterKey{queueSaiId});
    auto saiQueueConfig =
        detail::makeSaiQueueConfig(queueHandle->queue->adapterHostKey());
    queueHandles[saiQueueConfig] = std::move(queueHandle);
  }
  ensurePortQueueConfig(portSaiId, queueHandles, queues);
  return queueHandles;
}

} // namespace facebook::fboss
