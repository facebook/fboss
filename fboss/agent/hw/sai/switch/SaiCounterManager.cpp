/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/sai/switch/SaiCounterManager.h"
#include "fboss/agent/hw/sai/api/CounterApi.h"
#include "fboss/agent/hw/sai/store/SaiStore.h"
#include "fboss/agent/hw/sai/switch/SaiManagerTable.h"
#include "fboss/agent/hw/sai/switch/SaiSwitchManager.h"

namespace facebook::fboss {

std::shared_ptr<SaiCounterHandle> SaiCounterManager::incRefOrAddRouteCounter(
    std::string counterID) {
#if SAI_API_VERSION >= SAI_VERSION(1, 10, 0)
  auto [entry, emplaced] =
      routeCounters_.refOrEmplace(counterID, counterID, getRouteStatsMapRef());
  if (emplaced) {
    SaiCharArray32 labelValue{};
    if (counterID.size() > 32) {
      throw FbossError(
          "CounterID label size ", counterID.size(), " exceeds max(32)");
    }
    std::copy(counterID.begin(), counterID.end(), labelValue.begin());

    SaiCounterTraits::CreateAttributes attrs{
        labelValue, SAI_COUNTER_TYPE_REGULAR};
    if (routeCounters_.size() > maxRouteCounterIDs_) {
      XLOG(ERR) << "RouteCounterIDs in use " << routeCounters_.size()
                << " exceed max count " << maxRouteCounterIDs_;
      throw FbossError(
          fmt::format(
              "CounterIDs in use: {} exceeds max: {}",
              routeCounters_.size(),
              maxRouteCounterIDs_));
    }
    auto& counterStore = saiStore_->get<SaiCounterTraits>();
    entry->counter = counterStore.setObject(attrs, attrs);
    routeStats_->reinitStat(counterID, std::nullopt);
  }
  return entry;
#else
  return nullptr;
#endif
}

void SaiCounterManager::updateStats() {
  auto now = std::chrono::duration_cast<std::chrono::seconds>(
      std::chrono::system_clock::now().time_since_epoch());
  for (const auto& counter : routeCounters_) {
    auto counterName = counter.first;
    auto counterHandle = counter.second.lock();
#if SAI_API_VERSION >= SAI_VERSION(1, 10, 0)
    counterHandle->counter->updateStats(
        {SAI_COUNTER_STAT_BYTES}, SAI_STATS_MODE_READ);
#endif
    auto& stats = counterHandle->counter->getStats();
    routeStats_->updateStat(now, counterName, stats.begin()->second);
  }
}

uint64_t SaiCounterManager::getStats(std::string counterID) const {
  auto handle = routeCounters_.get(counterID);
#if SAI_API_VERSION >= SAI_VERSION(1, 10, 0)
  handle->counter->updateStats({SAI_COUNTER_STAT_BYTES}, SAI_STATS_MODE_READ);
#endif
  auto stats = handle->counter->getStats();
  return stats.begin()->second;
}

} // namespace facebook::fboss
