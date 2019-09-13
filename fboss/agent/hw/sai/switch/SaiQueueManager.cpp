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

namespace facebook {
namespace fboss {

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
} // namespace detail

SaiQueueManager::SaiQueueManager(
    SaiManagerTable* managerTable,
    const SaiPlatform* platform)
    : managerTable_(managerTable), platform_(platform) {}

std::shared_ptr<SaiQueue> SaiQueueManager::createQueue(
    PortSaiId portSaiId,
    const PortQueue& portQueue) {
  SaiQueueTraits::CreateAttributes attributes =
      detail::makeQueueAttributes(portSaiId, portQueue);
  SaiQueueTraits::AdapterHostKey k = tupleProjection<
      SaiQueueTraits::CreateAttributes,
      SaiQueueTraits::AdapterHostKey>(attributes);
  auto& store = SaiStore::getInstance()->get<SaiQueueTraits>();
  return store.setObject(k, attributes);
}

folly::F14FastMap<uint8_t, std::shared_ptr<SaiQueue>>
SaiQueueManager::createQueues(PortSaiId portSaiId, const QueueConfig& queues) {
  folly::F14FastMap<uint8_t, std::shared_ptr<SaiQueue>> ret;
  for (const auto& q : queues) {
    ret[q->getID()] = createQueue(portSaiId, *q);
  }
  return ret;
}

} // namespace fboss
} // namespace facebook
