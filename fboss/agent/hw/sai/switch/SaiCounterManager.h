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

#include <fboss/agent/hw/HwFb303Stats.h>
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
  std::shared_ptr<HwFb303Stats> statsMap;
  SaiCounterHandle(std::string name, std::shared_ptr<HwFb303Stats> map)
      : counterName(name), statsMap(map) {}
  ~SaiCounterHandle() {
    if (counter) {
      counter.reset();
      statsMap->removeStat(counterName);
    }
  }
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
      : saiStore_(saiStore), managerTable_(managerTable), platform_(platform) {
    routeStats_ =
        std::make_shared<HwFb303Stats>(platform->getMultiSwitchStatsPrefix());
  }

  std::shared_ptr<HwFb303Stats> getRouteStatsMapRef() {
    return routeStats_;
  }
  std::shared_ptr<SaiCounterHandle> incRefOrAddRouteCounter(
      std::string counterID);
  void setMaxRouteCounterIDs(uint32_t count) {
    maxRouteCounterIDs_ = count;
  }
  uint64_t getStats(std::string counterID) const;
  void updateStats();

 private:
  SaiStore* saiStore_;
  SaiManagerTable* managerTable_;
  SaiPlatform* platform_;
  FlatRefMap<RouteCounterID, SaiCounterHandle> routeCounters_;
  std::shared_ptr<HwFb303Stats> routeStats_;
  uint32_t maxRouteCounterIDs_;
};

} // namespace facebook::fboss
