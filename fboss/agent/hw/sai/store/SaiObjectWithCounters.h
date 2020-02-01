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
#include "fboss/lib/TupleUtils.h"

#include <variant>

namespace facebook::fboss {

template <typename SaiObjectTraits>
class SaiObjectWithCounters : public SaiObject<SaiObjectTraits> {
 public:
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

  template <typename T = SaiObjectTraits>
  void updateStats() {
    static_assert(SaiObjectHasStats<T>::value, "invalid traits for the api");
    auto& api = SaiApiTable::getInstance()->getApi<typename T::SaiApiT>();
    counters_ = api.template getStats<T>(this->adapterKey());
  }

  template <typename T = SaiObjectTraits>
  const std::vector<uint64_t>& getStats() const {
    static_assert(SaiObjectHasStats<T>::value, "invalid traits for the api");
    return counters_;
  }

 private:
  std::vector<uint64_t> counters_;
};

} // namespace facebook::fboss
