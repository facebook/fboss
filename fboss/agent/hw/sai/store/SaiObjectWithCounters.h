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
#include "fboss/agent/hw/sai/api/Traits.h"
#include "fboss/agent/hw/sai/store/SaiObject.h"
#include "fboss/lib/RefMap.h"
#include "fboss/lib/TupleUtils.h"

#include <variant>

namespace facebook::fboss {

template <typename SaiObjectTraits>
class SaiObjectWithCounters : public SaiObject<SaiObjectTraits> {
 public:
  friend class SaiObjectStore<SaiObjectTraits>;
  friend class ::SaiStoreTest;
  // Load from adapter key
  explicit SaiObjectWithCounters(
      const typename SaiObjectTraits::AdapterKey& adapterKey)
      : SaiObject<SaiObjectTraits>(adapterKey) {}

  // Create a new one from adapter host key and attributes
  SaiObjectWithCounters(
      const typename SaiObjectTraits::AdapterHostKey& adapterHostKey,
      const typename SaiObjectTraits::CreateAttributes& attributes,
      sai_object_id_t switchId)
      : SaiObject<SaiObjectTraits>(adapterHostKey, attributes, switchId) {}

  using StatsMap = folly::F14FastMap<sai_stat_id_t, uint64_t>;

  template <typename T = SaiObjectTraits>
  void updateStats() {
    static_assert(SaiObjectHasStats<T>::value, "invalid traits for the api");
    auto& api = SaiApiTable::getInstance()->getApi<typename T::SaiApiT>();
    const auto& countersToRead =
        api.template getStats<T>(this->adapterKey(), SAI_STATS_MODE_READ);
    fillInStats(SaiObjectTraits::CounterIdsToRead.data(), countersToRead);
    const auto& countersToReadAndClear = api.template getStats<T>(
        this->adapterKey(), SAI_STATS_MODE_READ_AND_CLEAR);
    fillInStats(
        SaiObjectTraits::CounterIdsToReadAndClear.data(),
        countersToReadAndClear);
  }

  template <typename T = SaiObjectTraits>
  void updateStats(
      const std::vector<sai_stat_id_t>& counterIds,
      sai_stats_mode_t mode) {
    static_assert(SaiObjectHasStats<T>::value, "invalid traits for the api");
    auto& api = SaiApiTable::getInstance()->getApi<typename T::SaiApiT>();
    const auto& counters =
        api.template getStats<T>(this->adapterKey(), counterIds, mode);
    fillInStats(counterIds.data(), counters);
  }

  template <typename T = SaiObjectTraits>
  const StatsMap getStats() const {
    static_assert(SaiObjectHasStats<T>::value, "invalid traits for the api");
    return counterId2Value_;
  }

  template <typename T = SaiObjectTraits>
  const StatsMap getStats(const std::vector<sai_stat_id_t>& counterIds) const {
    static_assert(SaiObjectHasStats<T>::value, "invalid traits for the api");
    StatsMap toRet;
    for (auto counterId : counterIds) {
      auto citr = counterId2Value_.find(counterId);
      if (citr == counterId2Value_.end()) {
        throw FbossError("Could not find counterId: ", counterId);
      }
      toRet.insert(*citr);
    }
    return toRet;
  }
  template <typename T = SaiObjectTraits>
  void clearStats(const std::vector<sai_stat_id_t>& counterIds) const {
    static_assert(SaiObjectHasStats<T>::value, "invalid traits for the api");
    const auto& api = SaiApiTable::getInstance()->getApi<typename T::SaiApiT>();
    api.template clearStats<T>(this->adapterKey(), counterIds);
  }

  template <typename T = SaiObjectTraits>
  void clearStats() const {
    static_assert(SaiObjectHasStats<T>::value, "invalid traits for the api");
    const auto& api = SaiApiTable::getInstance()->getApi<typename T::SaiApiT>();
    api.template clearStats<T>(this->adapterKey());
  }

 private:
  void fillInStats(
      const sai_stat_id_t* ids,
      const std::vector<uint64_t>& counters) {
    for (auto i = 0; i < counters.size(); ++i) {
      counterId2Value_[ids[i]] = counters[i];
    }
  }
  StatsMap counterId2Value_;
};

} // namespace facebook::fboss
