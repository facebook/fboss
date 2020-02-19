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

#include "fboss/agent/hw/sai/api/SaiApiTable.h"
#include "fboss/agent/hw/sai/api/SchedulerApi.h"
#include "fboss/agent/hw/sai/store/SaiObject.h"
#include "fboss/agent/state/PortQueue.h"
#include "fboss/agent/state/StateDelta.h"
#include "fboss/agent/types.h"

#include <memory>
#include <unordered_map>

namespace facebook::fboss {

class SaiManagerTable;
class SaiPlatform;

using SaiScheduler = SaiObject<SaiSchedulerTraits>;

class SaiSchedulerManager {
 public:
  SaiSchedulerManager(
      SaiManagerTable* managerTable,
      const SaiPlatform* platform);
  std::shared_ptr<SaiScheduler> createScheduler(const PortQueue& portQueue);
  void fillSchedulerSettings(
      const SaiScheduler* scheduler,
      PortQueue* portQueue) const;

 private:
  SaiApiTable* apiTable_;
  SaiManagerTable* managerTable_;
  const SaiPlatform* platform_;
};

} // namespace facebook::fboss
