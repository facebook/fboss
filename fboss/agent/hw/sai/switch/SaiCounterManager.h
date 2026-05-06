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
#include "fboss/agent/hw/sai/api/CounterApi.h"
#include "fboss/agent/hw/sai/store/SaiObject.h"
#include "fboss/agent/hw/sai/store/SaiObjectWithCounters.h"
#include "fboss/agent/if/gen-cpp2/ctrl_types.h"
#include "fboss/agent/platforms/sai/SaiPlatform.h"

#include "fboss/lib/RefMap.h"

#include <memory>

namespace facebook::fboss {

class SaiManagerTable;
class StateDelta;
class SaiStore;
class SaiPlatform;

using SaiCounter = SaiObjectWithCounters<SaiCounterTraits>;

struct SaiCounterHandle {
  std::shared_ptr<SaiCounter> counter;
  std::string counterName;
  HwSwitchCounter hwSwitchCounter;
  explicit SaiCounterHandle(std::string name) : counterName(name) {}
  SaiCounterHandle(const SaiCounterHandle&) = delete;
  SaiCounterHandle& operator=(const SaiCounterHandle&) = delete;
  SaiCounterHandle(SaiCounterHandle&&) = delete;
  SaiCounterHandle& operator=(SaiCounterHandle&&) = delete;
  CounterSaiId adapterKey() {
    return counter->adapterKey();
  }
};

class SaiCounterManager {
 public:
  explicit SaiCounterManager(
      SaiStore* saiStore,
      SaiManagerTable* managerTable,
      SaiPlatform* platform)
      : saiStore_(saiStore), managerTable_(managerTable), platform_(platform) {}

  std::shared_ptr<SaiCounterHandle> incRefOrAddRouteCounter(
      std::string counterID);
  uint64_t getStats(std::string counterID) const;
  void updateStats();
  HwSwitchCounterStats getHwSwitchCounterStats() const;

 private:
  SaiStore* saiStore_;
  SaiManagerTable* managerTable_;
  SaiPlatform* platform_;
  FlatRefMap<RouteCounterID, SaiCounterHandle> routeCounters_;
};

} // namespace facebook::fboss
