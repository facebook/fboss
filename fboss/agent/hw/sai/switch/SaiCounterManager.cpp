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
#include "fboss/agent/hw/sai/api/SaiApiTable.h"
#include "fboss/agent/hw/sai/store/SaiStore.h"
#include "fboss/agent/hw/sai/switch/SaiManagerTable.h"
#include "fboss/agent/hw/sai/switch/SaiSwitchManager.h"
#include "fboss/agent/hw/switch_asics/HwAsic.h"
#include "fboss/agent/platforms/sai/SaiPlatform.h"

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
    auto& counterStore = saiStore_->get<SaiCounterTraits>();
    entry->counter = counterStore.setObject(attrs, attrs);
    routeStats_->reinitStat(counterID, std::nullopt);
  }
  return entry;
#else
  return nullptr;
#endif
}

} // namespace facebook::fboss
