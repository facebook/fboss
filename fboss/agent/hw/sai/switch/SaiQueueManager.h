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
#include "fboss/agent/hw/sai/api/SaiApiTable.h"
#include "fboss/agent/state/PortQueue.h"
#include "fboss/agent/state/StateDelta.h"
#include "fboss/agent/types.h"

#include <memory>
#include <unordered_map>

namespace facebook {
namespace fboss {

class SaiManagerTable;
class SaiPlatform;

class SaiQueue {
 public:
  SaiQueue(
      SaiApiTable* apiTable,
      SaiManagerTable* managerTable,
      sai_object_id_t saiPortId,
      const QueueApiParameters::Attributes& attributes);
  SaiQueue(
      SaiApiTable* apiTable,
      sai_object_id_t id,
      sai_object_id_t saiPortId,
      const QueueApiParameters::Attributes& attributes);
  ~SaiQueue();
  SaiQueue(const SaiQueue& other) = delete;
  SaiQueue(SaiQueue&& other) = delete;
  SaiQueue& operator=(const SaiQueue& other) = delete;
  SaiQueue& operator=(SaiQueue&& other) = delete;
  bool operator==(const SaiQueue& other) const;
  bool operator!=(const SaiQueue& other) const;
  void updatePortQueue(PortQueue& portQueue);

  const QueueApiParameters::Attributes attributes() const {
    return attributes_;
  }
  sai_object_id_t id() const {
    return id_;
  }

  sai_object_id_t getSaiPortId() {
    return saiPortId_;
  }

 private:
  SaiApiTable* apiTable_;
  sai_object_id_t id_;
  sai_object_id_t saiPortId_;
  QueueApiParameters::Attributes attributes_;
};

class SaiQueueManager {
 public:
  SaiQueueManager(
      SaiApiTable* apiTable,
      SaiManagerTable* managerTable,
      const SaiPlatform* platform);
  std::unique_ptr<SaiQueue> createQueue(
      sai_object_id_t saiPortId,
      uint32_t numQueues,
      PortQueue& portQueue);

 private:
  SaiApiTable* apiTable_;
  SaiManagerTable* managerTable_;
  const SaiPlatform* platform_;
};

} // namespace fboss
} // namespace facebook
