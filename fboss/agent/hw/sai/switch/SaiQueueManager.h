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

#include "fboss/agent/hw/sai/api/QueueApi.h"
#include "fboss/agent/hw/sai/store/SaiObject.h"
#include "fboss/agent/state/PortQueue.h"
#include "fboss/agent/state/StateDelta.h"
#include "fboss/agent/types.h"

#include "folly/container/F14Map.h"

#include <memory>

namespace facebook::fboss {

class SaiManagerTable;
class SaiPlatform;

using SaiQueue = SaiObject<SaiQueueTraits>;

class SaiQueueManager {
 public:
  SaiQueueManager(SaiManagerTable* managerTable, const SaiPlatform* platform);

  std::shared_ptr<SaiQueue> createQueue(
      PortSaiId portSaiId,
      const PortQueue& portQueue);

  folly::F14FastMap<uint8_t, std::shared_ptr<SaiQueue>> createQueues(
      PortSaiId portSaiId,
      const QueueConfig& queues);

 private:
  SaiManagerTable* managerTable_;
  const SaiPlatform* platform_;
};

} // namespace facebook::fboss
