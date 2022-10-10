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

#include "fboss/agent/hw/sai/api/SaiApi.h"
#include "fboss/agent/hw/sai/api/SaiAttribute.h"
#include "fboss/agent/hw/sai/api/SaiAttributeDataTypes.h"
#include "fboss/agent/hw/sai/api/SaiVersion.h"
#include "fboss/agent/hw/sai/api/Types.h"

#include <folly/logging/xlog.h>

#include <optional>
#include <tuple>

extern "C" {
#include <sai.h>
}

namespace facebook::fboss {

class CounterApi;

struct SaiCounterTraits {
  static constexpr sai_object_type_t ObjectType = SAI_OBJECT_TYPE_COUNTER;
  using SaiApiT = CounterApi;
  struct Attributes {
    using EnumType = sai_counter_attr_t;
    using Type = SaiAttribute<EnumType, SAI_COUNTER_ATTR_TYPE, sai_int32_t>;
#if SAI_API_VERSION >= SAI_VERSION(1, 10, 0)
    using Label =
        SaiAttribute<EnumType, SAI_COUNTER_ATTR_LABEL, SaiCharArray32>;
#endif
  };
  using AdapterKey = CounterSaiId;
#if SAI_API_VERSION >= SAI_VERSION(1, 10, 0)
  using CreateAttributes = std::tuple<Attributes::Label, Attributes::Type>;
#else
  using CreateAttributes = std::tuple<Attributes::Type>;
#endif
  using AdapterHostKey = CreateAttributes;
};

SAI_ATTRIBUTE_NAME(Counter, Type);
#if SAI_API_VERSION >= SAI_VERSION(1, 10, 0)
SAI_ATTRIBUTE_NAME(Counter, Label);
#endif

template <>
struct SaiObjectHasStats<SaiCounterTraits> : public std::true_type {};

class CounterApi : public SaiApi<CounterApi> {
 public:
  static constexpr sai_api_t ApiType = SAI_API_COUNTER;
  CounterApi() {
    sai_status_t status =
        sai_api_query(ApiType, reinterpret_cast<void**>(&api_));
    saiApiCheckError(status, ApiType, "Failed to query for counter api");
  }
  sai_status_t _create(
      CounterSaiId* id,
      sai_object_id_t switch_id,
      size_t count,
      sai_attribute_t* attr_list) const {
    return api_->create_counter(rawSaiId(id), switch_id, count, attr_list);
  }

  sai_status_t _remove(CounterSaiId id) const {
    return api_->remove_counter(id);
  }

  sai_status_t _getAttribute(CounterSaiId id, sai_attribute_t* attr) const {
    return api_->get_counter_attribute(id, 1, attr);
  }

  sai_status_t _setAttribute(CounterSaiId id, const sai_attribute_t* attr)
      const {
    return api_->set_counter_attribute(id, attr);
  }

  sai_status_t _getStats(
      CounterSaiId id,
      uint32_t num_of_counters,
      const sai_stat_id_t* counter_ids,
      sai_stats_mode_t mode,
      uint64_t* counters) const {
    return mode == SAI_STATS_MODE_READ
        ? api_->get_counter_stats(id, num_of_counters, counter_ids, counters)
        : api_->get_counter_stats_ext(
              id, num_of_counters, counter_ids, mode, counters);
  }

  sai_status_t _clearStats(
      CounterSaiId id,
      uint32_t num_of_counters,
      const sai_stat_id_t* counter_ids) const {
    return api_->clear_counter_stats(id, num_of_counters, counter_ids);
  }

  sai_counter_api_t* api_;
  friend class SaiApi<CounterApi>;
};
} // namespace facebook::fboss
